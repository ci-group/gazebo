/*
 * Copyright (C) 2012-2016 Open Source Robotics Foundation
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
/* Desc: A ball joint
 * Author: Nate Koenig, Andrew Howard
 * Date: 21 May 2003
 */

#include "gazebo/common/Assert.hh"
#include "gazebo/common/Console.hh"
#include "gazebo/common/Exception.hh"

#include "gazebo/physics/bullet/BulletTypes.hh"
#include "gazebo/physics/bullet/BulletLink.hh"
#include "gazebo/physics/bullet/BulletBallJoint.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
BulletBallJoint::BulletBallJoint(btDynamicsWorld *_world, BasePtr _parent)
    : BallJoint<BulletJoint>(_parent)
{
  GZ_ASSERT(_world, "bullet world pointer is NULL");
  this->bulletWorld = _world;
  this->bulletBall = NULL;
}

//////////////////////////////////////////////////
BulletBallJoint::~BulletBallJoint()
{
}

//////////////////////////////////////////////////
void BulletBallJoint::Load(sdf::ElementPtr _sdf)
{
  BallJoint<BulletJoint>::Load(_sdf);
}

//////////////////////////////////////////////////
void BulletBallJoint::Init()
{
  BallJoint<BulletJoint>::Init();

  // Cast to BulletLink
  BulletLinkPtr bulletChildLink =
    boost::static_pointer_cast<BulletLink>(this->childLink);
  BulletLinkPtr bulletParentLink =
    boost::static_pointer_cast<BulletLink>(this->parentLink);

  // Local variables used to compute pivots and axes in body-fixed frames
  // for the parent and child links.
  math::Vector3 pivotParent, pivotChild;
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
  }

  // If both links exist, then create a joint between the two links.
  if (bulletChildLink && bulletParentLink)
  {
    this->bulletBall = new btPoint2PointConstraint(
      *bulletChildLink->GetBulletLink(),
      *bulletParentLink->GetBulletLink(),
      BulletTypes::ConvertVector3(pivotChild),
      BulletTypes::ConvertVector3(pivotParent));
  }
  // If only the child exists, then create a joint between the child
  // and the world.
  else if (bulletChildLink)
  {
    this->bulletBall = new btPoint2PointConstraint(
      *bulletChildLink->GetBulletLink(),
      BulletTypes::ConvertVector3(pivotChild));
  }
  // If only the parent exists, then create a joint between the parent
  // and the world.
  else if (bulletParentLink)
  {
    this->bulletBall = new btPoint2PointConstraint(
      *bulletParentLink->GetBulletLink(),
      BulletTypes::ConvertVector3(pivotParent));
  }
  // Throw an error if no links are given.
  else
  {
    gzthrow("joint without links\n");
  }

  this->constraint = this->bulletBall;

  // Add the joint to the world
  GZ_ASSERT(this->bulletWorld, "bullet world pointer is NULL");
  this->bulletWorld->addConstraint(this->constraint);

  // Allows access to impulse
  this->constraint->enableFeedback(true);

  // Setup Joint force and torque feedback
  this->SetupJointFeedback();
}

//////////////////////////////////////////////////
math::Vector3 BulletBallJoint::GetAnchor(unsigned int /*_index*/) const
{
  return this->anchorPos;
}

/////////////////////////////////////////////////
void BulletBallJoint::SetVelocity(unsigned int /*_index*/, double /*_angle*/)
{
  gzerr << "Not implemented\n";
}

/////////////////////////////////////////////////
double BulletBallJoint::GetVelocity(unsigned int /*_index*/) const
{
  gzerr << "Not implemented\n";
  return 0;
}

/////////////////////////////////////////////////
math::Vector3 BulletBallJoint::GetGlobalAxis(unsigned int /*_index*/) const
{
  gzerr << "Not implemented\n";
  return math::Vector3();
}

/////////////////////////////////////////////////
math::Angle BulletBallJoint::GetAngleImpl(unsigned int /*_index*/) const
{
  gzerr << "Not implemented\n";
  return math::Angle();
}

//////////////////////////////////////////////////
bool BulletBallJoint::SetHighStop(unsigned int /*_index*/,
                                   const math::Angle &/*_angle*/)
{
  if (this->bulletBall)
  {
    // this function has additional parameters that we may one day
    // implement. Be warned that this function will reset them to default
    // settings
    // this->bulletBall->setLimit(this->btBall->getLowerLimit(),
    //                         _angle.Radian());
    gzerr << "BulletBallJoint limits not implemented" << std::endl;
    return false;
  }
  else
  {
    gzerr << "bulletBall does not yet exist" << std::endl;
    return false;
  }
}

//////////////////////////////////////////////////
void BulletBallJoint::SetForceImpl(unsigned int /*_index*/, double /*_torque*/)
{
  gzerr << "Not implemented";
}

//////////////////////////////////////////////////
bool BulletBallJoint::SetLowStop(unsigned int /*_index*/,
                                  const math::Angle &/*_angle*/)
{
  if (this->bulletBall)
  {
    // this function has additional parameters that we may one day
    // implement. Be warned that this function will reset them to default
    // settings
    // this->bulletBall->setLimit(-_angle.Radian(),
    //                         this->bulletBall->getUpperLimit());
    gzerr << "BulletBallJoint limits not implemented" << std::endl;
    return false;
  }
  else
  {
    gzerr << "bulletBall does not yet exist" << std::endl;
    return false;
  }
}

//////////////////////////////////////////////////
math::Vector3 BulletBallJoint::GetAxis(unsigned int /*_index*/) const
{
  return math::Vector3();
}

//////////////////////////////////////////////////
void BulletBallJoint::SetAxis(unsigned int /*_index*/,
                        const math::Vector3 &/*_axis*/)
{
  gzerr << "BulletBallJoint::SetAxis not implemented" << std::endl;
}

//////////////////////////////////////////////////
math::Angle BulletBallJoint::GetHighStop(unsigned int /*_index*/)
{
  gzerr << "BulletBallJoint::GetHighStop not implemented" << std::endl;
  return math::Angle();
}

//////////////////////////////////////////////////
math::Angle BulletBallJoint::GetLowStop(unsigned int /*_index*/)
{
  gzerr << "BulletBallJoint::GetLowStop not implemented" << std::endl;
  return math::Angle();
}
