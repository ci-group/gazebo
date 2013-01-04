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
#ifndef _IMAGEVIEW_HH_
#define _IMAGEVIEW_HH_

#include <string>

#include "gazebo/common/Time.hh"
#include "gazebo/msgs/msgs.hh"

#include "gazebo/transport/TransportTypes.hh"

#include "gazebo/gui/qt.h"
#include "gazebo/gui/viewers/TopicView.hh"

namespace gazebo
{
  namespace gui
  {
    class ImageView : public TopicView
    {
      Q_OBJECT

      /// \brief Constructor
      /// \param[in] _parent Pointer to the parent widget.
      public: ImageView(QWidget *_parent = NULL);

      /// \brief Destructor
      public: virtual ~ImageView();

      // Documentation inherited
      public: virtual void SetTopic(const std::string &_topicName);

      // Documentation inherited
      private: virtual void UpdateImpl();

      /// \brief Receives incoming image messages.
      /// \param[in] _msg New image message.
      private: void OnImage(ConstImageStampedPtr &_msg);

      /// \brief A label is used to display the image data.
      private: QLabel *imageLabel;

      /// \brief Storage mechansim for image data.
      private: QPixmap pixmap;
    };
  }
}
#endif
