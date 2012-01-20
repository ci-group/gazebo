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
 * SVN: $Id: Collision.hh 7640 2009-05-13 02:06:08Z natepak $
 */

#ifndef BulletGEOM_HH
#define BulletGEOM_HH

#include <string>

#include "common/Param.hh"
#include "Entity.hh"
#include "math/Pose.hh"
#include "math/Vector3.hh"
#include "physics/Collision.hh"

class btCollisionShape;

namespace gazebo
{
  namespace physics
  {
    class Link;
    class XMLConfigNode;
    class BulletPhysics;

    /// \addtogroup gazebo_physics_ode
    /// \brief Base class for all Bullet collisions
    /// \{
    /// \brief Base class for all collisions
    class BulletCollision : public Collision
    {
      /// \brief Constructor
      // public: Collision(Link *body, const std::string &name);
      public: BulletCollision(Link *body);

      /// \brief Destructor
      public: virtual ~BulletCollision();

      /// \brief Load the collision
      public: virtual void Load(common::XMLConfigNode *node);

      /// \brief Load the collision
      public: virtual void Save(std::string &prefix, std::ostream &stream);

      /// \brief Update function for collisions
      public: virtual void Update();

      /// \brief On pose change
      public: virtual void OnPoseChange();

      /// \brief Set the category bits, used during collision detection
      /// \param bits The bits
      public: virtual void SetCategoryBits(unsigned int bits);

      /// \brief Set the collide bits, used during collision detection
      /// \param bits The bits
      public: virtual void SetCollideBits(unsigned int bits);

      /// \brief Get the mass of the collision
      public: Mass GetLinkMassMatrix();

      /// \brief Get the bounding box, defined by the physics engine
      public: virtual void GetBoundingBox(math::Vector3 &min,
                                          math::Vector3 &max) const;

      /// \brief Set the collision shape
      public: void SetCollisionShape(btCollisionShape *shape);

      /// \brief Get the bullet collision shape
      public: btCollisionShape *GetCollisionShape() const;

      /// \brief Set the index of the compound shape
      public: void SetCompoundShapeIndex(int index);

      protected: BulletPhysics *bulletPhysics;
      protected: btCollisionShape *collisionShape;

      protected: int compoundShapeIndex;
    };
    /// \}
  }
}
#endif
