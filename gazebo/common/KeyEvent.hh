/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
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
#ifndef _KEYEVENT_HH_
#define _KEYEVENT_HH_

#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace common
  {
    /// \addtogroup gazebo_common
    /// \{

    /// \class KeyEvent KeyEvent.hh common/common.hh
    /// \brief Generic description of a keyboard event.
    class GAZEBO_VISIBLE KeyEvent
    {
      /// \brief Key event types enumeration.
      public: enum EventType {NO_EVENT, PRESS, RELEASE};

      /// \brief Constructor.
      public: KeyEvent()
              : type(NO_EVENT), key(0)
              {}

      /// \brief Event type.
      public: EventType type;

      public: int key;
    };
    /// \}
  }
}
#endif
