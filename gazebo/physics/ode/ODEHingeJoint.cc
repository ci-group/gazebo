/*
 * Copyright (C) 2012-2013 Open Source Robotics Foundation
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
 * Author: Nate Koenig, Andrew Howard
 * Date: 21 May 2003
 */

#include <boost/bind.hpp>

#include "gazebo/gazebo_config.h"
#include "gazebo/common/Console.hh"

#include "gazebo/physics/Model.hh"
#include "gazebo/physics/Link.hh"
#include "gazebo/physics/ode/ODEHingeJoint.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
ODEHingeJoint::ODEHingeJoint(dWorldID _worldId, BasePtr _parent)
    : HingeJoint<ODEJoint>(_parent)
{
  this->jointId = dJointCreateHinge(_worldId, NULL);
}

//////////////////////////////////////////////////
ODEHingeJoint::~ODEHingeJoint()
{
  if (this->applyDamping)
    physics::Joint::DisconnectJointUpdate(this->applyDamping);
}

//////////////////////////////////////////////////
void ODEHingeJoint::Load(sdf::ElementPtr _sdf)
{
  HingeJoint<ODEJoint>::Load(_sdf);

  this->SetParam(dParamFMax, 0);
}

//////////////////////////////////////////////////
math::Vector3 ODEHingeJoint::GetAnchor(int /*index*/) const
{
  dVector3 result;

  if (this->jointId)
    dJointGetHingeAnchor(this->jointId, result);
  else
    gzerr << "ODE Joint ID is invalid\n";

  return math::Vector3(result[0], result[1], result[2]);
}

//////////////////////////////////////////////////
void ODEHingeJoint::SetAnchor(int /*index*/, const math::Vector3 &_anchor)
{
  if (this->childLink)
    this->childLink->SetEnabled(true);
  if (this->parentLink)
    this->parentLink->SetEnabled(true);

  if (this->jointId)
    dJointSetHingeAnchor(this->jointId, _anchor.x, _anchor.y, _anchor.z);
  else
    gzerr << "ODE Joint ID is invalid\n";
}


//////////////////////////////////////////////////
math::Vector3 ODEHingeJoint::GetGlobalAxis(int /*_index*/) const
{
  dVector3 result;
  if (this->jointId)
    dJointGetHingeAxis(this->jointId, result);
  else
    gzerr << "ODE Joint ID is invalid\n";

  return math::Vector3(result[0], result[1], result[2]);
}

//////////////////////////////////////////////////
void ODEHingeJoint::SetAxis(int _index, const math::Vector3 &_axis)
{
  ODEJoint::SetAxis(_index, _axis);

  if (this->childLink)
    this->childLink->SetEnabled(true);
  if (this->parentLink)
    this->parentLink->SetEnabled(true);

  /// ODE needs global axis
  /// \TODO: currently we assume joint axis is specified in model frame,
  /// this is incorrect, and should be corrected to be
  /// joint frame which is specified in child link frame.
  math::Vector3 globalAxis = _axis;
  if (this->parentLink)
    globalAxis =
      this->GetParent()->GetModel()->GetWorldPose().rot.RotateVector(_axis);

  if (this->jointId)
    dJointSetHingeAxis(this->jointId, globalAxis.x, globalAxis.y, globalAxis.z);
  else
    gzerr << "ODE Joint ID is invalid\n";
}

//////////////////////////////////////////////////
math::Angle ODEHingeJoint::GetAngleImpl(int /*index*/) const
{
  math::Angle result;
  if (this->jointId)
    result = math::fixnan(dJointGetHingeAngle(this->jointId));
  else
    gzerr << "ODE Joint ID is invalid\n";

  return result;
}

//////////////////////////////////////////////////
double ODEHingeJoint::GetVelocity(int /*index*/) const
{
  double result = 0;

  if (this->jointId)
    result = math::fixnan(dJointGetHingeAngleRate(this->jointId));
  else
    gzerr << "ODE Joint ID is invalid\n";

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
void ODEHingeJoint::SetForceImpl(int /*_index*/, double _effort)
{
  if (this->jointId)
    dJointAddHingeTorque(this->jointId, _effort);
  else
    gzerr << "ODE Joint ID is invalid\n";
}

//////////////////////////////////////////////////
double ODEHingeJoint::GetParam(int _parameter) const
{
  double result = 0;

  if (this->jointId)
    result = math::fixnan(dJointGetHingeParam(this->jointId, _parameter));
  else
    gzerr << "ODE Joint ID is invalid\n";

  return result;
}

//////////////////////////////////////////////////
void ODEHingeJoint::SetParam(int _parameter, double _value)
{
  ODEJoint::SetParam(_parameter, _value);

  if (this->jointId)
    dJointSetHingeParam(this->jointId, _parameter, _value);
  else
    gzerr << "ODE Joint ID is invalid\n";
}
