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
#ifndef _GAZEBO_SPLASH_SCREEN_HH_
#define _GAZEBO_SPLASH_SCREEN_HH_

#include "gazebo/gui/qt.h"
#include "gazebo/gui/TimePanel.hh"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace gui
  {
    class SplashScreenPrivate;

    class GAZEBO_VISIBLE SplashScreen : public QObject
    {
      Q_OBJECT

      /// \brief Constructor
      /// \param[in] _parent Parent widget.
      public: SplashScreen();

      /// \brief Destructor
      public: virtual ~SplashScreen();

      /// \brief Show message in splash screen
      public: virtual void ShowMessage(const std::string &_message);

      /// \brief Update splash screen
      public slots: void Update();

      /// \internal
      /// \brief Pointer to private data.
      private: SplashScreenPrivate *dataPtr;
    };
  }
}

#endif
