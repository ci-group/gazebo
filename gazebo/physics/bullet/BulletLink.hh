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
/* Desc: Bullet Link class
 * Author: Nate Koenig
 * Date: 15 May 2009
 */

#ifndef _BULLETLINK_HH_
#define _BULLETLINK_HH_

#include "physics/bullet/bullet_inc.h"
#include "physics/bullet/BulletTypes.hh"
#include "physics/Link.hh"

class btRigidBody;

namespace gazebo
{
  namespace physics
  {
    class BulletMotionState;

    /// \ingroup gazebo_physics
    /// \addtogroup gazebo_physics_bullet Bullet Physics
    /// \brief bullet physics engine wrapper
    /// \{

    /// \brief Bullet Link class
    class BulletLink : public Link
    {
      /// \brief Constructor
      public: BulletLink(EntityPtr _parent);

      /// \brief Destructor
      public: virtual ~BulletLink();

      // Documentation inherited.
      public: virtual void Load(sdf::ElementPtr _ptr);

      // Documentation inherited.
      public: virtual void Init();

      // Documentation inherited.
      public: virtual void Fini();

      // Documentation inherited.
      public: virtual void Update();

      // Documentation inherited.
      public: virtual void OnPoseChange();

      // Documentation inherited.
      public: virtual void SetEnabled(bool enable) const;

      // Documentation inherited.
      public: virtual bool GetEnabled() const {return true;}

      // Documentation inherited.
      public: virtual void SetLinearVel(const math::Vector3 &vel);

      // Documentation inherited.
      public: virtual void SetAngularVel(const math::Vector3 &vel);

      // Documentation inherited.
      public: virtual void SetForce(const math::Vector3 &force);

      // Documentation inherited.
      public: virtual void SetTorque(const math::Vector3 &force);

      // Documentation inherited.
      public: virtual math::Vector3 GetWorldLinearVel() const;

      // Documentation inherited.
      public: virtual math::Vector3 GetWorldAngularVel() const;

      // Documentation inherited.
      public: virtual math::Vector3 GetWorldForce() const;

      // Documentation inherited.
      public: virtual math::Vector3 GetWorldTorque() const;

      // Documentation inherited.
      public: virtual void SetGravityMode(bool mode);

      // Documentation inherited.
      public: virtual bool GetGravityMode() const;

      // Documentation inherited.
      public: virtual void SetSelfCollide(bool collide);

      /// \brief Get the bullet rigid body.
      /// \return Pointer to bullet rigid body object.
      public: btRigidBody *GetBulletLink() const;

      // Documentation inherited.
      public: virtual void SetLinearDamping(double damping);

      // Documentation inherited.
      public: virtual void SetAngularDamping(double damping);

      /// \brief Set the relative pose of a child collision.
      /*public: void SetCollisionRelativePose(BulletCollision *collision,
                                            const math::Pose &newPose);
                                            */

      // Documentation inherited.
      public: virtual void AddForce(const math::Vector3 &_force);

      // Documentation inherited.
      public: virtual void AddRelativeForce(const math::Vector3 &_force);

      // Documentation inherited.
      public: virtual void AddForceAtWorldPosition(const math::Vector3 &_force,
                                                   const math::Vector3 &_pos);

      // Documentation inherited.
      public: virtual void AddForceAtRelativePosition(
                  const math::Vector3 &_force,
                  const math::Vector3 &_relpos);

      // Documentation inherited.
      public: virtual void AddTorque(const math::Vector3 &_torque);

      // Documentation inherited.
      public: virtual void AddRelativeTorque(const math::Vector3 &_torque);

      // Documentation inherited.
      public: virtual void SetAutoDisable(bool _disable);

      /// \brief Pointer to bullet compound shape, which is a container
      ///        for other child shapes.
      private: btCompoundShape *compoundShape;

      /// \brief Pointer to bullet motion state, which manages updates to the
      ///        world pose from bullet.
      private: BulletMotionStatePtr motionState;

      /// \brief Pointer to the bullet rigid body object.
      private: btRigidBody *rigidLink;

      /// \brief Pointer to the bullet physics engine.
      private: BulletPhysicsPtr bulletPhysics;
    };
    /// \}
  }
}
#endif
