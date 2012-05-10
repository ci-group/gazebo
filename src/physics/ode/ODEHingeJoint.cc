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
/* Desc: A ODEHingeJoint
 * Author: Nate Keonig, Andrew Howard
 * Date: 21 May 2003
 */

#include <boost/bind.hpp>

#include "gazebo_config.h"
#include "common/Console.hh"

#include "physics/Link.hh"
#include "physics/ode/ODEHingeJoint.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
ODEHingeJoint::ODEHingeJoint(dWorldID _worldId)
    : HingeJoint<ODEJoint>()
{
  this->jointId = dJointCreateHinge(_worldId, NULL);
}

//////////////////////////////////////////////////
ODEHingeJoint::~ODEHingeJoint()
{
}

//////////////////////////////////////////////////
void ODEHingeJoint::Load(sdf::ElementPtr _sdf)
{
  HingeJoint<ODEJoint>::Load(_sdf);

  this->SetParam(dParamFMax, 0);
  this->SetForce(0, 0);
}

//////////////////////////////////////////////////
math::Vector3 ODEHingeJoint::GetAnchor(int /*index*/) const
{
  dVector3 result;

  dJointGetHingeAnchor(this->jointId, result);

  return math::Vector3(result[0], result[1], result[2]);
}

//////////////////////////////////////////////////
void ODEHingeJoint::SetAnchor(int /*index*/, const math::Vector3 &_anchor)
{
  if (this->childLink)
    this->childLink->SetEnabled(true);
  if (this->parentLink)
    this->parentLink->SetEnabled(true);

  dJointSetHingeAnchor(this->jointId, _anchor.x, _anchor.y, _anchor.z);
}


//////////////////////////////////////////////////
math::Vector3 ODEHingeJoint::GetGlobalAxis(int /*_index*/) const
{
    dVector3 result;
    dJointGetHingeAxis(this->jointId, result);
    return math::Vector3(result[0], result[1], result[2]);
}

//////////////////////////////////////////////////
void ODEHingeJoint::SetAxis(int /*index*/, const math::Vector3 &_axis)
{
  if (this->childLink)
    this->childLink->SetEnabled(true);
  if (this->parentLink)
    this->parentLink->SetEnabled(true);

  dJointSetHingeAxis(this->jointId, _axis.x, _axis.y, _axis.z);
}

//////////////////////////////////////////////////
void ODEHingeJoint::SetDamping(int /*index*/, const double _damping)
{
  this->damping_coefficient = _damping;
  dJointSetDamping(this->jointId, this->damping_coefficient);
}

//////////////////////////////////////////////////
void ODEHingeJoint::ApplyDamping()
{
  double damping_force = this->damping_coefficient * this->GetVelocity(0);
  this->SetForce(0, damping_force);
}

//////////////////////////////////////////////////
math::Angle ODEHingeJoint::GetAngleImpl(int /*index*/) const
{
  math::Angle result = dJointGetHingeAngle(this->jointId);

  return result;
}

//////////////////////////////////////////////////
double ODEHingeJoint::GetVelocity(int /*index*/) const
{
  double result = dJointGetHingeAngleRate(this->jointId);

  return result;
}

//////////////////////////////////////////////////
void ODEHingeJoint::SetVelocity(int /*index*/, double _angle)
{
  this->SetParam(dParamVel, _angle);
}

//////////////////////////////////////////////////
void ODEHingeJoint::SetMaxForce(int /*index*/, double _t)
{
  return this->SetParam(dParamFMax, _t);
}

//////////////////////////////////////////////////
double ODEHingeJoint::GetMaxForce(int /*index*/)
{
  return this->GetParam(dParamFMax);
}

//////////////////////////////////////////////////
void ODEHingeJoint::SetForce(int /*index*/, double _torque)
{
  if (this->childLink)
    this->childLink->SetEnabled(true);
  if (this->parentLink)
    this->parentLink->SetEnabled(true);
  dJointAddHingeTorque(this->jointId, _torque);
}

//////////////////////////////////////////////////
double ODEHingeJoint::GetParam(int _parameter) const
{
  double result = dJointGetHingeParam(this->jointId, _parameter);

  return result;
}

//////////////////////////////////////////////////
void ODEHingeJoint::SetParam(int _parameter, double _value)
{
  ODEJoint::SetParam(_parameter, _value);

  dJointSetHingeParam(this->jointId, _parameter, _value);
}
