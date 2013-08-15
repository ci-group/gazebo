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

#include "gazebo/physics/Model.hh"
#include "gazebo/physics/bullet/BulletLink.hh"
#include "gazebo/physics/bullet/BulletPhysics.hh"
#include "gazebo/physics/bullet/BulletHingeJoint.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
BulletHingeJoint::BulletHingeJoint(btDynamicsWorld *_world, BasePtr _parent)
    : HingeJoint<BulletJoint>(_parent)
{
  GZ_ASSERT(_world, "bullet world pointer is NULL");
  this->bulletWorld = _world;
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
    boost::static_pointer_cast<BulletLink>(this->childLink);
  BulletLinkPtr bulletParentLink =
    boost::static_pointer_cast<BulletLink>(this->parentLink);

  // Get axis unit vector (expressed in world frame).
  math::Vector3 axis = this->initialWorldAxis;
  if (axis == math::Vector3::Zero)
  {
    gzerr << "axis must have non-zero length, resetting to 0 0 1\n";
    axis.Set(0, 0, 1);
  }

  // Local variables used to compute pivots and axes in body-fixed frames
  // for the parent and child links.
  math::Vector3 pivotParent, pivotChild, axisParent, axisChild;
  math::Pose pose;

  // Initialize pivots to anchorPos, which is expressed in the
  // world coordinate frame.
  pivotParent = this->anchorPos;
  pivotChild = this->anchorPos;

  // Check if parentLink exists. If not, the parent will be the world.
  if (this->parentLink)
  {
    // Compute relative pose between joint anchor and CoG of parent link.
    pose = this->parentLink->GetWorldCoGPose();
    // Subtract CoG position from anchor position, both in world frame.
    pivotParent -= pose.pos;
    // Rotate pivot offset and axis into body-fixed frame of parent.
    pivotParent = pose.rot.RotateVectorReverse(pivotParent);
    axisParent = pose.rot.RotateVectorReverse(axis);
    axisParent = axisParent.Normalize();
  }
  // Check if childLink exists. If not, the child will be the world.
  if (this->childLink)
  {
    // Compute relative pose between joint anchor and CoG of child link.
    pose = this->childLink->GetWorldCoGPose();
    // Subtract CoG position from anchor position, both in world frame.
    pivotChild -= pose.pos;
    // Rotate pivot offset and axis into body-fixed frame of child.
    pivotChild = pose.rot.RotateVectorReverse(pivotChild);
    axisChild = pose.rot.RotateVectorReverse(axis);
    axisChild = axisChild.Normalize();
  }

  // If both links exist, then create a joint between the two links.
  if (bulletChildLink && bulletParentLink)
  {
    this->bulletHinge = new btHingeConstraint(
        *(bulletChildLink->GetBulletLink()),
        *(bulletParentLink->GetBulletLink()),
        BulletTypes::ConvertVector3(pivotChild),
        BulletTypes::ConvertVector3(pivotParent),
        BulletTypes::ConvertVector3(axisChild),
        BulletTypes::ConvertVector3(axisParent));
  }
  // If only the child exists, then create a joint between the child
  // and the world.
  else if (bulletChildLink)
  {
    this->bulletHinge = new btHingeConstraint(
        *(bulletChildLink->GetBulletLink()),
        BulletTypes::ConvertVector3(pivotChild),
        BulletTypes::ConvertVector3(axisChild));
  }
  // If only the parent exists, then create a joint between the parent
  // and the world.
  else if (bulletParentLink)
  {
    this->bulletHinge = new btHingeConstraint(
        *(bulletParentLink->GetBulletLink()),
        BulletTypes::ConvertVector3(pivotParent),
        BulletTypes::ConvertVector3(axisParent));
  }
  // Throw an error if no links are given.
  else
  {
    gzthrow("joint without links\n");
  }

  if (!this->bulletHinge)
    gzthrow("unable to create bullet hinge constraint\n");

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
    this->angleOffset + limitElem->Get<double>("lower"),
    this->angleOffset + limitElem->Get<double>("upper"));

  // Add the joint to the world
  GZ_ASSERT(this->bulletWorld, "bullet world pointer is NULL");
  this->bulletWorld->addConstraint(this->bulletHinge, true);

  // Allows access to impulse
  this->bulletHinge->enableFeedback(true);

  // Setup Joint force and torque feedback
  this->SetupJointFeedback();
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

    /// \TODO: currently we assume joint axis is specified in model frame,
    /// this is incorrect, and should be corrected to be
    /// joint frame which is specified in child link frame.
    if (this->parentLink)
      this->initialWorldAxis =
        this->GetParent()->GetModel()->GetWorldPose().rot.RotateVector(_axis);
    else
      this->initialWorldAxis = _axis;
  }
  else
  {
    gzerr << "SetAxis for existing joint is not implemented\n";
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
  if (this->bulletHinge)
    result = this->bulletHinge->getHingeAngle() - this->angleOffset;
  else
    gzwarn << "bulletHinge does not exist, returning default angle\n";
  return result;
}

