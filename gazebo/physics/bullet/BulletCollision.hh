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
/* Desc: Collision class
 * Author: Nate Koenig
 * Date: 13 Feb 2006
 */

#ifndef __BULLET_COLLISION_HH__
#define __BULLET_COLLISION_HH__

#include <string>

/*

#include "common/Param.hh"
#include "Entity.hh"
#include "math/Pose.hh"
#include "math/Vector3.hh"
#include "physics/Collision.hh"
*/

#include "physics/PhysicsTypes.hh"
#include "physics/Collision.hh"

class btCollisionShape;

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics_ode
    /// \brief Base class for all Bullet collisions
    /// \{
    /// \brief Base class for all collisions
    class BulletCollision : public Collision
    {
      /// \brief Constructor
      public: BulletCollision(LinkPtr _parent);

      /// \brief Destructor
      public: virtual ~BulletCollision();

      /// \brief Load the collision
      public: virtual void Load(sdf::ElementPtr _ptr);

      /// \brief On pose change
      public: virtual void OnPoseChange();

      /// \brief Set the category bits, used during collision detection
      /// \param bits The bits
      public: virtual void SetCategoryBits(unsigned int bits);

      /// \brief Set the collide bits, used during collision detection
      /// \param bits The bits
      public: virtual void SetCollideBits(unsigned int bits);

      /// \brief Get the bounding box, defined by the physics engine
      public: virtual math::Box GetBoundingBox() const;

      /// \brief Set the collision shape
      public: void SetCollisionShape(btCollisionShape *shape);

      /// \brief Get the bullet collision shape
      public: btCollisionShape *GetCollisionShape() const;

      /// \brief Set the index of the compound shape
      public: void SetCompoundShapeIndex(int index);

      protected: btCollisionShape *collisionShape;
    };
    /// \}
  }
}
#endif
