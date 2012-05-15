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
/* Desc: Sphere collisionetry
 * Author: Nate Keonig
 * Date: 21 May 2009
 */

#ifndef __BULLETSPHERESHAPE_HH__
#define __BULLETSPHERESHAPE_HH__

#include "physics/bullet/BulletPhysics.hh"
#include "physics/SphereShape.hh"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{
    /// \addtogroup gazebo_physics_bullet ODE Physics
    /// \{
    /// \brief Bullet sphere collision
    class BulletSphereShape : public SphereShape
    {
      /// \brief Constructor
      public: BulletSphereShape(CollisionPtr _parent) : SphereShape(_parent) {}

      /// \brief Destructor
      public: virtual ~BulletSphereShape() {}

      /// \brief Set the radius
      public: void SetRadius(const double &_radius)
              {
                SphereShape::SetRadius(_radius);
                BulletCollisionPtr bParent;
                bParent = boost::shared_dynamic_cast<BulletCollision>(
                    this->collisionParent);

                bParent->SetCollisionShape(new btSphereShape(_radius));
              }
    };
    /// \}
    /// \}
  }
}
#endif
