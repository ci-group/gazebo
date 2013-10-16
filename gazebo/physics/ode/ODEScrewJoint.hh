/*
 * Copyright (C) 2012-1013 Open Source Robotics Foundation
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
/* Desc: A screw or primastic joint
 * Author: Nate Koenig, Andrew Howard
 * Date: 21 May 2003
 */

#ifndef _ODESCREWJOINT_HH_
#define _ODESCREWJOINT_HH_

#include "gazebo/physics/ScrewJoint.hh"
#include "gazebo/physics/ode/ODEJoint.hh"

namespace gazebo
{
  namespace physics
  {
    /// \brief A screw joint.
    class ODEScrewJoint : public ScrewJoint<ODEJoint>
    {
      /// \brief Constructor.
      /// \param[in] _worldId ODE world id.
      /// \param[in] _parent Pointer to the Link that is the joint' parent
      public: ODEScrewJoint(dWorldID _worldId, BasePtr _parent);

      /// \brief Destructor.
      public: virtual ~ODEScrewJoint();

      // Documentation inherited
      public: virtual void Load(sdf::ElementPtr _sdf);

      // Documentation inherited
      public: virtual math::Vector3 GetGlobalAxis(int _index) const;

      // Documentation inherited
      public: virtual void SetAxis(int index, const math::Vector3 &_axis);

      // Documentation inherited
      public: virtual void SetThreadPitch(int _index, double _threadPitch);

      // Documentation inherited
      public: virtual double GetThreadPitch(unsigned int _index);

      // Documentation inherited
      public: virtual math::Angle GetAngleImpl(int _index) const;

      // Documentation inherited
      public: virtual double GetVelocity(int _index) const;

      // Documentation inherited
      public: virtual void SetVelocity(int _index, double _angle);

      // Documentation inherited
      public: virtual void SetMaxForce(int _index, double _t);

      // Documentation inherited
      public: virtual double GetMaxForce(int _index);

      // Documentation inherited
      public: virtual double GetParam(int _parameter) const;

      // Documentation inherited
      public: virtual void SetParam(int _parameter, double _value);

      // Documentation inherited
      protected: virtual void SetForceImpl(int _index, double _effort);
    };
  }
}
#endif
