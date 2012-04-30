/*
 * Copyright 2011 Nate Koenig & Andrew Howard
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
#ifndef __ODECYLINDERSHAPE_HH__
#define __ODECYLINDERSHAPE_HH__

#include "physics/CylinderShape.hh"
#include "physics/ode/ODEPhysics.hh"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{
    /// \addtogroup gazebo_physics_ode ODE Physics
    /// \{
    /// \brief ODE cylinder shape
    class ODECylinderShape : public CylinderShape
    {
      public: ODECylinderShape(CollisionPtr _parent) : CylinderShape(_parent) {}
      public: virtual ~ODECylinderShape() {}
      public: void SetSize(const double &_radius, const double &_length)
      {
        CylinderShape::SetSize(_radius, _length);
        ODECollisionPtr oParent;
        oParent =
          boost::shared_dynamic_cast<ODECollision>(this->collisionParent);

        if (oParent->GetCollisionId() == NULL)
          oParent->SetCollision(dCreateCylinder(0, _radius, _length), true);
        else
          dGeomCylinderSetParams(oParent->GetCollisionId(), _radius, _length);
      }
    };

    /// \}
    /// \}
  }
}
#endif






