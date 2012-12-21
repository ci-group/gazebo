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
#include "gazebo/transport/Transport.hh"
#include "gazebo/transport/Node.hh"
#include "gazebo/transport/Publisher.hh"

#include "gazebo/msgs/MsgFactory.hh"
#include "gazebo/gui/viewers/TextView.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
TextView::TextView(const std::string &_msgType)
  : TopicView(_msgType)
{
  this->setWindowTitle(tr("Gazebo: Text View"));

  this->bufferSize = 10;
  this->paused = false;

  // Create the text list
  // {
  QVBoxLayout *frameLayout = new QVBoxLayout;

  this->msgList = new QListWidget();

  QHBoxLayout *controlLayout = new QHBoxLayout;

  QSpinBox *bufferSpin = new QSpinBox();
  bufferSpin->setMinimum(1);
  bufferSpin->setValue(this->bufferSize);
  connect(bufferSpin, SIGNAL(valueChanged(int)), this, SLOT(OnBuffer(int)));

  controlLayout->addWidget(new QLabel(tr("Buffer: ")));
  controlLayout->addWidget(bufferSpin);

  QCheckBox *pauseCheck = new QCheckBox();
  pauseCheck->setChecked(this->paused);
  connect(pauseCheck, SIGNAL(clicked(bool)), this, SLOT(OnPause(bool)));

  controlLayout->addWidget(new QLabel(tr("Pause: ")));
  controlLayout->addWidget(pauseCheck);

  frameLayout->addWidget(this->msgList);
  frameLayout->addLayout(controlLayout);

  this->frame->setObjectName("blackBorderFrame");
  this->frame->setLayout(frameLayout);
  // }
}

/////////////////////////////////////////////////
TextView::~TextView()
{
}

/////////////////////////////////////////////////
void TextView::UpdateImpl()
{
  // Anything?
}

/////////////////////////////////////////////////
void TextView::SetTopic(const std::string &_topicName)
{
  TopicView::SetTopic(_topicName);

  this->msg = msgs::MsgFactory::NewMsg(this->msgTypeName);

  this->msgList->clear();

  // Subscribe to the new topic if we have generated an appropriate message
  if (this->msg)
  {
    this->sub.reset();
    this->sub = this->node->Subscribe(_topicName, &TextView::OnText, this);
  }
  else
  {
    this->msgList->addItem(new QListWidgetItem(QString::fromStdString(
          std::string("Unable to parse message of type[") +
          this->msgTypeName + "]")));
  }
}

/////////////////////////////////////////////////
void TextView::OnText(const std::string &_msg)
{
  if (this->paused)
    return;

  // Update the Hz and Bandwidth info.
  this->OnMsg(common::Time::GetWallTime(), _msg.size());

  // Convert the raw data to a message.
  msg->ParseFromString(_msg);

  {
    boost::mutex::scoped_lock lock(this->mutex);

    // Create a new list item.
    QListWidgetItem *item = new QListWidgetItem(
        QString::fromStdString(msg->DebugString()));

    // Add the new text to the output view.
    this->msgList->addItem(item);

    // Remove items if the list is too long.
    while (this->msgList->count() > this->bufferSize)
      delete this->msgList->takeItem(0);
  }
}

/////////////////////////////////////////////////
void TextView::OnPause(bool _value)
{
  boost::mutex::scoped_lock lock(this->mutex);
  this->paused = _value;
}

/////////////////////////////////////////////////
void TextView::OnBuffer(int _value)
{
  boost::mutex::scoped_lock lock(this->mutex);

  this->bufferSize = _value;

  // Remove and item if the list is too long.
  while (this->msgList->count() > this->bufferSize)
    delete this->msgList->takeItem(0);
}
