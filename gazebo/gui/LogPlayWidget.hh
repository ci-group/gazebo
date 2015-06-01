/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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
#ifndef _GAZEBO_LOG_PLAY_WIDGET_HH_
#define _GAZEBO_LOG_PLAY_WIDGET_HH_

#include "gazebo/common/Time.hh"
#include "gazebo/gui/qt.h"
#include "gazebo/gui/TimePanel.hh"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace gui
  {
    class LogPlayWidgetPrivate;
    class LogPlayViewPrivate;
    class TimePanel;

    /// \class LogPlayWidget LogPlayWidget.hh
    /// \brief Widget which displays log playback options.
    class GAZEBO_VISIBLE LogPlayWidget : public QWidget
    {
      Q_OBJECT

      /// \brief Constructor
      /// \param[in] _parent Parent widget, commonly a TimePanel.
      public: LogPlayWidget(QWidget *_parent = 0);

      /// \brief Destructor
      public: virtual ~LogPlayWidget();

      /// \brief Returns if the simulation is displayed as paused.
      /// \return True if paused, false otherwise.
      public: bool IsPaused() const;

      /// \brief Set whether to display the simulation as paused.
      /// \param[in] _p True to display the simulation as paused. False
      /// indicates the simulation is running
      public: void SetPaused(const bool _paused);

      /// \brief Emit signal to set current time.
      /// \param[in] _time Current time.
      public: void EmitSetCurrentTime(common::Time _time);

      /// \brief Emit signal to set start time.
      /// \param[in] _time Start time.
      public: void EmitSetStartTime(common::Time _time);

      /// \brief Emit signal to set end time.
      /// \param[in] _time End time.
      public: void EmitSetEndTime(common::Time _time);

      /// \brief Play simulation.
      public slots: void OnPlay();

      /// \brief Pause simulation.
      public slots: void OnPause();

      /// \brief Step simulation forward.
      public slots: void OnStepForward();

      /// \brief Qt signal to show the play button.
      signals: void ShowPlay();

      /// \brief Qt signal to hide the play button.
      signals: void HidePlay();

      /// \brief Qt signal to show the pause button.
      signals: void ShowPause();

      /// \brief Qt signal to hide the pause button.
      signals: void HidePause();

      /// \brief Qt signal used to set the current time line edit.
      /// \param[in] _string String representation of current time.
      signals: void SetCurrentTime(const QString &);

      /// \brief Qt signal used to set the end time line edit.
      /// \param[in] _string String representation of current time.
      signals: void SetEndTime(const QString &);

      /// \brief Qt signal used to set the current time in the view.
      /// \param[in] _time Time in ms.
      signals: void SetCurrentTime(int _time);

      /// \brief Qt signal used to set the start time in the view.
      /// \param[in] _time Time in ms.
      signals: void SetStartTime(int _time);

      /// \brief Qt signal used to set the end time in the view.
      /// \param[in] _time Eime in ms.
      signals: void SetEndTime(int _time);

      /// \internal
      /// \brief Pointer to private data.
      private: LogPlayWidgetPrivate *dataPtr;
    };

    /// \class LogPlayView LogPlayView.hh
    /// \brief View for the timeline.
    class GAZEBO_VISIBLE LogPlayView: public QGraphicsView
    {
      Q_OBJECT

      /// \brief Constructor;
      /// \param[in] _parent Parent widget.
      public: LogPlayView(LogPlayWidget *_parent = 0);

      /// \brief Set the position of the current time item.
      /// \param[in] _msec Absolute time in ms.
      public slots: void SetCurrentTime(int _msec);

      /// \brief Set the log start time.
      /// \param[in] _msec Start time in ms.
      public slots: void SetStartTime(int _msec);

      /// \brief Set the log end time.
      /// \param[in] _msec End time position in ms.
      public slots: void SetEndTime(int _msec);

      /// \brief Draw the timeline.
      public slots: void DrawTimeline();

      /// \internal
      /// \brief Pointer to private data.
      private: LogPlayViewPrivate *dataPtr;
    };

    /// \class CurrentTimeItem CurrentTimeItem.hh
    /// \brief Item which represents the current time within the view.
    class GAZEBO_VISIBLE CurrentTimeItem: public QObject,
        public QGraphicsRectItem
    {
      Q_OBJECT

      /// \brief Constructor;
      public: CurrentTimeItem();

      // Documentation inherited
      private: virtual void paint(QPainter *_painter,
          const QStyleOptionGraphicsItem *_option, QWidget *_widget);
    };
  }
}

#endif
