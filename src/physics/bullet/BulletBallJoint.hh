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
/* Desc: A ball joint
 * Author: Nate Keonig, Andrew Howard
 * Date: 21 May 2003
 */

#ifndef __BULLETBALLJOINT_HH__
#define __BULLETBALLJOINT_HH__

#include "physics/BallJoint.hh"
#include "physics/bullet/BulletJoint.hh"
#include "physics/bullet/BulletPhysics.hh"

class btPoint2PointConstraint;

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{
    /// \addtogroup gazebo_physics_bullet Bullet Physics
    /// \{
    /// \brief A ball joint
    class BulletBallJoint : public BallJoint<BulletJoint>
    {
      /// \brief Constructor
      public: BulletBallJoint(btDynamicsWorld *_world);

      /// \brief Destructor
      public: virtual ~BulletBallJoint();

      /// \brief Get joint's anchor point
      public: math::Vector3 GetAnchor(int _index) const;

      /// \brief Set joint's anchor point
      public: void SetAnchor(int _index, const math::Vector3 &_anchor);

      /// \brief Set joint damping, not yet implemented
      public: virtual void SetDamping(int _index, double _damping);

      /// \brief Attach the two bodies with this joint
      public: void Attach(LinkPtr _one, LinkPtr _two);

      /// \brief Get the axis of rotation
      public: virtual math::Vector3 GetAxis(int /*_index*/) const
              {return math::Vector3();}

      /// \brief Set the velocity of an axis(index).
      public: virtual void SetVelocity(int _index, double _angle);

      /// \brief Get the rotation rate of an axis(index)
      public: virtual double GetVelocity(int _index) const;

      /// \brief Get the max allowed force of an axis(index).
      public: virtual double GetMaxForce(int _index);

      /// \brief Set the max allowed force of an axis(index).
      public: virtual void SetMaxForce(int _index, double _t);

      /// \brief Get the angle of rotation of an axis(index)
      public: virtual math::Angle GetAngle(int _index) const;

      /// \brief Get the axis of rotation
      public: virtual math::Vector3 GetGlobalAxis(int _index) const;

      /// \brief Get the angle of rotation
      public: virtual math::Angle GetAngleImpl(int _index) const;

      private: btPoint2PointConstraint *btBall;
    };
    /// \}
    /// \}
  }
}
#endif
