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
#ifndef _CAMERASENSORWIDGET_HH_
#define _CAMERASENSORWIDGET_HH_

#include "gazebo/gui/qt.h"
#include "gazebo/transport/TransportTypes.hh"

namespace gazebo
{
  namespace gui
  {
    class CameraSensorWidget : public QWidget
    {
      Q_OBJECT

      /// \brief Constructor
      public: CameraSensorWidget(QWidget *_parent = 0);

      /// \brief Destructor
      public: virtual ~CameraSensorWidget();

      private: transport::NodePtr node;
      private: transport::PublisherPtr pub;
      private: transport::SubscriberPtr sub;
    };
  }
}
#endif
