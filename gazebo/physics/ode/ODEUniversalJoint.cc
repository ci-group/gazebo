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
/* Desc: A universal joint
 * Author: Nate Koenig, Andrew Howard
 * Date: 21 May 2003
 */

#include "gazebo_config.h"
#include "common/Console.hh"

#include "physics/Link.hh"
#include "physics/ode/ODEUniversalJoint.hh"

using namespace gazebo;
using namespace physics;


//////////////////////////////////////////////////
ODEUniversalJoint::ODEUniversalJoint(dWorldID _worldId, BasePtr _parent)
    : UniversalJoint<ODEJoint>(_parent)
{
  this->jointId = dJointCreateUniversal(_worldId, NULL);
}

//////////////////////////////////////////////////
ODEUniversalJoint::~ODEUniversalJoint()
{
  if (this->applyDamping)
    physics::Joint::DisconnectJointUpdate(this->applyDamping);
}

//////////////////////////////////////////////////
math::Vector3 ODEUniversalJoint::GetAnchor(int /*index*/) const
{
  dVector3 result;
  dJointGetUniversalAnchor(this->jointId, result);

  return math::Vector3(result[0], result[1], result[2]);
}

//////////////////////////////////////////////////
void ODEUniversalJoint::SetAnchor(int /*index*/, const math::Vector3 &_anchor)
{
  if (this->childLink) this->childLink->SetEnabled(true);
  if (this->parentLink) this->parentLink->SetEnabled(true);

  dJointSetUniversalAnchor(this->jointId, _anchor.x, _anchor.y, _anchor.z);
}

//////////////////////////////////////////////////
math::Vector3 ODEUniversalJoint::GetGlobalAxis(int _index) const
{
  dVector3 result;

  if (_index == 0)
    dJointGetUniversalAxis1(this->jointId, result);
  else
    dJointGetUniversalAxis2(this->jointId, result);

  return math::Vector3(result[0], result[1], result[2]);
}

//////////////////////////////////////////////////
void ODEUniversalJoint::SetAxis(int _index, const math::Vector3 &_axis)
{
  if (this->childLink) this->childLink->SetEnabled(true);
  if (this->parentLink) this->parentLink->SetEnabled(true);

  if (_index == 0)
    dJointSetUniversalAxis1(this->jointId, _axis.x, _axis.y, _axis.z);
  else
    dJointSetUniversalAxis2(this->jointId, _axis.x, _axis.y, _axis.z);
}

//////////////////////////////////////////////////
math::Angle ODEUniversalJoint::GetAngleImpl(int _index) const
{
  math::Angle result;

  if (this->jointId)
  {
    if (_index == 0)
      result = dJointGetUniversalAngle1(this->jointId);
    else
      result = dJointGetUniversalAngle2(this->jointId);
  }

  return result;
}

//////////////////////////////////////////////////
double ODEUniversalJoint::GetVelocity(int _index) const
{
  double result;

  if (_index == 0)
    result = dJointGetUniversalAngle1Rate(this->jointId);
  else
    result = dJointGetUniversalAngle2Rate(this->jointId);

  return result;
}

//////////////////////////////////////////////////
void ODEUniversalJoint::SetVelocity(int _index, double _angle)
{
  if (_index == 0)
    this->SetParam(dParamVel, _angle);
  else
    this->SetParam(dParamVel2, _angle);
}

//////////////////////////////////////////////////
void ODEUniversalJoint::SetForce(int _index, double _effort)
{
  if (_index < 0 || static_cast<unsigned int>(_index) >= this->GetAngleCount())
  {
    gzerr << "Calling ODEUniversalJoint::SetForce with an index ["
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
  if (this->childLink) this->childLink->SetEnabled(true);
  if (this->parentLink) this->parentLink->SetEnabled(true);

  if (_index == 0)
    dJointAddUniversalTorques(this->jointId, _effort, 0);
  else
    dJointAddUniversalTorques(this->jointId, 0, _effort);
}

//////////////////////////////////////////////////
void ODEUniversalJoint::SetMaxForce(int _index, double _t)
{
  if (_index == 0)
    this->SetParam(dParamFMax, _t);
  else
    this->SetParam(dParamFMax2, _t);
}

//////////////////////////////////////////////////
double ODEUniversalJoint::GetMaxForce(int _index)
{
  if (_index == 0)
    return this->GetParam(dParamFMax);
  else
    return this->GetParam(dParamFMax2);
}

//////////////////////////////////////////////////
void ODEUniversalJoint::SetParam(int _parameter, double _value)
{
  ODEJoint::SetParam(_parameter, _value);
  dJointSetUniversalParam(this->jointId, _parameter, _value);
}







