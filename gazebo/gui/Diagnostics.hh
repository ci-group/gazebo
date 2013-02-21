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

#ifndef _DIAGNOSTICS_WIDGET_HH_
#define _DIAGNOSTICS_WIDGET_HH_

#include <map>
#include <boost/thread/mutex.hpp>

#include "gazebo/msgs/msgs.hh"
#include "gazebo/common/Time.hh"
#include "gazebo/transport/TransportTypes.hh"
#include "gazebo/gui/qt.h"

namespace gazebo
{
  namespace gui
  {
    class IncrementalPlot;

    /// \brief Plot diagnostic information
    class Diagnostics : public QDialog
    {
      Q_OBJECT

      /// \brief Constructor.
      /// \param[in] _parent Pointer to the parent widget.
      public: Diagnostics(QWidget *_parent);

      /// \brief Destructor.
      public: virtual ~Diagnostics();

      /// \brief Called when a diagnostic message is received.
      /// \param[in] _msg Diagnostic message.
      private: void OnMsg(ConstDiagnosticsPtr &_msg);

      /// \brief Update plots.
      private slots: void Update();

      /// \brief QT callback for the pause check button.
      /// \param[in] _value True when paused.
      private slots: void OnPause(bool _value);

      /// \brief Node for communications.
      private: transport::NodePtr node;

      /// \brief Subscribes to diagnostic info.
      private: transport::SubscriberPtr sub;

      /// \brief Start time of plotting.
      private: common::Time startTime;

      /// \brief The list of diagnostic labels.
      private: QListWidget *labelList;

      private: typedef std::map<QString, std::list<QPointF> > PointMap;

      /// \brief The currently selected label.
      private: PointMap selectedLabels;

      /// \brief True when plotting is paused.
      private: bool paused;

      /// \brief Mutex to protect the point map
      private: boost::mutex mutex;

      /// \brief Plotting widget
      private: IncrementalPlot *plot;
    };
  }
}
#endif
