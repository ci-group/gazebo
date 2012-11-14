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
#ifndef _ODEBOXSHAPE_HH_
#define _ODEBOXSHAPE_HH_

#include "math/Vector3.hh"

#include "physics/ode/ODEPhysics.hh"
#include "physics/ode/ODETypes.hh"
#include "physics/ode/ODECollision.hh"

#include "physics/PhysicsTypes.hh"
#include "physics/BoxShape.hh"

namespace gazebo
{
  namespace physics
  {
    /// \brief ODE Box shape
    class ODEBoxShape : public BoxShape
    {
      /// \brief Constructor.
      /// \param[in] _parent Parent Collision.
      public: explicit ODEBoxShape(ODECollisionPtr _parent)
              : BoxShape(_parent) {}

      /// \brief Destructor.
      public: virtual ~ODEBoxShape() {}

      // Documentation inherited.
      public: virtual void SetSize(const math::Vector3 &_size)
      {
        BoxShape::SetSize(_size);

        ODECollisionPtr oParent;
        oParent = boost::shared_dynamic_cast<ODECollision>(
            this->collisionParent);

        if (oParent->GetCollisionId() == NULL)
          oParent->SetCollision(dCreateBox(0, _size.x, _size.y, _size.z), true);
        else
          dGeomBoxSetLengths(oParent->GetCollisionId(),
                             _size.x, _size.y, _size.z);
      }
    };
  }
}
#endif
