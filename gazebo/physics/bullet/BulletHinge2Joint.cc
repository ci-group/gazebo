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

#include "common/Exception.hh"
#include "common/Console.hh"

#include "physics/bullet/BulletTypes.hh"
#include "physics/bullet/BulletLink.hh"
#include "physics/bullet/BulletPhysics.hh"
#include "physics/bullet/BulletHinge2Joint.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
BulletHinge2Joint::BulletHinge2Joint(btDynamicsWorld *_world)
    : Hinge2Joint<BulletJoint>()
{
  this->world = _world;
}

//////////////////////////////////////////////////
BulletHinge2Joint::~BulletHinge2Joint()
{
}

//////////////////////////////////////////////////
void BulletHinge2Joint::Load(sdf::ElementPtr _sdf)
{
  Hinge2Joint<BulletJoint>::Load(_sdf);
}

//////////////////////////////////////////////////
void BulletHinge2Joint::Attach(LinkPtr _one, LinkPtr _two)
{
  Hinge2Joint<BulletJoint>::Attach(_one, _two);

  BulletLinkPtr bulletChildLink =
    boost::shared_static_cast<BulletLink>(this->childLink);
  BulletLinkPtr bulletParentLink =
    boost::shared_static_cast<BulletLink>(this->parentLink);

  if (!bulletChildLink || !bulletParentLink)
    gzthrow("Requires bullet bodies");

  sdf::ElementPtr axis1Elem = this->sdf->GetElement("axis");
  math::Vector3 axis1 = axis1Elem->GetValueVector3("xyz");

  sdf::ElementPtr axis2Elem = this->sdf->GetElement("axis");
  math::Vector3 axis2 = axis2Elem->GetValueVector3("xyz");

  btVector3 banchor(this->anchorPos.x, this->anchorPos.y, this->anchorPos.z);
  btVector3 baxis1(axis1.x, axis1.y, axis1.z);
  btVector3 baxis2(axis2.x, axis2.y, axis2.z);

  this->btHinge2 = new btHinge2Constraint(
      *bulletParentLink->GetBulletLink(),
      *bulletChildLink->GetBulletLink(),
      banchor, baxis1, baxis2);

  this->constraint = this->btHinge2;

  // Add the joint to the world
  this->world->addConstraint(this->constraint, true);

  // Allows access to impulse
  this->constraint->enableFeedback(true);
}

//////////////////////////////////////////////////
math::Vector3 BulletHinge2Joint::GetAnchor(int /*index*/) const
{
  return this->anchorPos;
}

//////////////////////////////////////////////////
math::Vector3 BulletHinge2Joint::GetAxis(int /*index*/) const
{
  btVector3 vec = this->btHinge2->getAxis1();
  return math::Vector3(vec.getX(), vec.getY(), vec.getZ());
}

//////////////////////////////////////////////////
math::Angle BulletHinge2Joint::GetAngle(int /*_index*/) const
{
  return this->btHinge2->getAngle1();
}

//////////////////////////////////////////////////
double BulletHinge2Joint::GetVelocity(int /*_index*/) const
{
  gzerr << "Not implemented";
  return 0;
}

//////////////////////////////////////////////////
void BulletHinge2Joint::SetVelocity(int /*_index*/, double /*_angle*/)
{
  gzerr << "Not implemented";
}

//////////////////////////////////////////////////
void BulletHinge2Joint::SetAnchor(int /*_index*/,
                                  const math::Vector3 &/*_anchor*/)
{
  gzerr << "Not implemented";
}

//////////////////////////////////////////////////
void BulletHinge2Joint::SetAxis(int /*_index*/, const math::Vector3 &/*_axis*/)
{
  // Bullet seems to handle setAxis improperly. It readjust all the pivot
  // points
  /*btmath::Vector3 vec(_axis.x, _axis.y, _axis.z);
  ((btHingeConstraint*)this->btHinge)->setAxis(vec);
  */
}

//////////////////////////////////////////////////
void BulletHinge2Joint::SetDamping(int /*index*/, double /*_damping*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
void BulletHinge2Joint::SetForce(int /*_index*/, double /*_torque*/)
{
  gzerr << "Not implemented";
}

//////////////////////////////////////////////////
void BulletHinge2Joint::SetMaxForce(int /*_index*/, double /*_t*/)
{
  gzerr << "Not implemented";
}

//////////////////////////////////////////////////
double BulletHinge2Joint::GetMaxForce(int /*_index*/)
{
  gzerr << "Not implemented";
  return 0;
}

//////////////////////////////////////////////////
void BulletHinge2Joint::SetHighStop(int /*_index*/, math::Angle _angle)
{
  this->btHinge2->setUpperLimit(_angle.GetAsRadian());
}

//////////////////////////////////////////////////
void BulletHinge2Joint::SetLowStop(int /*_index*/, math::Angle _angle)
{
  this->btHinge2->setLowerLimit(_angle.GetAsRadian());
}

//////////////////////////////////////////////////
math::Angle BulletHinge2Joint::GetHighStop(int _index)
{
  btRotationalLimitMotor *motor =
    this->btHinge2->getRotationalLimitMotor(_index);
  if (motor)
    return motor->m_hiLimit;

  gzthrow("Unable to get high stop for axis _index[" << _index << "]");
  return 0;
}

//////////////////////////////////////////////////
math::Angle BulletHinge2Joint::GetLowStop(int _index)
{
  btRotationalLimitMotor *motor =
    this->btHinge2->getRotationalLimitMotor(_index);
  if (motor)
    return motor->m_loLimit;

  gzthrow("Unable to get high stop for axis _index[" << _index << "]");
  return 0;
}

//////////////////////////////////////////////////
math::Vector3 BulletHinge2Joint::GetGlobalAxis(int /*_index*/) const
{
  gzerr << "BulletHinge2Joint::GetGlobalAxis not implemented\n";
  return math::Vector3();
}

//////////////////////////////////////////////////
math::Angle BulletHinge2Joint::GetAngleImpl(int /*_index*/) const
{
  gzerr << "BulletHinge2Joint::GetAngleImpl not implemented\n";
  return math::Angle();
}
