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
/* Desc: Box shape
 * Author: Nate Keonig
 * Date: 14 Oct 2009
 */

#ifndef __BULLETBOXSHAPE_HH__
#define __BULLETBOXSHAPE_HH__

#include "physics/bullet/BulletPhysics.hh"
#include "physics/BoxShape.hh"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{
    /// \addtogroup gazebo_physics_bullet ODE Physics
    /// \{
    /// \brief Bullet box collision
    class BulletBoxShape : public BoxShape
    {
      /// \brief Constructor
      public: BulletBoxShape(CollisionPtr _parent) : BoxShape(_parent) {}

      /// \brief Destructor
      public: virtual ~BulletBoxShape() {}

      /// \brief Set the size of the box
      public: void SetSize(const math::Vector3 &_size)
              {
                BoxShape::SetSize(_size);
                BulletCollisionPtr bParent;
                bParent = boost::shared_dynamic_cast<BulletCollision>(
                    this->collisionParent);

                /// Bullet requires the half-extents of the box
                bParent->SetCollisionShape(new btBoxShape(
                    btVector3(_size.x*0.5, _size.y*0.5, _size.z*0.5)));
              }
    };
    /// \}
    /// \}
  }
}
#endif
