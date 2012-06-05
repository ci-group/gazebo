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
/* Desc: Cylinder shape
 * Author: Nate Keonig
 * Date: 14 Oct 2009
 */

#ifndef __BULLETCYLINDERSHAPE_HH__
#define __BULLETCYLINDERSHAPE_HH__

#include "physics/bullet/BulletPhysics.hh"
#include "physics/CylinderShape.hh"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{
    /// \addtogroup gazebo_physics_bullet ODE Physics
    /// \{

    /// \brief Cylinder collision
    class BulletCylinderShape : public CylinderShape
    {
      /// \brief Constructor
      public: BulletCylinderShape(CollisionPtr _parent)
              : CylinderShape(_parent) {}

      /// \brief Destructor
      public: virtual ~BulletCylinderShape() {}

      /// \brief Set the size of the cylinder
      public: void SetSize(const double &_radius, const double &_length)
              {
                CylinderShape::SetSize(_radius, _length);
                BulletCollisionPtr bParent;
                bParent = boost::shared_dynamic_cast<BulletCollision>(
                    this->collisionParent);

                bParent->SetCollisionShape(new btCylinderShapeZ(
                    btVector3(_radius, _radius, _length * 0.5)));
              }
    };
    /// \}
    /// \}
  }
}
#endif
