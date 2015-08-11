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
#ifndef _GAZEBO_RENDERING_WIDE_ANGLE_CAMERA_CAMERA_PRIVATE_HH_
#define _GAZEBO_RENDERING_WIDE_ANGLE_CAMERA_CAMERA_PRIVATE_HH_

#include <deque>
#include <utility>
#include <list>

#include "gazebo/msgs/msgs.hh"
#include "gazebo/util/system.hh"

namespace Ogre
{
  class CompositorInstance;
}

namespace gazebo
{
  namespace rendering
  {
    /// \brief Private data for the WideAngleCamera class
    class GZ_RENDERING_VISIBLE WideAngleCameraPrivate
    {
      /// \brief Mutex to lock while rendering the world
      public: boost::mutex renderMutex;
    };
  }
}
#endif
