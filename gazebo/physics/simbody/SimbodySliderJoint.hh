/*
 * Copyright 2012 Open Source Robotics Foundation
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
/* Desc: A slider or primastic joint
 * Author: Nate Koenig
 * Date: 24 May 2009
 */

#ifndef _SIMBODYSLIDERJOINT_HH_
#define _SIMBODYSLIDERJOINT_HH_
#include "math/Angle.hh"
#include "math/Vector3.hh"
#include "physics/simbody/SimbodyJoint.hh"
#include "physics/SliderJoint.hh"
#include "physics/simbody/SimbodyPhysics.hh"

namespace gazebo
{
  namespace physics
  {
    /// \ingroup gazebo_physics
    /// \addtogroup gazebo_physics_simbody Simbody Physics
    /// \{

    /// \brief A slider joint
    class SimbodySliderJoint : public SliderJoint<SimbodyJoint>
    {
      /// \brief Constructor
      public: SimbodySliderJoint(MultibodySystem *world, BasePtr _parent);

      /// \brief Destructor
      public: virtual ~SimbodySliderJoint();

      /// \brief Load the SimbodySliderJoint
      protected: virtual void Load(sdf::ElementPtr _sdf);

      /// \brief Attach the two bodies with this joint
      public: void Attach(LinkPtr _one, LinkPtr _two);

      /// \brief Set the axis of motion
      public: void SetAxis(int _index, const math::Vector3 &_axis);

      /// \brief Set joint damping, not yet implemented
      public: virtual void SetDamping(int _index, const double _damping);

      /// \brief Set the high stop of an axis(index).
      public: virtual void SetHighStop(int _index, const math::Angle &_angle);

      /// \brief Set the low stop of an axis(index).
      public: virtual void SetLowStop(int _index, const math::Angle &_angle);

      /// \brief Get the high stop of an axis(index).
      public: virtual math::Angle GetHighStop(int _index);

      /// \brief Get the low stop of an axis(index).
      public: virtual math::Angle GetLowStop(int _index);

      /// \brief Get the position of the joint
      public: virtual math::Angle GetAngle(int _index) const;

      /// \brief Get the rate of change
      public: virtual double GetVelocity(int _index) const;

       /// \brief Set the velocity of an axis(index).
      public: virtual void SetVelocity(int _index, double _angle);

      /// \brief Set the slider force
      public: virtual void SetForce(int _index, double _force);

      /// \brief Set the max allowed force of an axis(index).
      public: virtual void SetMaxForce(int _index, double _t);

      /// \brief Get the max allowed force of an axis(index).
      public: virtual double GetMaxForce(int _index);

      /// \brief Get the axis of rotation
      public: virtual math::Vector3 GetGlobalAxis(int _index) const;

      /// \brief Get the angle of rotation
      public: virtual math::Angle GetAngleImpl(int _index) const;
    };

  /// \}
  }
}
#endif

