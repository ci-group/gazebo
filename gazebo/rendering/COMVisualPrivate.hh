/*
 * Copyright (C) 2012-2016 Open Source Robotics Foundation
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

#ifndef _COMVISUAL_PRIVATE_HH_
#define _COMVISUAL_PRIVATE_HH_

#include <string>

#include "gazebo/rendering/VisualPrivate.hh"

namespace ogre
{
  class SceneNode;
}

namespace gazebo
{
  namespace rendering
  {
    class DynamicLines;

    /// \brief Private data for the COM Visual class
    class COMVisualPrivate : public VisualPrivate
    {
      /// \brief Lines that make the cross marking the center of mass.
      public: DynamicLines *crossLines;

      /// \brief Sphere with density of lead and equivalent mass.
      public: Ogre::SceneNode *sphereNode;

      /// \brief Inertia pose in link frame.
      public: math::Pose inertiaPose;

      /// \brief Parent link name.
      public: std::string linkName;

      /// \brief Link mass.
      public: double mass;
    };
  }
}
#endif
