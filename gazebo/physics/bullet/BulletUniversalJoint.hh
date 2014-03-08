/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
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
#ifndef _GAZEBO_BULLETUNIVERSALJOINT_HH_
#define _GAZEBO_BULLETUNIVERSALJOINT_HH_

#include "gazebo/physics/bullet/gzBtUniversalConstraint.hh"
#include "gazebo/physics/UniversalJoint.hh"
#include "gazebo/physics/bullet/BulletJoint.hh"
#include "gazebo/physics/bullet/BulletPhysics.hh"

class btUniversalConstraint;

namespace gazebo
{
  namespace physics
  {
    /// \ingroup gazebo_physics
    /// \addtogroup gazebo_physics_bullet Bullet Physics
    /// \{

    /// \brief A bullet universal joint class
    class BulletUniversalJoint : public UniversalJoint<BulletJoint>
    {
      /// \brief Constructor
      public: BulletUniversalJoint(btDynamicsWorld *world, BasePtr _parent);

      /// \brief Destuctor
      public: virtual ~BulletUniversalJoint();

      // Documentation inherited.
      public: virtual void Load(sdf::ElementPtr _sdf);

      // Documentation inherited.
      public: virtual void Init();

      /// \brief Get the anchor point
      public: virtual math::Vector3 GetAnchor(unsigned int _index) const;

      /// \brief Set the first axis of rotation
      public: void SetAxis(unsigned int _index, const math::Vector3 &_axis);

      /// \brief Set the velocity of an axis(index).
      public: virtual void SetVelocity(unsigned int _index, double _angle);

      /// \brief Get the angular rate of axis 1
      public: virtual double GetVelocity(unsigned int _index) const;

      /// \brief Set the max allowed force of an axis(index).
      public: virtual void SetMaxForce(unsigned int _index, double _t);

      /// \brief Get the max allowed force of an axis(index).
      public: virtual double GetMaxForce(unsigned int _index);

      /// \brief Set the high stop of an axis(index).
      public: virtual void SetHighStop(unsigned int _index,
                  const math::Angle &_angle);

      /// \brief Set the low stop of an axis(index).
      public: virtual void SetLowStop(unsigned int _index,
                  const math::Angle &_angle);

      /// \brief Get the high stop of an axis(index).
      public: virtual math::Angle GetHighStop(unsigned int _index);

      /// \brief Get the low stop of an axis(index).
      public: virtual math::Angle GetLowStop(unsigned int _index);

      /// \brief Get the axis of rotation
      public: virtual math::Vector3 GetGlobalAxis(unsigned int _index) const;

      /// \brief Get the angle of rotation
      public: virtual math::Angle GetAngleImpl(unsigned int _index) const;

      /// \brief Set the torque of a joint.
      protected: virtual void SetForceImpl(unsigned int _index, double _torque);

      /// \brief Pointer to bullet universal constraint
      private: gzBtUniversalConstraint *bulletUniversal;

      /// \brief Offset angle used in GetAngleImpl, so that angles are reported
      /// relative to the initial configuration.
      private: double angleOffset[2];

      /// \brief Initial value of joint axis, expressed as unit vector
      /// in world frame.
      private: math::Vector3 initialWorldAxis[2];
    };
    /// \}
  }
}
#endif
