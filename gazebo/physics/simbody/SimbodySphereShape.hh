/*
 * Copyright 2013 Open Source Robotics Foundation
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
 * Author: Nate Koenig
 * Date: 21 May 2009
 */

#ifndef _SIMBODY_SPHERESHAPE_HH_
#define _SIMBODY_SPHERESHAPE_HH_

#include "gazebo/physics/simbody/SimbodyPhysics.hh"
#include "gazebo/physics/SphereShape.hh"

namespace gazebo
{
  namespace physics
  {
    /// \ingroup gazebo_physics
    /// \addtogroup gazebo_physics_simbody Simbody Physics
    /// \{

    /// \brief Simbody sphere collision
    class SimbodySphereShape : public SphereShape
    {
      /// \brief Constructor
      public: SimbodySphereShape(CollisionPtr _parent) : SphereShape(_parent) {}

      /// \brief Destructor
      public: virtual ~SimbodySphereShape() {}

      /// \brief Set the radius
      /// \param[in] _radius Sphere radius
      public: void SetRadius(double _radius)
              {
                if (_radius < 0)
                {
                  gzerr << "Sphere shape does not support negative radius\n";
                  return;
                }
                if (math::equal(_radius, 0.0))
                {
                  // Warn user, but still create shape with very small value
                  // otherwise later resize operations using setLocalScaling
                  // will not be possible
                  gzwarn << "Setting sphere shape's radius to zero \n";
                  _radius = 1e-4;
                }

                SphereShape::SetRadius(_radius);
                SimbodyCollisionPtr bParent;
                bParent = boost::dynamic_pointer_cast<SimbodyCollision>(
                    this->collisionParent);

                // set collision shape
              }
    };
    /// \}
  }
}
#endif
