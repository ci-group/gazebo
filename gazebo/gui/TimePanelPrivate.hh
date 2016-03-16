/*
 * Copyright (C) 2015-2016 Open Source Robotics Foundation
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
#ifndef _GAZEBO_TIME_PANEL_PRIVATE_HH_
#define _GAZEBO_TIME_PANEL_PRIVATE_HH_

#include <list>
#include <vector>

#include "gazebo/gui/qt.h"

namespace gazebo
{
  namespace gui
  {
    /// \internal
    /// \brief Private data for the TimePanel class
    class TimePanelPrivate
    {
      /// \brief Node used for communication.
      public: transport::NodePtr node;

      /// \brief Subscriber to the statistics topic.
      public: transport::SubscriberPtr statsSub;

      /// \brief Publish user command messages for the server to place in the
      /// undo queue.
      public: transport::PublisherPtr userCmdPub;

      /// \brief Event based connections.
      public: std::vector<event::ConnectionPtr> connections;

      /// \brief List of simulation times used to compute averages.
      public: std::list<common::Time> simTimes;

      /// \brief List of real times used to compute averages.
      public: std::list<common::Time> realTimes;

      /// \brief Mutex to protect the memeber variables.
      public: boost::mutex mutex;

      /// \brief Paused state of the simulation.
      public: bool paused;

      /// \brief Paused state of the simulation.
      public: TimeWidget *timeWidget;

      /// \brief Paused state of the simulation.
      public: LogPlayWidget *logPlayWidget;
    };
  }
}
#endif
