/*
 * Copyright (C) 2012-2015 Open Source Robotics Foundation
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

#include "gazebo/transport/Node.hh"

#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/GuiEvents.hh"
#include "gazebo/gui/GuiIface.hh"
#include "gazebo/rendering/UserCamera.hh"
#include "gazebo/gui/TimeWidget.hh"
#include "gazebo/gui/LogPlayWidget.hh"
#include "gazebo/gui/TimePanel.hh"
#include "gazebo/gui/TimePanelPrivate.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
TimePanel::TimePanel(QWidget *_parent)
  : QWidget(_parent), dataPtr(new TimePanelPrivate)
{
  this->setObjectName("timePanel");

  // Time Widget
  this->dataPtr->timeWidget = new TimeWidget(this);
  connect(this, SIGNAL(SetTimeWidgetVisible(bool)),
      this->dataPtr->timeWidget, SLOT(setVisible(bool)));

  // LogPlay Widget
  this->dataPtr->logPlayWidget = new LogPlayWidget(this);
  connect(this, SIGNAL(SetLogPlayWidgetVisible(bool)),
      this->dataPtr->logPlayWidget, SLOT(setVisible(bool)));

  // Layout
  QHBoxLayout *mainLayout = new QHBoxLayout;
  mainLayout->addWidget(this->dataPtr->timeWidget);
  mainLayout->addWidget(this->dataPtr->logPlayWidget);
  this->setLayout(mainLayout);

  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  this->layout()->setContentsMargins(0, 0, 0, 0);

  // Transport
  this->dataPtr->node = transport::NodePtr(new transport::Node());
  this->dataPtr->node->Init();

  this->dataPtr->statsSub = this->dataPtr->node->Subscribe(
      "~/world_stats", &TimePanel::OnStats, this);

  this->dataPtr->worldControlPub = this->dataPtr->node->
      Advertise<msgs::WorldControl>("~/world_control");

  // Timer
  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(Update()));
  timer->start(33);

  // Connections
  this->dataPtr->connections.push_back(
      gui::Events::ConnectFullScreen(
      boost::bind(&TimePanel::OnFullScreen, this, _1)));

  connect(g_playAct, SIGNAL(changed()), this, SLOT(OnPlayActionChanged()));
  this->OnPlayActionChanged();
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
  this->dataPtr->node.reset();

  delete this->dataPtr;
  this->dataPtr = NULL;
}

/////////////////////////////////////////////////
void TimePanel::OnPlayActionChanged()
{
  // Tests don't see external actions
  if (!g_stepAct)
    return;

  if (this->IsPaused())
  {
    g_stepAct->setToolTip("Step the world");
    g_stepAct->setEnabled(true);
  }
  else
  {
    g_stepAct->setToolTip("Pause the world before stepping");
    g_stepAct->setEnabled(false);
  }
}

/////////////////////////////////////////////////
void TimePanel::ShowRealTimeFactor(bool _show)
{
  if (this->dataPtr->timeWidget->isVisible())
    this->dataPtr->timeWidget->ShowRealTimeFactor(_show);
  else
    gzwarn << "Time widget not visible" << std::endl;
}

/////////////////////////////////////////////////
void TimePanel::ShowRealTime(bool _show)
{
  if (this->dataPtr->timeWidget->isVisible())
    this->dataPtr->timeWidget->ShowRealTime(_show);
  else
    gzwarn << "Time widget not visible" << std::endl;
}

/////////////////////////////////////////////////
void TimePanel::ShowSimTime(bool _show)
{
  if (this->dataPtr->timeWidget->isVisible())
    this->dataPtr->timeWidget->ShowSimTime(_show);
  else
    gzwarn << "Time widget not visible" << std::endl;
}

/////////////////////////////////////////////////
void TimePanel::ShowIterations(bool _show)
{
  if (this->dataPtr->timeWidget->isVisible())
    this->dataPtr->timeWidget->ShowIterations(_show);
  else
    gzwarn << "Time widget not visible" << std::endl;
}

/////////////////////////////////////////////////
void TimePanel::ShowFPS(bool _show)
{
  if (this->dataPtr->timeWidget->isVisible())
    this->dataPtr->timeWidget->ShowFPS(_show);
  else
    gzwarn << "Time widget not visible" << std::endl;
}

/////////////////////////////////////////////////
void TimePanel::ShowStepWidget(bool _show)
{
  if (this->dataPtr->timeWidget->isVisible())
    this->dataPtr->timeWidget->ShowStepWidget(_show);
  else
    gzwarn << "Time widget not visible" << std::endl;
}

/////////////////////////////////////////////////
bool TimePanel::IsPaused() const
{
  return this->dataPtr->paused;
}

/////////////////////////////////////////////////
void TimePanel::SetPaused(bool _paused)
{
  this->dataPtr->paused = _paused;

  if (this->dataPtr->timeWidget->isVisible())
    this->dataPtr->timeWidget->SetPaused(_paused);
  else if (this->dataPtr->logPlayWidget->isVisible())
    this->dataPtr->logPlayWidget->SetPaused(_paused);
}

/////////////////////////////////////////////////
void TimePanel::OnStats(ConstWorldStatisticsPtr &_msg)
{
  boost::mutex::scoped_lock lock(this->dataPtr->mutex);

  this->dataPtr->simTimes.push_back(msgs::Convert(_msg->sim_time()));
  if (this->dataPtr->simTimes.size() > 20)
    this->dataPtr->simTimes.pop_front();

  this->dataPtr->realTimes.push_back(msgs::Convert(_msg->real_time()));
  if (this->dataPtr->realTimes.size() > 20)
    this->dataPtr->realTimes.pop_front();

  if (_msg->has_log_playback() && _msg->log_playback())
  {
    this->SetTimeWidgetVisible(false);
    this->SetLogPlayWidgetVisible(true);
  }
  else
  {
    this->SetTimeWidgetVisible(true);
    this->SetLogPlayWidgetVisible(false);
  }

  if (_msg->has_paused())
    this->SetPaused(_msg->paused());

  if (this->dataPtr->timeWidget->isVisible())
  {
    // Set simulation time
    this->dataPtr->timeWidget->EmitSetSimTime(
        QString::fromStdString(FormatTime(_msg->sim_time())));

    // Set real time
    this->dataPtr->timeWidget->EmitSetRealTime(
        QString::fromStdString(FormatTime(_msg->real_time())));

    // Set the iterations
    this->dataPtr->timeWidget->EmitSetIterations(QString::fromStdString(
        boost::lexical_cast<std::string>(_msg->iterations())));
  }
  else if (this->dataPtr->logPlayWidget->isVisible())
  {
    // Set simulation time
    this->dataPtr->logPlayWidget->EmitSetCurrentTime(
        QString::fromStdString(FormatTime(_msg->sim_time())));
  }
}

/////////////////////////////////////////////////
std::string TimePanel::FormatTime(const msgs::Time &_msg)
{
  std::ostringstream stream;
  unsigned int day, hour, min, sec, msec;

  stream.str("");

  sec = _msg.sec();

  day = sec / 86400;
  sec -= day * 86400;

  hour = sec / 3600;
  sec -= hour * 3600;

  min = sec / 60;
  sec -= min * 60;

  msec = rint(_msg.nsec() * 1e-6);

  stream << std::setw(2) << std::setfill('0') << day << " ";
  stream << std::setw(2) << std::setfill('0') << hour << ":";
  stream << std::setw(2) << std::setfill('0') << min << ":";
  stream << std::setw(2) << std::setfill('0') << sec << ".";
  stream << std::setw(3) << std::setfill('0') << msec;

  return stream.str();
}


/////////////////////////////////////////////////
void TimePanel::Update()
{
  boost::mutex::scoped_lock lock(this->dataPtr->mutex);

  std::ostringstream percent;

  common::Time simAvg, realAvg;
  std::list<common::Time>::iterator simIter, realIter;

  simIter = ++(this->dataPtr->simTimes.begin());
  realIter = ++(this->dataPtr->realTimes.begin());
  while (simIter != this->dataPtr->simTimes.end() &&
      realIter != this->dataPtr->realTimes.end())
  {
    simAvg += ((*simIter) - this->dataPtr->simTimes.front());
    realAvg += ((*realIter) - this->dataPtr->realTimes.front());
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

  if (this->dataPtr->timeWidget->isVisible())
    this->dataPtr->timeWidget->SetPercentRealTimeEdit(percent.str().c_str());

  rendering::UserCameraPtr cam = gui::get_active_camera();
  if (cam)
  {
    std::ostringstream avgFPS;
    avgFPS << cam->GetAvgFPS();

    if (this->dataPtr->timeWidget->isVisible())
    {
      // Set the avg fps
      this->dataPtr->timeWidget->EmitSetFPS(QString::fromStdString(
          boost::lexical_cast<std::string>(avgFPS.str().c_str())));
    }
  }
}

/////////////////////////////////////////////////
void TimePanel::OnTimeReset()
{
  msgs::WorldControl msg;
  msg.mutable_reset()->set_all(false);
  msg.mutable_reset()->set_time_only(true);
  this->dataPtr->worldControlPub->Publish(msg);
}

/////////////////////////////////////////////////
void TimePanel::OnStepValueChanged(int _value)
{
  emit gui::Events::inputStepSize(_value);
}
