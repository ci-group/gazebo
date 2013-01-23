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
/* Desc: Bullet motion state class.
 * Author: Nate Koenig
 * Date: 25 May 2009
 */

#include "physics/Link.hh"
#include "physics/bullet/BulletPhysics.hh"
#include "physics/bullet/BulletMotionState.hh"
#include "physics/bullet/BulletTypes.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
BulletMotionState::BulletMotionState(LinkPtr _link)
  : btMotionState()
{
  this->link = _link;
}

//////////////////////////////////////////////////
BulletMotionState::~BulletMotionState()
{
}

// //////////////////////////////////////////////////
// math::Pose BulletMotionState::GetWorldPose() const
// {
//   return this->worldPose;
// }

// //////////////////////////////////////////////////
// void BulletMotionState::SetWorldPosition(const math::Vector3 &_pos)
// {
//   this->worldPose.pos = _pos;
// }

// //////////////////////////////////////////////////
// void BulletMotionState::SetWorldRotation(const math::Quaternion &_rot)
// {
//   this->worldPose.rot = _rot;
// }

// //////////////////////////////////////////////////
// void BulletMotionState::SetWorldPose(const math::Pose &_pose)
// {
//   this->worldPose = _pose;
// }

// //////////////////////////////////////////////////
// void BulletMotionState::SetCoG(const math::Vector3 &_cog)
// {
//   this->cog = _cog;
//   math::Vector3 cg = this->worldPose.rot.RotateVector(this->cog);
//   this->worldPose.pos += cg;
// }

//////////////////////////////////////////////////
void BulletMotionState::getWorldTransform(btTransform &_cogWorldTrans) const
{
  _cogWorldTrans = BulletTypes::ConvertPose(this->link->GetWorldCoGPose());
}

//////////////////////////////////////////////////
void BulletMotionState::setWorldTransform(const btTransform &_cogWorldTrans)
{
  math::Pose pose = BulletTypes::ConvertPose(_cogWorldTrans);

  math::Vector3 cg = pose.rot.RotateVector(this->link->GetInertial()->GetCoG());
  pose.pos -= cg;

  // The second argument is set to false to prevent Entity.cc from propagating
  // the pose change all the way back to bullet.
  // \TODO: consider using the dirtyPose mechanism employed by ODE.
  this->link->SetWorldPose(pose, false);
}
