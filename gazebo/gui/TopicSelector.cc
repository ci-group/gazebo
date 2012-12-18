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

#include "gazebo/transport/transport.hh"

#include "gazebo/common/Events.hh"
#include "gazebo/gui/JointControlWidget.hh"
#include "gazebo/gui/TopicSelector.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
TopicSelector::TopicSelector(QWidget *_parent)
  : QWidget(_parent)
{
  // This name is used in the qt style sheet
  this->setObjectName("topicSelector");

  // Create the main layout for this widget
  QVBoxLayout *mainLayout = new QVBoxLayout;

  this->treeWidget = new QTreeWidget();
  this->treeWidget->setColumnCount(1);
  this->treeWidget->header()->hide();
  this->treeWidget->setFocusPolicy(Qt::NoFocus);
  this->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->treeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->treeWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  // connect(this->treeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
  //        this, SLOT(OnModelSelection(QTreeWidgetItem *, int)));


  QFrame *frame = new QFrame;
  QVBoxLayout *frameLayout = new QVBoxLayout;
  frameLayout->addWidget(this->treeWidget);
  frameLayout->setContentsMargins(0, 0, 0, 0);
  frame->setLayout(frameLayout);

  mainLayout->addWidget(frame);

  // Let the stylesheet handle the margin sizes
  mainLayout->setContentsMargins(0, 0, 0, 0);

  // Assign the mainlayout to this widget
  this->setLayout(mainLayout);

  this->GetTopicList();
}

/////////////////////////////////////////////////
TopicSelector::~TopicSelector()
{
}

/////////////////////////////////////////////////
void TopicSelector::GetTopicList()
{
  std::map<std::string, std::list<std::string> > topics;
  topics = transport::TopicManager::Instance()->GetAdvertisedTopics();

  std::map<std::string, bool> inserted;
  for (std::map<std::string, std::list<std::string> >::iterator
       iter = topics.begin(); iter != topics.end(); ++iter)
  {
    if (inserted.find(iter->first) == inserted.end() &&
        iter->first.find("__dbg") == std::string::npos)
    {
      std::cout << iter->first << "\n";
      inserted[iter->first] = true;
    }
  }
}
