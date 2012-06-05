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

#include "gazebo_config.h"
#include "common/Console.hh"

#include "physics/Link.hh"
#include "physics/ode/ODEHinge2Joint.hh"

using namespace gazebo;
using namespace physics;


//////////////////////////////////////////////////
ODEHinge2Joint::ODEHinge2Joint(dWorldID _worldId)
    : Hinge2Joint<ODEJoint>()
{
  this->jointId = dJointCreateHinge2(_worldId, NULL);
}

//////////////////////////////////////////////////
ODEHinge2Joint::~ODEHinge2Joint()
{
}

//////////////////////////////////////////////////
void ODEHinge2Joint::Load(sdf::ElementPtr _sdf)
{
  Hinge2Joint<ODEJoint>::Load(_sdf);
}

//////////////////////////////////////////////////
math::Vector3 ODEHinge2Joint::GetAnchor(int _index) const
{
  dVector3 result;

  if (_index == 0)
    dJointGetHinge2Anchor(this->jointId, result);
  else
    dJointGetHinge2Anchor2(this->jointId, result);

  return math::Vector3(result[0], result[1], result[2]);
}

//////////////////////////////////////////////////
void ODEHinge2Joint::SetAnchor(int /*index*/, const math::Vector3 &_anchor)
{
  if (this->childLink) this->childLink->SetEnabled(true);
  if (this->parentLink) this->parentLink->SetEnabled(true);
  dJointSetHinge2Anchor(this->jointId, _anchor.x, _anchor.y, _anchor.z);
}

//////////////////////////////////////////////////
void ODEHinge2Joint::SetAxis(int _index, const math::Vector3 &_axis)
{
  if (this->childLink) this->childLink->SetEnabled(true);
  if (this->parentLink) this->parentLink->SetEnabled(true);

  if (_index == 0)
    dJointSetHinge2Axis1(this->jointId, _axis.x, _axis.y, _axis.z);
  else
    dJointSetHinge2Axis2(this->jointId, _axis.x, _axis.y, _axis.z);
}

//////////////////////////////////////////////////
void ODEHinge2Joint::SetDamping(int /*_index*/, const double _damping)
{
  dJointSetDamping(this->jointId, _damping);
}

//////////////////////////////////////////////////
math::Vector3 ODEHinge2Joint::GetGlobalAxis(int _index) const
{
  dVector3 result;

  if (_index == 0)
    dJointGetHinge2Axis1(this->jointId, result);
  else
    dJointGetHinge2Axis2(this->jointId, result);

  return math::Vector3(result[0], result[1], result[2]);
}

//////////////////////////////////////////////////
math::Angle ODEHinge2Joint::GetAngleImpl(int _index) const
{
  if (_index == 0)
    return dJointGetHinge2Angle1(this->jointId);
  else
    gzerr << "ODE has not function to get the second angle in a hinge2 joint";

  return math::Angle(0);
}

//////////////////////////////////////////////////
double ODEHinge2Joint::GetVelocity(int _index) const
{
  double result;

  if (_index == 0)
    result = dJointGetHinge2Angle1Rate(this->jointId);
  else
    result = dJointGetHinge2Angle2Rate(this->jointId);

  return result;
}

//////////////////////////////////////////////////
void ODEHinge2Joint::SetVelocity(int _index, double _angle)
{
  if (_index == 0)
    this->SetParam(dParamVel, _angle);
  else
    this->SetParam(dParamVel2, _angle);
}

//////////////////////////////////////////////////
double ODEHinge2Joint::GetMaxForce(int _index)
{
  if (_index == 0)
    return this->GetParam(dParamFMax);
  else
    return this->GetParam(dParamFMax2);
}


//////////////////////////////////////////////////
void ODEHinge2Joint::SetMaxForce(int _index, double _t)
{
  if (_index == 0)
    this->SetParam(dParamFMax, _t);
  else
    this->SetParam(dParamFMax2, _t);
}


//////////////////////////////////////////////////
void ODEHinge2Joint::SetForce(int _index, double _torque)
{
  if (this->childLink) this->childLink->SetEnabled(true);
  if (this->parentLink) this->parentLink->SetEnabled(true);

  if (_index == 0)
    dJointAddHinge2Torques(this->jointId, _torque, 0);
  else
    dJointAddHinge2Torques(this->jointId, 0, _torque);
}

//////////////////////////////////////////////////
double ODEHinge2Joint::GetParam(int _parameter) const
{
  double result = dJointGetHinge2Param(this->jointId, _parameter);

  return result;
}

//////////////////////////////////////////////////
void ODEHinge2Joint::SetParam(int _parameter, double _value)
{
  ODEJoint::SetParam(_parameter, _value);
  dJointSetHinge2Param(this->jointId, _parameter, _value);
}
