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
/* Desc: A hinge joint with 2 degrees of freedom
 * Author: Nate Koenig, Andrew Howard
 * Date: 21 May 2003
 */

#ifndef _SIMBODYHINGE2JOINT_HH_
#define _SIMBODYHINGE2JOINT_HH_

#include "math/Angle.hh"
#include "math/Vector3.hh"
#include "physics/Hinge2Joint.hh"
#include "physics/simbody/SimbodyJoint.hh"
#include "physics/simbody/SimbodyPhysics.hh"

namespace gazebo
{
  namespace physics
  {
    /// \ingroup gazebo_physics
    /// \addtogroup gazebo_physics_simbody Simbody Physics
    /// \{

    /// \brief A two axis hinge joint
    class SimbodyHinge2Joint : public Hinge2Joint<SimbodyJoint>
    {
      /// \brief Constructor
      public: SimbodyHinge2Joint(MultibodySystem *world, BasePtr _parent);

      /// \brief Destructor
      public: virtual ~SimbodyHinge2Joint();

      /// \brief Load the SimbodyHinge2Joint
      protected: virtual void Load(sdf::ElementPtr _sdf);

      /// \brief Attach the two bodies with this joint
      public: virtual void Attach(LinkPtr _one, LinkPtr _two);

      /// \brief Set the anchor point
      public: virtual void SetAnchor(int _index, const math::Vector3 &_anchor);

      /// \brief Get anchor point
      public: virtual math::Vector3 GetAnchor(int _index) const;

      /// \brief Set the first axis of rotation
      public: virtual void SetAxis(int _index, const math::Vector3 &_axis);

      /// \brief Set joint damping, not yet implemented
      public: virtual void SetDamping(int _index, double _damping);

      /// \brief Get first axis of rotation
      public: virtual math::Vector3 GetAxis(int _index) const;

      /// \brief Get angle of rotation about first axis
      public: math::Angle GetAngle(int _index) const;

      /// \brief Get rate of rotation about first axis
      public: double GetVelocity(int _index) const;

      /// \brief Set the velocity of an axis(index).
      public: virtual void SetVelocity(int _index, double _angle);

      /// \brief Set the torque
      public: void SetForce(int _index, double _torque);

      /// \brief Set the max allowed force of an axis(index).
      public: virtual void SetMaxForce(int _index, double _t);

      /// \brief Get the max allowed force of an axis(index).
      public: virtual double GetMaxForce(int _index);

      /// \brief Set the high stop of an axis(index).
      public: virtual void SetHighStop(int _index, const math::Angle &_angle);

      /// \brief Set the low stop of an axis(index).
      public: virtual void SetLowStop(int _index, const math::Angle &_angle);

      /// \brief Get the high stop of an axis(index).
      public: virtual math::Angle GetHighStop(int _index);

      /// \brief Get the low stop of an axis(index).
      public: virtual math::Angle GetLowStop(int _index);

      /// \brief Get the axis of rotation
      public: virtual math::Vector3 GetGlobalAxis(int _index) const;

      /// \brief Get the angle of rotation
      public: virtual math::Angle GetAngleImpl(int _index) const;
    };

  /// \}
  }
}
#endif
