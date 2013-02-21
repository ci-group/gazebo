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

#ifndef _DIAGNOSTICS_HH_
#define _DIAGNOSTICS_HH_

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
      public: Diagnostics(QWidget *_parent);

      /// \brief Destructor.
      public: virtual ~Diagnostics();

      /// \brief Signal used to add a point to the plot.
      signals: void AddPoint(const QPointF &_point);

      /// \brief Called when a diagnostic message is received.
      /// \param[in] _msg Diagnostic message.
      private: void OnMsg(ConstDiagnosticsPtr &_msg);

      /// \brief QT callback when a diagnostic label is selected.
      /// \param[in] _item The selected item.
      private slots: void OnLabelSelected(QListWidgetItem *_item);

      /// \brief QT callback for the pause check button.
      /// \param[in] _value True when paused.
      private slots: void OnPause(bool _value);

      /// \brief The plot.
      private: IncrementalPlot *plot;

      /// \brief Node for communications.
      private: transport::NodePtr node;

      /// \brief Subscribes to diagnostic info.
      private: transport::SubscriberPtr sub;

      /// \brief Start time of plotting.
      private: common::Time startTime;

      /// \brief The list of diagnostic labels.
      private: QListWidget *labelList;

      /// \brief The currently selected label.
      private: std::string selectedLabel;

      /// \brief True when plotting is paused.
      private: bool paused;
    };
  }
}
#endif
