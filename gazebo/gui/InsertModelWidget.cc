/*
 * Copyright 2011 Nate Koenig & Andrew Howard
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#define BOOST_FILESYSTEM_VERSION 2
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include "sdf/sdf.hh"
#include "common/SystemPaths.hh"
#include "common/Console.hh"

#include "rendering/Rendering.hh"
#include "rendering/Scene.hh"
#include "rendering/UserCamera.hh"
#include "rendering/Visual.hh"
#include "gui/Gui.hh"
#include "gui/GuiEvents.hh"

#include "transport/Node.hh"
#include "transport/Publisher.hh"

#include "gui/InsertModelWidget.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
InsertModelWidget::InsertModelWidget(QWidget *_parent)
: QWidget(_parent)
{
  this->setObjectName("insertModel");

  QVBoxLayout *mainLayout = new QVBoxLayout;
  this->fileTreeWidget = new QTreeWidget();
  this->fileTreeWidget->setColumnCount(1);
  this->fileTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  this->fileTreeWidget->header()->hide();
  connect(this->fileTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
      this, SLOT(OnModelSelection(QTreeWidgetItem *, int)));

  QFrame *frame = new QFrame;
  QVBoxLayout *frameLayout = new QVBoxLayout;
  frameLayout->addWidget(this->fileTreeWidget, 0);
  frameLayout->setContentsMargins(0, 0, 0, 0);
  frame->setLayout(frameLayout);


  mainLayout->addWidget(frame);
  this->setLayout(mainLayout);
  this->layout()->setContentsMargins(0, 0, 0, 0);

  std::list<std::string> gazeboPaths =
    common::SystemPaths::Instance()->GetModelPaths();

  // Iterate over all the gazebo paths
  for (std::list<std::string>::iterator iter = gazeboPaths.begin();
      iter != gazeboPaths.end(); ++iter)
  {
    // This is the full model path
    std::string path = (*iter);

    // Create a top-level tree item for the path
    QTreeWidgetItem *topItem =
      new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(0),
          QStringList(QString("%1").arg(QString::fromStdString(path))));
    this->fileTreeWidget->addTopLevelItem(topItem);

    boost::filesystem::path dir(path);
    std::list<boost::filesystem::path> resultSet;

    if (boost::filesystem::exists(dir) &&
        boost::filesystem::is_directory(dir))
    {
      std::vector<boost::filesystem::path> paths;
      std::copy(boost::filesystem::directory_iterator(dir),
                boost::filesystem::directory_iterator(),
                std::back_inserter(paths));
      std::sort(paths.begin(), paths.end());

      // Iterate over all the models in the current gazebo path
      for (std::vector<boost::filesystem::path>::iterator dIter = paths.begin();
          dIter != paths.end(); ++dIter)
      {
        // This is for boost::filesystem version 3+
        std::string modelName;
        std::string fullPath = path + "/" + dIter->filename();
        std::string manifest = fullPath + "/manifest.xml";

        TiXmlDocument xmlDoc;
        if (xmlDoc.LoadFile(manifest))
        {
          TiXmlElement *modelXML = xmlDoc.FirstChildElement("model");
          if (!modelXML || !modelXML->FirstChildElement("name"))
            gzerr << "No model name in manifest[" << manifest << "]\n";
          else
            modelName = modelXML->FirstChildElement("name")->GetText();
        }

        // Add a child item for the model
        QTreeWidgetItem *childItem = new QTreeWidgetItem(topItem,
            QStringList(QString("%1").arg(
                QString::fromStdString(modelName))));
        childItem->setData(0, Qt::UserRole, QVariant(fullPath.c_str()));
        this->fileTreeWidget->addTopLevelItem(childItem);
      }
    }

    // Make all top-level items expanded. Trying to reduce mouse clicks.
    this->fileTreeWidget->expandItem(topItem);
  }

  transport::Connection *conn = new transport::Connection();
  conn->Connect("127.0.0.1", 5018);
  conn->AsyncRead(boost::bind(&InsertModelWidget::OnResponse, this, _1));

  msgs::Request *request = msgs::CreateRequest("models");
  std::string requestData;
  request->SerializeToString(&requestData);
  conn->EnqueueMsg(requestData);

  conn->ProcessWriteQueue();
}

/////////////////////////////////////////////////
void InsertModelWidget::OnResponse(const std::string &_data)
{
  std::cout << "RR:" << _data << "||\n";
  msgs::Response response;
  msgs::GzString_V models;

  response.ParseFromString(_data);
  models.ParseFromString(response.serialized_data());
}

/////////////////////////////////////////////////
InsertModelWidget::~InsertModelWidget()
{
}

/////////////////////////////////////////////////
void InsertModelWidget::OnModelSelection(QTreeWidgetItem *_item,
    int /*_column*/)
{
  if (_item)
  {
    std::string path, manifest, filename;

    QApplication::setOverrideCursor(Qt::BusyCursor);

    if (_item->parent())
      path = _item->parent()->text(0).toStdString() + "/";

    path = _item->data(0, Qt::UserRole).toString().toStdString();
    manifest = path + "/manifest.xml";


    TiXmlDocument xmlDoc;
    if (xmlDoc.LoadFile(manifest))
    {
      TiXmlElement *modelXML = xmlDoc.FirstChildElement("model");
      if (!modelXML)
        gzerr << "No <model> element in manifest[" << manifest << "]\n";
      else
      {
        filename = path + "/" + modelXML->FirstChildElement("sdf")->GetText();
      }
    }

    if (!filename.empty())
    {
      gui::Events::createEntity("model", filename);
      this->fileTreeWidget->clearSelection();
    }

    QApplication::setOverrideCursor(Qt::ArrowCursor);
  }
}
