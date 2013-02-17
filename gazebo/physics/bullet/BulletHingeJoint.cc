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
/* Desc: A BulletHingeJoint
 * Author: Nate Koenig, Andrew Howard
 * Date: 21 May 2003
 */
#include "gazebo/common/Assert.hh"
#include "gazebo/common/Console.hh"
#include "gazebo/common/Exception.hh"

#include "gazebo/physics/bullet/BulletLink.hh"
#include "gazebo/physics/bullet/BulletPhysics.hh"
#include "gazebo/physics/bullet/BulletHingeJoint.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
BulletHingeJoint::BulletHingeJoint(btDynamicsWorld *_world, BasePtr _parent)
    : HingeJoint<BulletJoint>(_parent)
{
  this->world = _world;
  this->bulletHinge = NULL;
  this->angleOffset = 0;
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
void BulletHingeJoint::Init()
{
  HingeJoint<BulletJoint>::Init();

  // Cast to BulletLink
  BulletLinkPtr bulletChildLink =
    boost::shared_static_cast<BulletLink>(this->childLink);
  BulletLinkPtr bulletParentLink =
    boost::shared_static_cast<BulletLink>(this->parentLink);

  // Get axis unit vector (expressed in world frame).
  math::Vector3 axis = this->initialWorldAxis;
  if (axis == math::Vector3::Zero)
  {
    gzerr << "axis must have non-zero length, resetting to 0 0 1\n";
    axis.Set(0, 0, 1);
  }

  // Local variables used to compute pivots and axes in body-fixed frames
  // for the parent and child links.
  math::Vector3 pivotA, pivotB, axisA, axisB;
  math::Pose pose;

  // Initialize pivot[AB] to anchorPos, which is expressed in the
  // world coordinate frame.
  pivotA = this->anchorPos;
  pivotB = this->anchorPos;
  // Check if parentLink exists. If not, the parent will be the world.
  if (this->parentLink)
  {
    // Compute relative pose between joint anchor and CoG of parent link.
    pose = this->parentLink->GetWorldCoGPose();
    // Subtract CoG position from anchor position, both in world frame.
    pivotA -= pose.pos;
    // Rotate pivot offset and axis into body-fixed frame of parent.
    pivotA = pose.rot.RotateVectorReverse(pivotA);
    axisA = pose.rot.RotateVectorReverse(axis);
    axisA = axisA.Normalize();
  }
  // Check if childLink exists. If not, the child will be the world.
  if (this->childLink)
  {
    // Compute relative pose between joint anchor and CoG of child link.
    pose = this->childLink->GetWorldCoGPose();
    // Subtract CoG position from anchor position, both in world frame.
    pivotB -= pose.pos;
    // Rotate pivot offset and axis into body-fixed frame of child.
    pivotB = pose.rot.RotateVectorReverse(pivotB);
    axisB = pose.rot.RotateVectorReverse(axis);
    axisB = axisB.Normalize();
  }

  // If both links exist, then create a joint between the two links.
  if (bulletChildLink && bulletParentLink)
  {
    this->bulletHinge = new btHingeConstraint(
        *(bulletParentLink->GetBulletLink()),
        *(bulletChildLink->GetBulletLink()),
        BulletTypes::ConvertVector3(pivotA),
        BulletTypes::ConvertVector3(pivotB),
        BulletTypes::ConvertVector3(axisA),
        BulletTypes::ConvertVector3(axisB));
  }
  // If only the child exists, then create a joint between the child
  // and the world.
  else if (bulletChildLink)
  {
    this->bulletHinge = new btHingeConstraint(
        *(bulletChildLink->GetBulletLink()),
        BulletTypes::ConvertVector3(pivotB),
        BulletTypes::ConvertVector3(axisB));
  }
  // If only the parent exists, then create a joint between the parent
  // and the world.
  else if (bulletParentLink)
  {
    this->bulletHinge = new btHingeConstraint(
        *(bulletParentLink->GetBulletLink()),
        BulletTypes::ConvertVector3(pivotA),
        BulletTypes::ConvertVector3(axisA));
  }
  // Throw an error if no links are given.
  else
  {
    gzthrow("joint without links\n");
  }

  if (!this->bulletHinge)
    gzthrow("memory allocation error\n");

  // Give parent class BulletJoint a pointer to this constraint.
  this->constraint = this->bulletHinge;

  // Set angleOffset based on hinge angle at joint creation.
  // GetAngleImpl will report angles relative to this offset.
  this->angleOffset = this->bulletHinge->getHingeAngle();

  // Apply joint angle limits here.
  // TODO: velocity and effort limits.
  GZ_ASSERT(this->sdf != NULL, "Joint sdf member is NULL");
  sdf::ElementPtr limitElem;
  limitElem = this->sdf->GetElement("axis")->GetElement("limit");
  this->bulletHinge->setLimit(
    this->angleOffset + limitElem->GetValueDouble("lower"),
    this->angleOffset + limitElem->GetValueDouble("upper"));

  // Add the joint to the world
  this->world->addConstraint(this->bulletHinge, true);

  // Allows access to impulse
  this->bulletHinge->enableFeedback(true);
}

//////////////////////////////////////////////////
math::Vector3 BulletHingeJoint::GetAnchor(int /*_index*/) const
{
  btTransform trans = this->bulletHinge->getAFrame();
  trans.getOrigin() +=
    this->bulletHinge->getRigidBodyA().getCenterOfMassTransform().getOrigin();
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
void BulletHingeJoint::SetAxis(int /*_index*/, const math::Vector3 &_axis)
{
  // Note that _axis is given in a world frame,
  // but bullet uses a body-fixed frame
  if (this->bulletHinge == NULL)
  {
    // this hasn't been initialized yet, store axis in initialWorldAxis
    this->initialWorldAxis = _axis;
  }
  else
  {
    gzerr << "not implemented\n";
  }

  // Bullet seems to handle setAxis improperly. It readjust all the pivot
  // points
  /*btmath::Vector3 vec(_axis.x, _axis.y, _axis.z);
  ((btHingeConstraint*)this->bulletHinge)->setAxis(vec);
  */
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetDamping(int /*index*/, double /*_damping*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
math::Angle BulletHingeJoint::GetAngleImpl(int /*_index*/) const
{
  math::Angle result;
  if (this->bulletHinge != NULL)
    result = this->bulletHinge->getHingeAngle() - this->angleOffset;
  else
    gzwarn << "bulletHinge does not exist, returning default angle\n";
  return result;
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetVelocity(int /*_index*/, double /*_angle*/)
{
  // this->bulletHinge->enableAngularMotor(true, -_angle,
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
  this->bulletHinge->setMaxMotorImpulse(_t);
}

//////////////////////////////////////////////////
double BulletHingeJoint::GetMaxForce(int /*_index*/)
{
  return this->bulletHinge->getMaxMotorImpulse();
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetForce(int /*_index*/, double _torque)
{
  // math::Vector3 axis = this->GetLocalAxis(_index);
  // this->bulletHinge->enableAngularMotor(true);

  // z-axis of constraint frame
  btVector3 hingeAxisLocal =
    this->bulletHinge->getAFrame().getBasis().getColumn(2);

  btVector3 hingeAxisWorld =
    this->bulletHinge->getRigidBodyA().getWorldTransform().getBasis() *
    hingeAxisLocal;

  btVector3 hingeTorque = _torque * hingeAxisWorld;

  this->bulletHinge->getRigidBodyA().applyTorque(hingeTorque);
  this->bulletHinge->getRigidBodyB().applyTorque(-hingeTorque);
}

//////////////////////////////////////////////////
double BulletHingeJoint::GetForce(int /*_index*/)
{
  return this->bulletHinge->getAppliedImpulse();
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetHighStop(int /*_index*/,
                      const math::Angle &_angle)
{
  Joint::SetHighStop(0, _angle);
  if (this->bulletHinge)
  {
    // this function has additional parameters that we may one day
    // implement. Be warned that this function will reset them to default
    // settings
    this->bulletHinge->setLimit(this->bulletHinge->getLowerLimit(),
                                this->angleOffset + _angle.Radian());
  }
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetLowStop(int /*_index*/,
                     const math::Angle &_angle)
{
  Joint::SetLowStop(0, _angle);
  if (this->bulletHinge)
  {
    // this function has additional parameters that we may one day
    // implement. Be warned that this function will reset them to default
    // settings
    this->bulletHinge->setLimit(this->angleOffset + _angle.Radian(),
                                this->bulletHinge->getUpperLimit());
  }
}

//////////////////////////////////////////////////
math::Angle BulletHingeJoint::GetHighStop(int /*_index*/)
{
  math::Angle result;

  if (this->bulletHinge)
    result = this->bulletHinge->getUpperLimit();
  else
    gzthrow("Joint must be created first");

  return result;
}

//////////////////////////////////////////////////
math::Angle BulletHingeJoint::GetLowStop(int /*_index*/)
{
  math::Angle result;
  if (this->bulletHinge)
    result = this->bulletHinge->getLowerLimit();
  else
    gzthrow("Joint must be created first");

  return result;
}

//////////////////////////////////////////////////
math::Vector3 BulletHingeJoint::GetGlobalAxis(int /*_index*/) const
{
  math::Vector3 result;
  if (this->bulletHinge)
  {
    // I have not verified the following math, though I based it on internal
    // bullet code at line 250 of btHingeConstraint.cpp
    btVector3 vec =
      bulletHinge->getRigidBodyA().getCenterOfMassTransform().getBasis() *
      bulletHinge->getFrameOffsetA().getBasis().getColumn(2);
    result = BulletTypes::ConvertVector3(vec);
  }
  else
    gzwarn << "bulletHinge does not exist, returning fake axis\n";
  return result;
}