//////////////////////////////////////////////////
void BulletHingeJoint::SetVelocity(int _index, double _angle)
{
  // The following call should work, but doesn't:
  // this->bulletHinge->enableAngularMotor(true, _angle,
  // this->GetMaxForce(_index));

  // TODO: Should prescribe the linear velocity of the child link if there is an
  // offset between the child's CG and the joint anchor.
  math::Vector3 desiredVel;
  if (this->parentLink)
    desiredVel = this->parentLink->GetWorldAngularVel();
  desiredVel += _angle * this->GetGlobalAxis(_index);
  if (this->childLink)
    this->childLink->SetAngularVel(desiredVel);
}

//////////////////////////////////////////////////
double BulletHingeJoint::GetVelocity(int /*_index*/) const
{
  double result = 0;
  math::Vector3 globalAxis = this->GetGlobalAxis(0);
  if (this->childLink)
    result += globalAxis.Dot(this->childLink->GetWorldAngularVel());
  if (this->parentLink)
    result -= globalAxis.Dot(this->parentLink->GetWorldAngularVel());
  return result;
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
void BulletHingeJoint::SetForce(int _index, double _effort)
{
  if (_index < 0 || static_cast<unsigned int>(_index) >= this->GetAngleCount())
  {
    gzerr << "Calling BulletHingeJoint::SetForce with an index ["
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

  // truncate effort unless effortLimit is negative.
  if (this->effortLimit[_index] >= 0)
    _effort = math::clamp(_effort, -this->effortLimit[_index],
       this->effortLimit[_index]);

  if (this->bulletHinge)
  {
    BulletJoint::SetForce(_index, _effort);

    // z-axis of constraint frame
    btVector3 hingeAxisLocalA =
      this->bulletHinge->getFrameOffsetA().getBasis().getColumn(2);
    btVector3 hingeAxisLocalB =
      this->bulletHinge->getFrameOffsetB().getBasis().getColumn(2);

    btVector3 hingeAxisWorldA =
      this->bulletHinge->getRigidBodyA().getWorldTransform().getBasis() *
      hingeAxisLocalA;
    btVector3 hingeAxisWorldB =
      this->bulletHinge->getRigidBodyB().getWorldTransform().getBasis() *
      hingeAxisLocalB;

    btVector3 hingeTorqueA = _effort * hingeAxisWorldA;
    btVector3 hingeTorqueB = _effort * hingeAxisWorldB;

    this->bulletHinge->getRigidBodyA().applyTorque(hingeTorqueA);
    this->bulletHinge->getRigidBodyB().applyTorque(-hingeTorqueB);
  }
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
    gzerr << "Joint must be created before getting high stop\n";

  return result;
}

//////////////////////////////////////////////////
math::Angle BulletHingeJoint::GetLowStop(int /*_index*/)
{
  math::Angle result;
  if (this->bulletHinge)
    result = this->bulletHinge->getLowerLimit();
  else
    gzerr << "Joint must be created before getting low stop\n";

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
