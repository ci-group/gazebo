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
/* Desc: A hinge joint with 2 degrees of freedom
 * Author: Nate Keonig, Andrew Howard
 * Date: 21 May 2003
 */

#ifndef __BULLETHINGE2JOINT_HH__
#define __BULLETHINGE2JOINT_HH__

#include "math/Angle.hh"
#include "math/Vector3.hh"
#include "physics/Hinge2Joint.hh"
#include "physics/bullet/BulletJoint.hh"
#include "physics/bullet/BulletPhysics.hh"

class btHinge2Constraint;

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{
    /// \addtogroup gazebo_physics_bullet Bullet Physics
    /// \{
    /// \brief A two axis hinge joint
    class BulletHinge2Joint : public Hinge2Joint<BulletJoint>
    {
      /// \brief Constructor
      public: BulletHinge2Joint(btDynamicsWorld *world);

      /// \brief Destructor
      public: virtual ~BulletHinge2Joint();

      /// \brief Load the joint
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

      private: btHinge2Constraint *btHinge2;
    };

  /// \}
  /// \}
  }
}
#endif
