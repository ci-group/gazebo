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
 * Author: Nate Koenig, Andrew Howard
 * Date: 21 May 2003
 */
#include <boost/bind.hpp>

#include "gazebo/gazebo_config.h"
#include "gazebo/common/Console.hh"

#include "gazebo/physics/Model.hh"
#include "gazebo/physics/Link.hh"
#include "gazebo/physics/ode/ODESliderJoint.hh"

using namespace gazebo;
using namespace physics;


//////////////////////////////////////////////////
ODESliderJoint::ODESliderJoint(dWorldID _worldId, BasePtr _parent)
    : SliderJoint<ODEJoint>(_parent)
{
  this->jointId = dJointCreateSlider(_worldId, NULL);
}

//////////////////////////////////////////////////
ODESliderJoint::~ODESliderJoint()
{
  if (this->applyDamping)
    physics::Joint::DisconnectJointUpdate(this->applyDamping);
}

//////////////////////////////////////////////////
void ODESliderJoint::Load(sdf::ElementPtr _sdf)
{
  SliderJoint<ODEJoint>::Load(_sdf);
}

//////////////////////////////////////////////////
math::Vector3 ODESliderJoint::GetGlobalAxis(int /*_index*/) const
{
  dVector3 result;
  if (this->jointId)
    dJointGetSliderAxis(this->jointId, result);
  else
    gzerr << "ODE Joint ID is invalid\n";

  return math::Vector3(result[0], result[1], result[2]);
}

//////////////////////////////////////////////////
math::Angle ODESliderJoint::GetAngleImpl(int /*_index*/) const
{
  math::Angle result;
  if (this->jointId)
    result = dJointGetSliderPosition(this->jointId);
  else
    gzerr << "ODE Joint ID is invalid\n";

  return result;
}

//////////////////////////////////////////////////
double ODESliderJoint::GetVelocity(int /*index*/) const
{
  double result = 0;
  if (this->jointId)
    result = dJointGetSliderPositionRate(this->jointId);
  else
    gzerr << "ODE Joint ID is invalid\n";

  return result;
}

//////////////////////////////////////////////////
void ODESliderJoint::SetVelocity(int /*index*/, double _angle)
{
  this->SetParam(dParamVel, _angle);
}

//////////////////////////////////////////////////
void ODESliderJoint::SetAxis(int /*index*/, const math::Vector3 &_axis)
{
  if (this->childLink)
    this->childLink->SetEnabled(true);
  if (this->parentLink) this->parentLink->SetEnabled(true);

  /// ODE needs global axis
  /// \TODO: currently we assume joint axis is specified in model frame,
  /// this is incorrect, and should be corrected to be
  /// joint frame which is specified in child link frame.
  math::Vector3 globalAxis = _axis;
  if (this->parentLink)
    globalAxis =
      this->GetParent()->GetModel()->GetWorldPose().rot.RotateVector(_axis);

  if (this->jointId)
    dJointSetSliderAxis(this->jointId, _axis.x, _axis.y, _axis.z);
  else
    gzerr << "ODE Joint ID is invalid\n";
}

//////////////////////////////////////////////////
void ODESliderJoint::SetForce(int _index, double _effort)
{
  if (_index < 0 || static_cast<unsigned int>(_index) >= this->GetAngleCount())
  {
    gzerr << "Calling ODEScrewJoint::SetForce with an index ["
          << _index << "] out of range\n";
    return;
  }

  // truncating SetForce effort if velocity limit reached.
  if (this->velocityLimit[_index] >= 0)
  {
    if (this->GetVelocity(_index) > this->velocityLimit[_index])
      _effort = _effort > 0 ? 0 : _effort;
    else if (this->GetVelocity(_index) < -this->velocityLimit[_index])
      _effort = _effort < 0 ? 0 : _effort;
  }

  // truncate effort if effortLimit is not negative
  if (this->effortLimit[_index] >= 0.0)
    _effort = math::clamp(_effort, -this->effortLimit[_index],
      this->effortLimit[_index]);

  ODEJoint::SetForce(_index, _effort);
  if (this->childLink)
    this->childLink->SetEnabled(true);
  if (this->parentLink)
    this->parentLink->SetEnabled(true);

  if (this->jointId)
    dJointAddSliderForce(this->jointId, _effort);
  else
    gzerr << "ODE Joint ID is invalid\n";
}

//////////////////////////////////////////////////
void ODESliderJoint::SetParam(int _parameter, double _value)
{
  ODEJoint::SetParam(_parameter, _value);
  dJointSetSliderParam(this->jointId, _parameter, _value);
}

//////////////////////////////////////////////////
double ODESliderJoint::GetParam(int _parameter) const
{
  double result = 0;

  if (this->jointId)
    result = dJointGetSliderParam(this->jointId, _parameter);
  else
    gzerr << "ODE Joint ID is invalid\n";

  return result;
}

//////////////////////////////////////////////////
void ODESliderJoint::SetMaxForce(int /*_index*/, double _t)
{
  this->SetParam(dParamFMax, _t);
}

//////////////////////////////////////////////////
double ODESliderJoint::GetMaxForce(int /*_index*/)
{
  return this->GetParam(dParamFMax);
}
