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
#ifndef _GAZEBO_CYLINDERMAKER_HH_
#define _GAZEBO_CYLINDERMAKER_HH_

#include <string>

#include "gazebo/gui/EntityMaker.hh"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace msgs
  {
    class Visual;
  }
  namespace gui
  {
    /// \addtogroup gazebo_gui
    /// \{

    /// \class CylinderMaker CylinderMaker.hh
    /// \brief to make a cylinder
    class GAZEBO_VISIBLE CylinderMaker : public EntityMaker
    {
      /// \brief Constructor
      public: CylinderMaker();

      /// \brief Deconstructor
      public: virtual ~CylinderMaker();

      /// Documentation inherited
      public: virtual void Start(const rendering::UserCameraPtr _camera);

      /// Documentation inherited
      public: virtual void Stop();

      /// Documentation inherited
      public: virtual bool IsActive() const;

      /// \brief Get the SDF information for the cylinder.
      /// \return The SDF as a string.
      public: std::string GetSDFString();

      /// Documentation inherited
      private: virtual void CreateTheEntity();

      private: int state;
      private: msgs::Visual *visualMsg;

      private: static unsigned int counter;
    };
  }
}

#endif


