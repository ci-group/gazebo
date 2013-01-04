/*
 * Copyright 2012 Open Source Robotics Foundation
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
#include "gazebo/gui/Gui.hh"
#include "gazebo/gui/GuiEvents.hh"

#include "gazebo/transport/Transport.hh"
#include "gazebo/transport/Node.hh"
#include "gazebo/transport/Publisher.hh"

#include "gazebo/gui/viewers/TopicView.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
TopicView::TopicView(const std::string &_msgTypeName,
                     const std::string &_viewType)
: QWidget(), msgTypeName(_msgTypeName)
{
  this->node = transport::NodePtr(new transport::Node());
  this->node->Init();

  this->setWindowIcon(QIcon(":/images/gazebo.svg"));
  this->setWindowTitle(tr("Gazebo: Topic View"));
  this->setObjectName("cameraSensor");

  // Create the topic label and combo box
  // {
  QHBoxLayout *topicLayout = new QHBoxLayout;
  QLabel *topicLabel = new QLabel(tr("Topic: "));
  this->topicCombo = new TopicCombo(this, this->msgTypeName,
      _viewType, this->node);
  this->topicCombo->setObjectName("comboList");
  this->topicCombo->setMinimumSize(300, 25);

  topicLayout->addSpacing(10);
  topicLayout->addWidget(topicLabel);
  topicLayout->addWidget(this->topicCombo);
  topicLayout->addSpacing(10);
  topicLayout->addStretch(4);
  // }

  // Create the Hz and bandwidth labels
  // {
  QHBoxLayout *infoLayout = new QHBoxLayout;
  QLabel *hzLabel = new QLabel("Hz: ");
  this->hzEdit = new QLineEdit;
  this->hzEdit->setReadOnly(true);
  this->hzEdit->setFixedWidth(80);

  QLabel *bandwidthLabel = new QLabel("Bandwidth: ");
  this->bandwidthEdit = new QLineEdit;
  this->bandwidthEdit->setReadOnly(true);
  this->bandwidthEdit->setFixedWidth(110);

  infoLayout->addSpacing(10);
  infoLayout->addWidget(hzLabel);
  infoLayout->addWidget(this->hzEdit);
  infoLayout->addSpacing(4);
  infoLayout->addStretch(1);
  infoLayout->addWidget(bandwidthLabel);
  infoLayout->addWidget(this->bandwidthEdit);
  infoLayout->addStretch(4);
  // }

  // Create the frame used to display information
  this->frame = new QFrame;

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addLayout(topicLayout);
  mainLayout->addLayout(infoLayout);
  mainLayout->addWidget(frame);
  this->setLayout(mainLayout);
  this->layout()->setContentsMargins(4, 4, 4, 4);

  QTimer::singleShot(500, this, SLOT(Update()));
}

/////////////////////////////////////////////////
TopicView::~TopicView()
{
  delete this->topicCombo;
}

/////////////////////////////////////////////////
void TopicView::Update()
{
  // Update the child class.
  this->UpdateImpl();

  // Update the Hz output
  {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << this->hz;
    this->hzEdit->setText(tr(stream.str().c_str()));
  }

  // Update the Bandwidth output
  {
    std::ostringstream stream;

    // Sum up the byte information
    int sumBytes = 0;
    for (std::list<int>::iterator iter = this->msgSizes.begin();
        iter != this->msgSizes.end(); ++iter)
    {
      sumBytes += *iter;
    }

    // Compute the bandwidth
    common::Time dt = this->times.back() - this->times.front();
    double bandwidth = 0;

    if (dt != common::Time(0, 0))
      bandwidth = sumBytes / dt.Double();

    // Format the bandwidth output
    stream << std::fixed << std::setprecision(2);

    if (bandwidth < 1000)
      stream << bandwidth << " B/s";
    else if (bandwidth < 1000000)
      stream << bandwidth / 1024.0f << " KB/s";
    else
      stream << bandwidth/1.049e6 << " MB/s";

    this->bandwidthEdit->setText(tr(stream.str().c_str()));
  }

  // Set the timer to update again.
  QTimer::singleShot(500, this, SLOT(Update()));
}

/////////////////////////////////////////////////
void TopicView::OnMsg(const common::Time &_dataTime, int _size)
{
  // Calculate the Hz value.
  if (_dataTime != this->prevTime)
    this->hz = 1.0 / (_dataTime - this->prevTime).Double();

  // Store the previous time for future Hz calculations.
  this->prevTime = _dataTime;

  // Store the message size and clock time that it was received.
  this->msgSizes.push_back(_size);
  this->times.push_back(common::Time::GetWallTime());

  // Maintain a buffer of only 100 data points.
  if (this->msgSizes.size() > 100)
  {
    this->msgSizes.pop_front();
    this->times.pop_front();
  }
}

/////////////////////////////////////////////////
void TopicView::OnTopicChanged(int _index)
{
  // Set the current topic based on the index of the item selected in the
  // combobox
  this->SetTopic(this->topicCombo->itemText(_index).toStdString());
}

/////////////////////////////////////////////////
void TopicView::SetTopic(const std::string &_topicName)
{
  if (_topicName.empty())
    return;

  this->hz = 0.0;
  this->msgSizes.clear();
  this->times.clear();
  std::string topicName = this->node->EncodeTopicName(_topicName);

  disconnect(this->topicCombo, SIGNAL(currentIndexChanged(int)),
             this, SLOT(OnTopicChanged(int)));

  int index = this->topicCombo->findText(QString::fromStdString(topicName));
  if (index >= 0)
    this->topicCombo->setCurrentIndex(index);

  connect(this->topicCombo, SIGNAL(currentIndexChanged(int)),
          this, SLOT(OnTopicChanged(int)));
}

/////////////////////////////////////////////////
void TopicView::UpdateImpl()
{
}

/////////////////////////////////////////////////
TopicCombo::TopicCombo(QWidget *_w,
    const std::string &_msgTypeName,
    const std::string &_viewType,
    transport::NodePtr _node)
: QComboBox(_w), msgTypeName(_msgTypeName), viewType(_viewType), node(_node)
{
  this->UpdateList();
}

/////////////////////////////////////////////////
TopicCombo::~TopicCombo()
{
  this->node.reset();
}

/////////////////////////////////////////////////
void TopicCombo::showPopup()
{
  this->UpdateList();

  // show the list
  QComboBox::showPopup();
}

/////////////////////////////////////////////////
void TopicCombo::UpdateList()
{
  QString myText = this->currentText();

  this->blockSignals(true);

  // First clear out the combo box.
  this->clear();

  std::list<std::string> topics;

  // Get only the topics that match the message type, if the current viewer
  // is not a text viewer.
  if (this->viewType != "text")
  {
    // Get the list of all topics filtered by our message type.
    topics = transport::getAdvertisedTopics(this->msgTypeName);
  }
  // Otherwise select all the topics to show in the combo box.
  else
  {
    std::map<std::string, std::list<std::string> > allTopics;
    allTopics = transport::getAdvertisedTopics();

    for (std::map<std::string, std::list<std::string> >::iterator
         iter = allTopics.begin(); iter != allTopics.end(); ++iter)
    {
      // Add all the topic names
      for (std::list<std::string>::iterator topicIter = iter->second.begin();
           topicIter != iter->second.end(); ++topicIter)
      {
        if ((*topicIter).find("__dbg") == std::string::npos &&
            std::find(topics.begin(), topics.end(), *topicIter) == topics.end())
        {
          topics.push_back(*topicIter);
        }
      }
    }
  }

  // Add each topic to the combo box.
  for (std::list<std::string>::iterator iter = topics.begin();
      iter != topics.end(); ++iter)
  {
    if ((*iter).find("__dbg") != std::string::npos)
      continue;

    // Get the shorthand notation for the topic.
    std::string topicName = this->node->EncodeTopicName(*iter);

    this->addItem(QString::fromStdString(topicName));
  }

  int index = this->findText(myText);
  if (index >= 0)
    this->setCurrentIndex(index);

  this->blockSignals(false);
}
