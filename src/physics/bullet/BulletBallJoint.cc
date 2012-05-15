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
/* Desc: A ball joint
 * Author: Nate Koenig, Andrew Howard
 * Date: 21 May 2003
 */

#include "common/Exception.hh"
#include "common/Console.hh"

#include "physics/bullet/BulletTypes.hh"
#include "physics/bullet/BulletLink.hh"
#include "physics/bullet/BulletBallJoint.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
BulletBallJoint::BulletBallJoint(btDynamicsWorld *_world)
    : BallJoint<BulletJoint>()
{
  this->world = _world;
}

//////////////////////////////////////////////////
BulletBallJoint::~BulletBallJoint()
{
}

//////////////////////////////////////////////////
math::Vector3 BulletBallJoint::GetAnchor(int /*_index*/) const
{
  return this->anchorPos;
}

//////////////////////////////////////////////////
void BulletBallJoint::SetAnchor(int /*_index*/,
                                const math::Vector3 &/*_anchor*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
void BulletBallJoint::SetDamping(int /*_index*/, double /*_damping*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
void BulletBallJoint::Attach(LinkPtr _one, LinkPtr _two)
{
  BallJoint<BulletJoint>::Attach(_one, _two);

  BulletLinkPtr bulletChildLink =
    boost::shared_static_cast<BulletLink>(this->childLink);
  BulletLinkPtr bulletParentLink =
    boost::shared_static_cast<BulletLink>(this->parentLink);

  if (!bulletChildLink || !bulletParentLink)
    gzthrow("Requires bullet bodies");

  math::Vector3 pivotA, pivotB;

  // Compute the pivot point, based on the anchorPos
  pivotA = this->anchorPos - this->parentLink->GetWorldPose().pos;
  pivotB = this->anchorPos - this->childLink->GetWorldPose().pos;

  this->btBall = new btPoint2PointConstraint(
      *bulletParentLink->GetBulletLink(),
      *bulletChildLink->GetBulletLink(),
      btVector3(pivotA.x, pivotA.y, pivotA.z),
      btVector3(pivotB.x, pivotB.y, pivotB.z));

  this->constraint = this->btBall;

  // Add the joint to the world
  this->world->addConstraint(this->constraint);

  // Allows access to impulse
  this->constraint->enableFeedback(true);
}

/////////////////////////////////////////////////
void BulletBallJoint::SetVelocity(int /*_index*/, double /*_angle*/)
{
  gzerr << "Not implemented\n";
}

/////////////////////////////////////////////////
double BulletBallJoint::GetVelocity(int /*_index*/) const
{
  gzerr << "Not implemented\n";
  return 0;
}

/////////////////////////////////////////////////
double BulletBallJoint::GetMaxForce(int /*_index*/)
{
  gzerr << "Not implemented\n";
  return 0;
}

/////////////////////////////////////////////////
void BulletBallJoint::SetMaxForce(int /*_index*/, double /*_t*/)
{
  gzerr << "Not implemented\n";
  return;
}

/////////////////////////////////////////////////
math::Angle BulletBallJoint::GetAngle(int /*_index*/) const
{
  gzerr << "Not implemented\n";
  return 0;
}

/////////////////////////////////////////////////
math::Vector3 BulletBallJoint::GetGlobalAxis(int /*_index*/) const
{
  return math::Vector3();
}

/////////////////////////////////////////////////
math::Angle BulletBallJoint::GetAngleImpl(int /*_index*/) const
{
  return math::Angle();
}
