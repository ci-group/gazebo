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
/* Desc: A ray
 * Author: Nate Koenig
 * Date: 14 Oct 2009
 */

#include "physics/World.hh"
#include "physics/Link.hh"
#include "physics/ode/ODEPhysics.hh"
#include "physics/ode/ODETypes.hh"
#include "physics/ode/ODECollision.hh"
#include "physics/ode/ODERayShape.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
ODERayShape::ODERayShape(PhysicsEnginePtr _physicsEngine)
  : RayShape(_physicsEngine)
{
  this->SetName("ODE Ray Shape");

  this->physicsEngine =
    boost::shared_static_cast<ODEPhysics>(_physicsEngine);
  this->geomId = dCreateRay(this->physicsEngine->GetSpaceId(), 2.0);
  dGeomSetCategoryBits(this->geomId, GZ_SENSOR_COLLIDE);
  dGeomSetCollideBits(this->geomId, ~GZ_SENSOR_COLLIDE);
  this->collisionParent.reset();
}


//////////////////////////////////////////////////
ODERayShape::ODERayShape(CollisionPtr _parent)
    : RayShape(_parent)
{
  this->SetName("ODE Ray Shape");

  ODECollisionPtr collision =
    boost::shared_static_cast<ODECollision>(this->collisionParent);

  this->physicsEngine = boost::shared_static_cast<ODEPhysics>(
      this->collisionParent->GetWorld()->GetPhysicsEngine());
  this->geomId = dCreateRay(collision->GetSpaceId(), 1.0);

  // Create default ray with unit length
  collision->SetCollision(this->geomId, false);
  collision->SetCategoryBits(GZ_SENSOR_COLLIDE);
  collision->SetCollideBits(~GZ_SENSOR_COLLIDE);
}

//////////////////////////////////////////////////
ODERayShape::~ODERayShape()
{
  dGeomDestroy(this->geomId);
}

//////////////////////////////////////////////////
void ODERayShape::Update()
{
  math::Vector3 dir;

  if (this->collisionParent)
  {
    ODECollisionPtr collision =
      boost::shared_static_cast<ODECollision>(this->collisionParent);

    this->globalStartPos =
      this->collisionParent->GetLink()->GetWorldPose().CoordPositionAdd(
          this->relativeStartPos);

    this->globalEndPos =
      this->collisionParent->GetLink()->GetWorldPose().CoordPositionAdd(
          this->relativeEndPos);
  }

  dir = this->globalEndPos - this->globalStartPos;
  dir.Normalize();

  if (!math::equal(this->contactLen, 0))
  {
    dGeomRaySet(this->geomId,
        this->globalStartPos.x, this->globalStartPos.y, this->globalStartPos.z,
        dir.x, dir.y, dir.z);

    dGeomRaySetLength(this->geomId,
        this->globalStartPos.Distance(this->globalEndPos));
  }
}

//////////////////////////////////////////////////
void ODERayShape::GetIntersection(double &_dist, std::string &_entity)
{
  if (this->physicsEngine)
  {
    Intersection intersection;
    intersection.depth = 1000;

    this->physicsEngine->GetPhysicsUpdateMutex()->lock();

    // Do collision detection
    dSpaceCollide2(this->geomId,
        (dGeomID)(this->physicsEngine->GetSpaceId()),
        &intersection, &UpdateCallback);
    this->physicsEngine->GetPhysicsUpdateMutex()->unlock();

    _dist = intersection.depth;
    _entity = intersection.name;
  }
}

//////////////////////////////////////////////////
void ODERayShape::SetPoints(const math::Vector3 &_posStart,
                            const math::Vector3 &_posEnd)
{
  math::Vector3 dir;
  RayShape::SetPoints(_posStart, _posEnd);

  dir = this->globalEndPos - this->globalStartPos;
  dir.Normalize();

  dGeomRaySet(this->geomId, this->globalStartPos.x,
              this->globalStartPos.y, this->globalStartPos.z,
              dir.x, dir.y, dir.z);

  dGeomRaySetLength(this->geomId,
                     this->globalStartPos.Distance(this->globalEndPos));
}

//////////////////////////////////////////////////
void ODERayShape::UpdateCallback(void *_data, dGeomID _o1, dGeomID _o2)
{
  dContactGeom contact;
  ODERayShape::Intersection *inter = NULL;

  inter = static_cast<Intersection*>(_data);

  // Check space
  if (dGeomIsSpace(_o1) || dGeomIsSpace(_o2))
  {
    dSpaceCollide2(_o1, _o2, inter, &UpdateCallback);
  }
  else
  {
    ODECollision *collision1, *collision2;
    ODECollision *hitCollision = NULL;

    // Get pointers to the underlying collisions
    if (dGeomGetClass(_o1) == dGeomTransformClass)
      collision1 =
        static_cast<ODECollision*>(dGeomGetData(dGeomTransformGetGeom(_o1)));
    else
      collision1 = static_cast<ODECollision*>(dGeomGetData(_o1));

    if (dGeomGetClass(_o2) == dGeomTransformClass)
      collision2 =
        static_cast<ODECollision*>(dGeomGetData(dGeomTransformGetGeom(_o2)));
    else
      collision2 = static_cast<ODECollision*>(dGeomGetData(_o2));

    // Figure out which one is a ray; note that this assumes
    // that the ODE dRayClass is used *soley* by the RayCollision.
    if (dGeomGetClass(_o1) == dRayClass)
    {
      hitCollision = collision2;
      dGeomRaySetParams(_o1, 0, 0);
      dGeomRaySetClosestHit(_o1, 1);
    }
    else if (dGeomGetClass(_o2) == dRayClass)
    {
      hitCollision = collision1;
      dGeomRaySetParams(_o2, 0, 0);
      dGeomRaySetClosestHit(_o2, 1);
    }

    if (hitCollision)
    {
      // Check for ray/collision intersections
      int n = dCollide(_o1, _o2, 1, &contact, sizeof(contact));

      if (n > 0)
      {
        if (contact.depth < inter->depth)
        {
          inter->depth = contact.depth;
          inter->name = hitCollision->GetName();
        }
      }
    }
  }
}
