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
/* Desc: A universal joint
 * Author: Nate Keonig
 * Date: 24 May 2009
 */

#ifndef __BULLETUNIVERSALJOINT_HH__
#define __BULLETUNIVERSALJOINT_HH__

#include "physics/UniversalJoint.hh"
#include "physics/bullet/BulletJoint.hh"
#include "physics/bullet/BulletPhysics.hh"

class btUniversalConstraint;

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{
    /// \addtogroup gazebo_physics_bullet Bullet Physics
    /// \{
    /// \brief A universal joint
    class BulletUniversalJoint : public UniversalJoint<BulletJoint>
    {
      /// \brief Constructor
      public: BulletUniversalJoint(btDynamicsWorld *world);

      /// \brief Destuctor
      public: virtual ~BulletUniversalJoint();

      /// \brief Attach the two bodies with this joint
      public: void Attach(LinkPtr _one, LinkPtr _two);

      /// \brief Get the anchor point
      public: virtual math::Vector3 GetAnchor(int _index) const;

      /// \brief Set the anchor point
      public: virtual void SetAnchor(int _index, const math::Vector3 &_anchor);

      /// \brief Set the first axis of rotation
      public: void SetAxis(int _index, const math::Vector3 &_axis);

      /// \brief Set joint damping, not yet implemented
      public: virtual void SetDamping(int _index, const double _damping);

      /// \brief Get the first axis of rotation
      public: virtual math::Vector3 GetAxis(int _index) const;

      /// \brief Get the angle of axis 1
      public: virtual math::Angle GetAngle(int _index) const;

      /// \brief Set the velocity of an axis(index).
      public: virtual void SetVelocity(int _index, double _angle);

      /// \brief Get the angular rate of axis 1
      public: virtual double GetVelocity(int _index) const;

      /// \brief Set the torque of a joint.
      public: virtual void SetForce(int _index, double _torque);

      /// \brief Set the max allowed force of an axis(index).
      public: virtual void SetMaxForce(int _index, double _t);

      /// \brief Get the max allowed force of an axis(index).
      public: virtual double GetMaxForce(int _index);

      /// \brief Set the high stop of an axis(index).
      public: virtual void SetHighStop(int _index, math::Angle _angle);

      /// \brief Set the low stop of an axis(index).
      public: virtual void SetLowStop(int _index, math::Angle _angle);

      /// \brief Get the high stop of an axis(index).
      public: virtual math::Angle GetHighStop(int _index);

      /// \brief Get the low stop of an axis(index).
      public: virtual math::Angle GetLowStop(int _index);

      /// \brief Get the axis of rotation
      public: virtual math::Vector3 GetGlobalAxis(int _index) const;

      /// \brief Get the angle of rotation
      public: virtual math::Angle GetAngleImpl(int _index) const;

      private: btUniversalConstraint *btUniversal;
    };

    /// \}
    /// \}
  }
}
#endif
