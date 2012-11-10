/*
 * Copyright 2011 Nate Koenig
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
#ifndef _SHAPE_HH_
#define _SHAPE_HH_

#include <string>

#include "gazebo/msgs/msgs.hh"

#include "gazebo/common/CommonTypes.hh"

#include "gazebo/physics/PhysicsTypes.hh"
#include "gazebo/physics/Inertial.hh"
#include "gazebo/physics/Base.hh"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{

    /// \class Shape Shape.hh physics/physics.hh
    /// \brief Base class for all shapes.
    class Shape : public Base
    {
      /// \brief Constructor.
      /// \param[in] _parent Parent of the shape.
      public: explicit Shape(CollisionPtr _parent);

      /// \brief Destructor.
      public: virtual ~Shape();

      /// \brief Initialize the shape.
      public: virtual void Init() = 0;

      /// \brief Deprecated.
      public: virtual double GetMass(double _density) const GAZEBO_DEPRECATED
              {return _density;}

      /// \brief Deprecated
      public: virtual void GetInertial(double _mass, InertialPtr _inertial)
              const GAZEBO_DEPRECATED;

      /// \brief Deprecated
      public: virtual void FillShapeMsg(msgs::Geometry &_msg)
              GAZEBO_DEPRECATED;

      /// \brief Fill in the values for a geometry message.
      /// \param[out] _msg The geometry message to fill.
      public: virtual void FillMsg(msgs::Geometry &_msg) = 0;

      /// \brief Process a geometry message.
      /// \param[in] _msg The message to set values from.
      public: virtual void ProcessMsg(const msgs::Geometry &_msg) = 0;

      /// \brief This shape's collision parent.
      protected: CollisionPtr collisionParent;
    };
    /// \}
  }
}
#endif
