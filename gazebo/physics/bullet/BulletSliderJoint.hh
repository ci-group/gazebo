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
#ifndef _BULLETSLIDERJOINT_HH_
#define _BULLETSLIDERJOINT_HH_
#include "gazebo/math/Angle.hh"
#include "gazebo/math/Vector3.hh"
#include "gazebo/physics/bullet/BulletJoint.hh"
#include "gazebo/physics/SliderJoint.hh"
#include "gazebo/physics/bullet/BulletPhysics.hh"
#include "gazebo/util/system.hh"

class btSliderConstraint;

namespace gazebo
{
  namespace physics
  {
    /// \ingroup gazebo_physics
    /// \addtogroup gazebo_physics_bullet Bullet Physics
    /// \{

    /// \brief A slider joint
    class GAZEBO_VISIBLE BulletSliderJoint : public SliderJoint<BulletJoint>
    {
      /// \brief Constructor
      public: BulletSliderJoint(btDynamicsWorld *world, BasePtr _parent);

      /// \brief Destructor
      public: virtual ~BulletSliderJoint();

      /// \brief Load the BulletSliderJoint
      protected: virtual void Load(sdf::ElementPtr _sdf);

      // Documentation inherited.
      protected: virtual void Init();

      // Documentation inherited.
      public: virtual void SetAxis(unsigned int _index,
                  const math::Vector3 &_axis);

      // Documentation inherited.
      public: virtual void SetDamping(unsigned int _index,
                  const double _damping);

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

      /// \brief Get the rate of change
      public: virtual double GetVelocity(unsigned int _index) const;

       /// \brief Set the velocity of an axis(index).
      public: virtual void SetVelocity(unsigned int _index, double _angle);

      /// \brief Set the max allowed force of an axis(index).
      public: virtual void SetMaxForce(unsigned int _index, double _force);

      /// \brief Get the max allowed force of an axis(index).
      public: virtual double GetMaxForce(unsigned int _index);

      /// \brief Get the axis of rotation
      public: virtual math::Vector3 GetGlobalAxis(unsigned int _index) const;

      /// \brief Get the angle of rotation
      public: virtual math::Angle GetAngleImpl(unsigned int _index) const;

      /// \brief Set the slider force
      protected: virtual void SetForceImpl(unsigned int _index, double _effort);

      /// \brief Pointer to bullet slider constraint
      private: btSliderConstraint *bulletSlider;

      /// \brief Initial value of joint axis, expressed as unit vector
      ///        in world frame.
      private: math::Vector3 initialWorldAxis;
    };

  /// \}
  }
}
#endif
