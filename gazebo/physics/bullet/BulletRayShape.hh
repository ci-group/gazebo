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
/* Desc: A ray
 * Author: Nate Keonig
 * Date: 24 May 2009
 */

#ifndef __BULLETRAYGEOM_HH__
#define __BULLETRAYGEOM_HH__

#include <string>
#include "physics/RayShape.hh"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{
    /// \addtogroup gazebo_physics_bullet Bullet Physics
    /// \{
    /// \brief Ray shape
    class BulletRayShape : public RayShape
    {
      public: BulletRayShape(PhysicsEnginePtr _physicsEngine);

      /// \brief Constructor
      /// \param body Link the ray is attached to
      public: BulletRayShape(CollisionPtr _collision);

      /// \brief Destructor
      public: virtual ~BulletRayShape();

      /// \brief Update the ray collision
      public: virtual void Update();

      /// \brief Get the nearest intersection
      public: virtual void GetIntersection(double &_dist, std::string &_entity);

      /// \brief Set the ray based on starting and ending points relative to
      ///        the body
      /// \param posStart Start position, relative the body
      /// \param posEnd End position, relative to the body
      public: void SetPoints(const math::Vector3 &_posStart,
                             const math::Vector3 &_posEnd);

      private: BulletPhysicsPtr physicsEngine;
      private: btCollisionWorld::ClosestRayResultCallback rayCallback;
    };
    /// \}
    /// \}
  }
}
#endif
