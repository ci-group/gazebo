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
/* Desc: The base Simbody joint class
 * Author: Nate Koenig, Andrew Howard
 * Date: 21 May 2003
 */

#ifndef _SIMBODYJOINT_HH_
#define _SIMBODYJOINT_HH_

#include <boost/any.hpp>
#include <string>

#include <Simbody.h>

#include "physics/simbody/SimbodyPhysics.hh"
#include "physics/Joint.hh"

namespace gazebo
{
  namespace physics
  {
    /// \ingroup gazebo_physics
    /// \addtogroup gazebo_physics_simbody Simbody Physics
    /// \{

    /// \brief Base class for all joints
    class SimbodyJoint : public Joint
    {
      /// \brief Constructor
      public: SimbodyJoint(BasePtr _parent);

      /// \brief Destructor
      public: virtual ~SimbodyJoint();

      /// \brief Load a SimbodyJoint
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
              {gzerr << "Not implement in Simbody\n";}

      /// \brief Set the joint damping
      public: virtual void SetDamping(int /*index*/,
                                      const double /*damping*/)
              {gzerr << "Not implement in Simbody\n";}

      /// \brief Get the anchor point
      public: virtual math::Vector3 GetAnchor(int /*_index*/) const
              {gzerr << "Not implement in Simbody\n";
               return math::Vector3();}

      /// \brief Get the force the joint applies to the first body
      /// \param index The index of the body(0 or 1)
      public: virtual math::Vector3 GetLinkForce(unsigned int /*_index*/) const
              {gzerr << "Not implement in Simbody\n";
               return math::Vector3();}

      /// \brief Get the torque the joint applies to the first body
      /// \param index The index of the body(0 or 1)
      public: virtual math::Vector3 GetLinkTorque(unsigned int /*_index*/) const
              {gzerr << "Not implement in Simbody\n";
               return math::Vector3();}

      /// \brief Set a parameter for the joint
      public: virtual void SetAttribute(Attribute, int /*_index*/,
                                        double /*_value*/)
              {gzerr << "Not implement in Simbody\n";}

      // Documentation inherited.
      public: virtual void SetAttribute(const std::string &/*_key*/,
                                        int /*_index*/,
                                        const boost::any &/*_value*/)
              {gzerr << "Not implement in Simbody\n";}

      protected: SimTK::MultibodySystem *world;

      // Documentation inherited.
      public: virtual JointWrench GetForceTorque(int _index);

      // Simbody specific variables
      public: bool mustBreakLoopHere;

      // Normally A=F, B=M. But if reversed, then B=F, A=M.
      public: SimTK::Transform    X_PA; // parent body frame to mobilizer frame
      public: SimTK::Transform    X_CB; // child body frame to mobilizer frame
      public: SimTK::Transform defX_AB; // default mobilizer pose

      // Members below here are set when we build the Simbody model.

      // How this joint was modeled in the Simbody System. We used either a
      // mobilizer or a constraint, but not both. The type of either one is the
      // same as the joint type above.

      // isValid() if we used a mobilizer
      public: SimTK::MobilizedBody mobod;

      /// \brief: if mobilizer, did it reverse parent&child?
      public: bool isReversed;

      /// \brief: isValid() if we used a constraint
      public: SimTK::Constraint constraint;


    };
    /// \}
  }
}
#endif
