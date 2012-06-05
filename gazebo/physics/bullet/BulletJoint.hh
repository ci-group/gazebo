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
/* Desc: The base Bullet joint class
 * Author: Nate Keonig, Andrew Howard
 * Date: 21 May 2003
 */

#ifndef __BULLETJOINT_HH__
#define __BULLETJOINT_HH__

#include "physics/bullet/BulletPhysics.hh"
#include "physics/Joint.hh"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{
    /// \addtogroup gazebo_physics_ode
    /// \{
    /// \brief Base class for all joints
    class BulletJoint : public Joint
    {
      /// \brief Constructor
      public: BulletJoint();

      /// \brief Destructor
      public: virtual ~BulletJoint();

      /// \brief Load a joint
      public: void Load(sdf::ElementPtr _sdf);

      /// \brief Reset the joint
      public: virtual void Reset();

      /// \brief Get the body to which the joint is attached
      ///        according the _index
      public: LinkPtr GetJointLink(int _index) const;

      /// \brief Determines of the two bodies are connected by a joint
      public: bool AreConnected(LinkPtr _one, LinkPtr _two) const;

      /// \brief Detach this joint from all bodies
      public: virtual void Detach();

      /// \brief Set the anchor point
      public: virtual void SetAnchor(int /*index*/,
                                      const gazebo::math::Vector3 & /*anchor*/)
              {gzerr << "Not implement in Bullet\n";}

      /// \brief Set the joint damping
      public: virtual void SetDamping(int /*index*/,
                                      const double /*damping*/)
              {gzerr << "Not implement in Bullet\n";}

      /// \brief Get the anchor point
      public: virtual math::Vector3 GetAnchor(int /*_index*/) const
              {gzerr << "Not implement in Bullet\n";
               return math::Vector3();}

      /// \brief Get the force the joint applies to the first body
      /// \param index The index of the body(0 or 1)
      public: virtual math::Vector3 GetLinkForce(unsigned int /*_index*/) const
              {gzerr << "Not implement in Bullet\n";
               return math::Vector3();}

      /// \brief Get the torque the joint applies to the first body
      /// \param index The index of the body(0 or 1)
      public: virtual math::Vector3 GetLinkTorque(unsigned int /*_index*/) const
              {gzerr << "Not implement in Bullet\n";
               return math::Vector3();}

      /// \brief Set a parameter for the joint
      public: virtual void SetAttribute(Attribute, int /*_index*/,
                                        double /*_value*/)
              {gzerr << "Not implement in Bullet\n";}

      protected: btTypedConstraint *constraint;
      protected: btDynamicsWorld *world;
    };
    /// \}
    /// \}
  }
}
#endif
