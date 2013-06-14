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
#include <sstream>

#include "transport/Node.hh"

#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/GuiEvents.hh"
#include "gazebo/gui/TimePanel.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
TimePanel::TimePanel(QWidget *_parent)
  : QWidget(_parent)
{
  this->setObjectName("timePanel");

  QHBoxLayout *mainLayout = new QHBoxLayout;

  QFrame *frame = new QFrame;
  QHBoxLayout *frameLayout = new QHBoxLayout;

  this->percentRealTimeEdit = new QLineEdit;
  this->percentRealTimeEdit->setObjectName("timePanelPercentRealTime");
  this->percentRealTimeEdit->setReadOnly(true);
  this->percentRealTimeEdit->setFixedWidth(90);

  this->simTimeEdit = new QLineEdit;
  this->simTimeEdit->setObjectName("timePanelSimTime");
  this->simTimeEdit->setReadOnly(true);
  this->simTimeEdit->setFixedWidth(110);

  this->realTimeEdit = new QLineEdit;
  this->realTimeEdit->setObjectName("timePanelRealTime");
  this->realTimeEdit->setReadOnly(true);
  this->realTimeEdit->setFixedWidth(110);

  this->iterationsEdit = new QLineEdit;
  this->iterationsEdit->setReadOnly(true);
  this->iterationsEdit->setFixedWidth(110);

  QPushButton *timeResetButton = new QPushButton("Reset");
  timeResetButton->setFocusPolicy(Qt::NoFocus);
  connect(timeResetButton, SIGNAL(clicked()),
          this, SLOT(OnTimeReset()));

  frameLayout->addWidget(new QLabel(tr("Real Time Factor:")));
  frameLayout->addWidget(this->percentRealTimeEdit);

  frameLayout->addWidget(new QLabel(tr("Sim Time:")));
  frameLayout->addWidget(this->simTimeEdit);

  frameLayout->addWidget(new QLabel(tr("Real Time:")));
  frameLayout->addWidget(this->realTimeEdit);

  frameLayout->addWidget(new QLabel(tr("Iterations:")));
  frameLayout->addWidget(this->iterationsEdit);

  frameLayout->addWidget(timeResetButton);

  frame->setLayout(frameLayout);
  frame->layout()->setContentsMargins(0, 0, 0, 0);

  mainLayout->addWidget(frame);
  this->setLayout(mainLayout);

  this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  this->layout()->setContentsMargins(0, 0, 0, 0);

  this->node = transport::NodePtr(new transport::Node());
  this->node->Init();

  this->statsSub =
    this->node->Subscribe("~/world_stats", &TimePanel::OnStats, this);
  this->worldControlPub =
    this->node->Advertise<msgs::WorldControl>("~/world_control");

  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(Update()));
  timer->start(33);

  this->connections.push_back(
      gui::Events::ConnectFullScreen(
        boost::bind(&TimePanel::OnFullScreen, this, _1)));

  this->show();

  // Create a QueuedConnection to set iterations.
  // This is used for thread safety.
  connect(this, SIGNAL(SetIterations(QString)),
          this->iterationsEdit, SLOT(setText(QString)), Qt::QueuedConnection);

  // Create a QueuedConnection to set sim time.
  // This is used for thread safety.
  connect(this, SIGNAL(SetSimTime(QString)),
          this->simTimeEdit, SLOT(setText(QString)), Qt::QueuedConnection);

  // Create a QueuedConnection to set real time.
  // This is used for thread safety.
  connect(this, SIGNAL(SetRealTime(QString)),
          this->realTimeEdit, SLOT(setText(QString)), Qt::QueuedConnection);
}

/////////////////////////////////////////////////
void TimePanel::OnFullScreen(bool & /*_value*/)
{
  /*if (_value)
    this->hide();
  else
    this->show();
    */
}

/////////////////////////////////////////////////
TimePanel::~TimePanel()
{
  this->node.reset();
}

/////////////////////////////////////////////////
void TimePanel::OnStats(ConstWorldStatisticsPtr &_msg)
{
  boost::mutex::scoped_lock lock(this->mutex);
  std::ostringstream stream;

  this->simTimes.push_back(msgs::Convert(_msg->sim_time()));
  if (this->simTimes.size() > 20)
    this->simTimes.pop_front();

  this->realTimes.push_back(msgs::Convert(_msg->real_time()));
  if (this->realTimes.size() > 20)
    this->realTimes.pop_front();

  if (_msg->paused() && (g_playAct && !g_playAct->isVisible()))
  {
    g_playAct->setVisible(true);
    g_pauseAct->setVisible(false);
  }
  else if (!_msg->paused() && (g_pauseAct && !g_pauseAct->isVisible()))
  {
    g_pauseAct->setVisible(true);
    g_playAct->setVisible(false);
  }

  unsigned int day, hour, min, sec, msec;

  // Set simulation time
  {
    stream.str("");

    sec = _msg->sim_time().sec();

    day = sec / 86400;
    sec -= day * 86400;

    hour = sec / 3600;
    sec -= hour * 3600;

    min = sec / 60;
    sec -= min * 60;

    msec = rint(_msg->sim_time().nsec() * 1e-6);

    stream << std::setw(2) << std::setfill('0') << day << " ";
    stream << std::setw(2) << std::setfill('0') << hour << ":";
    stream << std::setw(2) << std::setfill('0') << min << ":";
    stream << std::setw(2) << std::setfill('0') << sec << ".";
    stream << std::setw(3) << std::setfill('0') << msec;

    this->SetSimTime(QString::fromStdString(stream.str()));
  }

  // Set real time
  {
    stream.str("");

    sec = _msg->real_time().sec();

    day = sec / 86400;
    sec -= day * 86400;

    hour = sec / 3600;
    sec -= hour * 3600;

    min = sec / 60;
    sec -= min * 60;

    msec = rint(_msg->sim_time().nsec() * 1e-6);

    stream << std::setw(2) << std::setfill('0') << day << " ";
    stream << std::setw(2) << std::setfill('0') << hour << ":";
    stream << std::setw(2) << std::setfill('0') << min << ":";
    stream << std::setw(2) << std::setfill('0') << sec << ".";
    stream << std::setw(3) << std::setfill('0') << msec;

    this->SetRealTime(QString::fromStdString(stream.str()));
  }

  // Set the iterations
  this->SetIterations(QString::fromStdString(
        boost::lexical_cast<std::string>(_msg->iterations())));
}

/////////////////////////////////////////////////
void TimePanel::Update()
{
  boost::mutex::scoped_lock lock(this->mutex);

  std::ostringstream percent;

  common::Time simAvg, realAvg;
  std::list<common::Time>::iterator simIter, realIter;

  simIter = ++(this->simTimes.begin());
  realIter = ++(this->realTimes.begin());
  while (simIter != this->simTimes.end() && realIter != this->realTimes.end())
  {
    simAvg += ((*simIter) - this->simTimes.front());
    realAvg += ((*realIter) - this->realTimes.front());
    ++simIter;
    ++realIter;
  }
  if (realAvg == 0)
    simAvg = 0;
  else
    simAvg = simAvg / realAvg;

  if (simAvg > 0)
    percent << std::fixed << std::setprecision(2) << simAvg.Double();
  else
    percent << "0";

  this->percentRealTimeEdit->setText(tr(percent.str().c_str()));
}

/////////////////////////////////////////////////
void TimePanel::OnTimeReset()
{
  msgs::WorldControl msg;
  msg.mutable_reset()->set_all(false);
  msg.mutable_reset()->set_time_only(true);
  this->worldControlPub->Publish(msg);
}
