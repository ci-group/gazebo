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
/* Desc: ODECollision class
 * Author: Nate Koenig
 * Date: 13 Feb 2006
 */

#include <sstream>

#include "common/Console.hh"
#include "math/Box.hh"

#include "physics/SurfaceParams.hh"
#include "physics/ode/ODEPhysics.hh"
#include "physics/ode/ODELink.hh"
#include "physics/ode/ODECollision.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
ODECollision::ODECollision(LinkPtr _link)
: Collision(_link)
{
  this->SetName("ODE_Collision");
  this->collisionId = NULL;
  this->onPoseChangeFunc = &ODECollision::OnPoseChangeNull;
}

//////////////////////////////////////////////////
ODECollision::~ODECollision()
{
  if (this->collisionId)
    dGeomDestroy(this->collisionId);
  this->collisionId = NULL;
}

//////////////////////////////////////////////////
void ODECollision::Load(sdf::ElementPtr _sdf)
{
  Collision::Load(_sdf);

  this->SetSpaceId(
      boost::shared_static_cast<ODELink>(this->link)->GetSpaceId());

  if (this->IsStatic())
  {
    this->SetCategoryBits(GZ_FIXED_COLLIDE);
    this->SetCollideBits(~GZ_FIXED_COLLIDE);
  }
}

//////////////////////////////////////////////////
void ODECollision::Fini()
{
  /*
     if (this->collisionId)
     dGeomDestroy(this->collisionId);
     this->collisionId = NULL;

     if (this->spaceId)
     dSpaceDestroy(this->spaceId);
     this->spaceId = NULL;
     */

  Collision::Fini();
}

//////////////////////////////////////////////////
void ODECollision::OnPoseChange()
{
  // Update all the models
  // (*this.*onPoseChangeFunc)();

  if (this->IsStatic() && this->collisionId && this->placeable)
    this->OnPoseChangeGlobal();
  else if (this->collisionId && this->placeable)
    this->OnPoseChangeRelative();
}

//////////////////////////////////////////////////
void ODECollision::SetCollision(dGeomID _collisionId, bool _placeable)
{
  // Must go first in this function
  this->collisionId = _collisionId;

  Collision::SetCollision(_placeable);

  if (dGeomGetSpace(this->collisionId) == 0)
  {
    dSpaceAdd(this->spaceId, this->collisionId);
    assert(dGeomGetSpace(this->collisionId) != 0);
  }

  if (this->collisionId && this->placeable)
  {
    if (this->IsStatic())
      this->onPoseChangeFunc = &ODECollision::OnPoseChangeGlobal;
    else
      this->onPoseChangeFunc = &ODECollision::OnPoseChangeRelative;
  }
  else
  {
    this->onPoseChangeFunc = &ODECollision::OnPoseChangeNull;
  }

  dGeomSetData(this->collisionId, this);
}

//////////////////////////////////////////////////
dGeomID ODECollision::GetCollisionId() const
{
  return this->collisionId;
}

//////////////////////////////////////////////////
int ODECollision::GetCollisionClass() const
{
  int result = 0;

  if (this->collisionId)
  {
    result = dGeomGetClass(this->collisionId);
  }

  return result;
}

//////////////////////////////////////////////////
void ODECollision::SetCategoryBits(unsigned int _bits)
{
  if (this->collisionId)
    dGeomSetCategoryBits(this->collisionId, _bits);
  if (this->spaceId)
    dGeomSetCategoryBits((dGeomID)this->spaceId, _bits);
}

//////////////////////////////////////////////////
void ODECollision::SetCollideBits(unsigned int _bits)
{
  if (this->collisionId)
    dGeomSetCollideBits(this->collisionId, _bits);
  if (this->spaceId)
    dGeomSetCollideBits((dGeomID)this->spaceId, _bits);
}

//////////////////////////////////////////////////
math::Box ODECollision::GetBoundingBox() const
{
  math::Box box;
  dReal aabb[6];

  memset(aabb, 0, 6 * sizeof(dReal));

  // if (this->collisionId && this->type != Shape::PLANE)
  dGeomGetAABB(this->collisionId, aabb);

  box.min.Set(aabb[0], aabb[2], aabb[4]);
  box.max.Set(aabb[1], aabb[3], aabb[5]);

  return box;
}

//////////////////////////////////////////////////
dSpaceID ODECollision::GetSpaceId() const
{
  return this->spaceId;
}

//////////////////////////////////////////////////
void ODECollision::SetSpaceId(dSpaceID _spaceid)
{
  this->spaceId = _spaceid;
}

/////////////////////////////////////////////////
void ODECollision::OnPoseChangeGlobal()
{
  dQuaternion q;

  // Transform into global pose since a static collision does not have a link
  math::Pose localPose = this->GetWorldPose();

  // un-offset cog location
  math::Vector3 cog_vec = this->link->GetInertial()->GetCoG();
  localPose.pos = localPose.pos - cog_vec;

  q[0] = localPose.rot.w;
  q[1] = localPose.rot.x;
  q[2] = localPose.rot.y;
  q[3] = localPose.rot.z;

  dGeomSetPosition(this->collisionId, localPose.pos.x, localPose.pos.y,
      localPose.pos.z);
  dGeomSetQuaternion(this->collisionId, q);
}

/////////////////////////////////////////////////
void ODECollision::OnPoseChangeRelative()
{
  dQuaternion q;
  // Transform into CoM relative Pose
  math::Pose localPose = this->GetRelativePose();

  // un-offset cog location
  math::Vector3 cog_vec = this->link->GetInertial()->GetCoG();
  localPose.pos = localPose.pos - cog_vec;

  q[0] = localPose.rot.w;
  q[1] = localPose.rot.x;
  q[2] = localPose.rot.y;
  q[3] = localPose.rot.z;

  // Set the pose of the encapsulated collision; this is always relative
  // to the CoM
  dGeomSetOffsetPosition(this->collisionId, localPose.pos.x, localPose.pos.y,
      localPose.pos.z);
  dGeomSetOffsetQuaternion(this->collisionId, q);
}

/////////////////////////////////////////////////
void ODECollision::OnPoseChangeNull()
{
}
