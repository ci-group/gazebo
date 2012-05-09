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
/* Desc: A BulletHingeJoint
 * Author: Nate Keonig, Andrew Howard
 * Date: 21 May 2003
 */
#include "common/Console.hh"
#include "common/Exception.hh"

#include "physics/bullet/BulletLink.hh"
#include "physics/bullet/BulletPhysics.hh"
#include "physics/bullet/BulletHingeJoint.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
BulletHingeJoint::BulletHingeJoint(btDynamicsWorld *_world)
    : HingeJoint<BulletJoint>()
{
  this->world = _world;
}

//////////////////////////////////////////////////
BulletHingeJoint::~BulletHingeJoint()
{
}

//////////////////////////////////////////////////
void BulletHingeJoint::Load(sdf::ElementPtr _sdf)
{
  HingeJoint<BulletJoint>::Load(_sdf);
}

//////////////////////////////////////////////////
void BulletHingeJoint::Attach(LinkPtr _one, LinkPtr _two)
{
  HingeJoint<BulletJoint>::Attach(_one, _two);

  BulletLinkPtr bulletChildLink =
    boost::shared_static_cast<BulletLink>(this->childLink);
  BulletLinkPtr bulletParentLink =
    boost::shared_static_cast<BulletLink>(this->parentLink);

  if (!bulletChildLink || !bulletParentLink)
    gzthrow("Requires bullet bodies");

  sdf::ElementPtr axisElem = this->sdf->GetElement("axis");
  math::Vector3 axis = axisElem->GetValueVector3("xyz");

  math::Vector3 pivotA, pivotB, axisA, axisB;

  // Compute the pivot point, based on the anchorPos
  pivotA = this->anchorPos - this->parentLink->GetWorldPose().pos;
  pivotB = this->anchorPos - this->childLink->GetWorldPose().pos;

  pivotA = this->parentLink->GetWorldPose().rot.RotateVectorReverse(pivotA);
  pivotB = this->childLink->GetWorldPose().rot.RotateVectorReverse(pivotB);

  axisA = this->parentLink->GetWorldPose().rot.RotateVectorReverse(axis);
  axisA = axisA.Round();

  axisB = this->childLink->GetWorldPose().rot.RotateVectorReverse(axis);
  axisB = axisB.Round();

  this->btHinge = new btHingeConstraint(
      *bulletParentLink->GetBulletLink(),
      *bulletChildLink->GetBulletLink(),
      btVector3(pivotA.x, pivotA.y, pivotA.z),
      btVector3(pivotB.x, pivotB.y, pivotB.z),
      btVector3(axisA.x, axisA.y, axisA.z),
      btVector3(axisB.x, axisB.y, axisB.z));

  this->constraint = this->btHinge;

  double angle = this->btHinge->getHingeAngle();
  this->btHinge->setLimit(angle - .4, angle + .4);
  // Add the joint to the world
  this->world->addConstraint(this->btHinge, true);

  // Allows access to impulse
  this->btHinge->enableFeedback(true);
}

//////////////////////////////////////////////////
math::Vector3 BulletHingeJoint::GetAnchor(int /*_index*/) const
{
  btTransform trans = this->btHinge->getAFrame();
  trans.getOrigin() +=
    this->btHinge->getRigidBodyA().getCenterOfMassTransform().getOrigin();
  return math::Vector3(trans.getOrigin().getX(),
      trans.getOrigin().getY(), trans.getOrigin().getZ());
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetAnchor(int /*_index*/,
                                 const math::Vector3 &/*_anchor*/)
{
  // The anchor (pivot in Bullet lingo), can only be set on creation
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetAxis(int /*_index*/, const math::Vector3 &/*_axis*/)
{
  // Bullet seems to handle setAxis improperly. It readjust all the pivot
  // points
  /*btmath::Vector3 vec(_axis.x, _axis.y, _axis.z);
  ((btHingeConstraint*)this->btHinge)->setAxis(vec);
  */
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetDamping(int /*index*/, double /*_damping*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
math::Angle BulletHingeJoint::GetAngle(int /*_index*/) const
{
  if (this->btHinge)
    return this->btHinge->getHingeAngle();
  else
    gzthrow("Joint has not been created");
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetVelocity(int /*_index*/, double /*_angle*/)
{
  // this->btHinge->enableAngularMotor(true, -_angle,
  // this->GetMaxForce(_index));
}

//////////////////////////////////////////////////
double BulletHingeJoint::GetVelocity(int /*_index*/) const
{
  gzerr << "Not implemented...\n";
  return 0;
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetMaxForce(int /*_index*/, double _t)
{
  this->btHinge->setMaxMotorImpulse(_t);
}

//////////////////////////////////////////////////
double BulletHingeJoint::GetMaxForce(int /*_index*/)
{
  return this->btHinge->getMaxMotorImpulse();
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetForce(int /*_index*/, double _torque)
{
  // math::Vector3 axis = this->GetLocalAxis(_index);
  // this->btHinge->enableAngularMotor(true);

  // z-axis of constraint frame
  btVector3 hingeAxisLocal =
    this->btHinge->getAFrame().getBasis().getColumn(2);

  btVector3 hingeAxisWorld =
    this->btHinge->getRigidBodyA().getWorldTransform().getBasis() *
    hingeAxisLocal;

  btVector3 hingeTorque = _torque * hingeAxisWorld;

  this->btHinge->getRigidBodyA().applyTorque(hingeTorque);
  this->btHinge->getRigidBodyB().applyTorque(-hingeTorque);
}


//////////////////////////////////////////////////
double BulletHingeJoint::GetForce(int /*_index*/)
{
  return this->btHinge->getAppliedImpulse();
  return 0;
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetHighStop(int /*_index*/, math::Angle /*_angle*/)
{
  if (this->btHinge)
  {
    // this function has additional parameters that we may one day
    // implement. Be warned that this function will reset them to default
    // settings
    // this->btHinge->setLimit(this->btHinge->getLowerLimit(),
    //                         _angle.GetAsRadian());
  }
  else
  {
    gzthrow("Joint must be created first");
  }
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetLowStop(int /*_index*/, math::Angle /*_angle*/)
{
  if (this->btHinge)
  {
    // this function has additional parameters that we may one day
    // implement. Be warned that this function will reset them to default
    // settings
    // this->btHinge->setLimit(-_angle.GetAsRadian(),
    //                         this->btHinge->getUpperLimit());
  }
  else
    gzthrow("Joint must be created first");
}

//////////////////////////////////////////////////
math::Angle BulletHingeJoint::GetHighStop(int /*_index*/)
{
  math::Angle result;

  if (this->btHinge)
    result = this->btHinge->getUpperLimit();
  else
    gzthrow("Joint must be created first");

  return result;
}

//////////////////////////////////////////////////
math::Angle BulletHingeJoint::GetLowStop(int /*_index*/)
{
  math::Angle result;
  if (this->btHinge)
    result = this->btHinge->getLowerLimit();
  else
    gzthrow("Joint must be created first");

  return result;
}

//////////////////////////////////////////////////
math::Vector3 BulletHingeJoint::GetGlobalAxis(int /*_index*/) const
{
  gzerr << "BulletHingeJoint::GetGlobalAxis not implemented\n";
  return math::Vector3();
}

//////////////////////////////////////////////////
math::Angle BulletHingeJoint::GetAngleImpl(int /*_index*/) const
{
  gzerr << "BulletHingeJoint::GetAngleImpl not implemented\n";
  return math::Angle();
}
