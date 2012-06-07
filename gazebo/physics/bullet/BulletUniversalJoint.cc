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
/* Desc: A universal joint
 * Author: Nate Keonig
 * Date: 24 May 2009
 */

#include "common/Exception.hh"
#include "common/Console.hh"
#include "physics/bullet/BulletLink.hh"
#include "physics/bullet/BulletTypes.hh"
#include "physics/bullet/BulletUniversalJoint.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
BulletUniversalJoint::BulletUniversalJoint(btDynamicsWorld *_world)
    : UniversalJoint<BulletJoint>()
{
  this->world = _world;
}

//////////////////////////////////////////////////
BulletUniversalJoint::~BulletUniversalJoint()
{
}

//////////////////////////////////////////////////
void BulletUniversalJoint::Attach(LinkPtr _one, LinkPtr _two)
{
  UniversalJoint<BulletJoint>::Attach(_one, _two);

  BulletLinkPtr bulletChildLink =
    boost::shared_static_cast<BulletLink>(this->childLink);
  BulletLinkPtr bulletParentLink =
    boost::shared_static_cast<BulletLink>(this->parentLink);

  if (!bulletChildLink || !bulletParentLink)
    gzthrow("Requires bullet bodies");

  sdf::ElementPtr axisElem = this->sdf->GetElement("axis");
  math::Vector3 axis1 = axisElem->GetValueVector3("xyz");
  math::Vector3 axis2 = axisElem->GetValueVector3("xyz");

  this->btUniversal = new btUniversalConstraint(
      *bulletParentLink->GetBulletLink(),
      *bulletChildLink->GetBulletLink(),
      btVector3(this->anchorPos.x, this->anchorPos.y, this->anchorPos.z),
      btVector3(axis1.x, axis1.y, axis1.z),
      btVector3(axis2.x, axis2.y, axis2.z));

  this->constraint = this->btUniversal;

  // Add the joint to the world
  this->world->addConstraint(this->btUniversal, true);

  // Allows access to impulse
  this->btUniversal->enableFeedback(true);
}

//////////////////////////////////////////////////
math::Vector3 BulletUniversalJoint::GetAnchor(int /*index*/) const
{
  return this->anchorPos;
}

//////////////////////////////////////////////////
void BulletUniversalJoint::SetAnchor(int /*_index*/,
                                     const math::Vector3 &/*_anchor*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
math::Vector3 BulletUniversalJoint::GetAxis(int _index) const
{
  btVector3 axis = this->btUniversal->getAxis(_index);
  return math::Vector3(axis.getX(), axis.getY(), axis.getZ());
}

//////////////////////////////////////////////////
void BulletUniversalJoint::SetDamping(int /*index*/, double /*_damping*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
void BulletUniversalJoint::SetAxis(int /*_index*/,
                                   const math::Vector3 &/*_axis*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
math::Angle BulletUniversalJoint::GetAngle(int _index) const
{
  if (_index == 0)
    return this->btUniversal->getAngle1();
  else
    return this->btUniversal->getAngle2();
}

//////////////////////////////////////////////////
double BulletUniversalJoint::GetVelocity(int /*_index*/) const
{
  gzerr << "Not implemented\n";
  return 0;
}

//////////////////////////////////////////////////
void BulletUniversalJoint::SetVelocity(int /*_index*/, double /*_angle*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
void BulletUniversalJoint::SetForce(int /*_index*/, double /*_torque*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
void BulletUniversalJoint::SetMaxForce(int /*_index*/, double /*_t*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
double BulletUniversalJoint::GetMaxForce(int /*_index*/)
{
  gzerr << "Not implemented\n";
  return 0;
}

//////////////////////////////////////////////////
void BulletUniversalJoint::SetHighStop(int _index, math::Angle _angle)
{
  if (this->btUniversal)
  {
    if (_index == 0)
      this->btUniversal->setUpperLimit(
        _angle.GetAsRadian(), this->GetHighStop(1).GetAsRadian());
    else
      this->btUniversal->setUpperLimit(
        this->GetHighStop(0).GetAsRadian(), _angle.GetAsRadian());
  }
  else
    gzthrow("Joint must be created first");
}

//////////////////////////////////////////////////
void BulletUniversalJoint::SetLowStop(int _index, math::Angle _angle)
{
  if (this->btUniversal)
  {
    if (_index == 0)
      this->btUniversal->setLowerLimit(
        _angle.GetAsRadian(), this->GetLowStop(1).GetAsRadian());
    else
      this->btUniversal->setUpperLimit(
        this->GetLowStop(0).GetAsRadian(), _angle.GetAsRadian());
  }
  else
    gzthrow("Joint must be created first");
}

//////////////////////////////////////////////////
math::Angle BulletUniversalJoint::GetHighStop(int _index)
{
  math::Angle result;

  if (this->btUniversal)
  {
    btRotationalLimitMotor *motor;
    motor = this->btUniversal->getRotationalLimitMotor(_index);
    result = motor->m_hiLimit;
  }
  else
    gzthrow("Joint must be created first");

  return result;
}

//////////////////////////////////////////////////
math::Angle BulletUniversalJoint::GetLowStop(int _index)
{
  math::Angle result;

  if (this->btUniversal)
  {
    btRotationalLimitMotor *motor;
    motor = this->btUniversal->getRotationalLimitMotor(_index);
    result = motor->m_loLimit;
  }
  else
    gzthrow("Joint must be created first");

  return result;
}

//////////////////////////////////////////////////
math::Vector3 BulletUniversalJoint::GetGlobalAxis(int /*_index*/) const
{
  gzerr << "BulletUniversalJoint::GetGlobalAxis not implemented\n";
  return math::Vector3();
}

//////////////////////////////////////////////////
math::Angle BulletUniversalJoint::GetAngleImpl(int /*_index*/) const
{
  gzerr << "BulletUniversalJoint::GetAngleImpl not implemented\n";
  return math::Angle();
}
