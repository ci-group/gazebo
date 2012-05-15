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
/* Desc: Collision class
 * Author: Nate Koenig
 * Date: 13 Feb 2006
 */

#include <sstream>

#include "msgs/msgs.h"
#include "msgs/MessageTypes.hh"

#include "common/Events.hh"
#include "common/Console.hh"

#include "transport/Publisher.hh"

#include "physics/Contact.hh"
#include "physics/Shape.hh"
#include "physics/BoxShape.hh"
#include "physics/CylinderShape.hh"
#include "physics/TrimeshShape.hh"
#include "physics/SphereShape.hh"
#include "physics/HeightmapShape.hh"
#include "physics/SurfaceParams.hh"
#include "physics/Model.hh"
#include "physics/Link.hh"
#include "physics/Collision.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
Collision::Collision(LinkPtr _link)
    : Entity(_link)
{
  this->AddType(Base::COLLISION);

  this->link = _link;

  this->transparency = 0;
  this->contactsEnabled = false;

  this->surface.reset(new SurfaceParams());
  this->inertial.reset(new Inertial());
}

//////////////////////////////////////////////////
Collision::~Collision()
{
}

//////////////////////////////////////////////////
void Collision::Fini()
{
  msgs::Request *msg = msgs::CreateRequest("entity_delete",
      this->GetScopedName()+"__COLLISION_VISUAL__");
  this->requestPub->Publish(*msg, true);

  Entity::Fini();
  this->link.reset();
  this->shape.reset();
  this->surface.reset();
  this->connections.clear();
}

//////////////////////////////////////////////////
void Collision::Load(sdf::ElementPtr _sdf)
{
  Entity::Load(_sdf);

  this->SetRelativePose(
    this->sdf->GetOrCreateElement("origin")->GetValuePose("pose"));

  this->surface->Load(this->sdf->GetOrCreateElement("surface"));


  if (this->shape)
    this->shape->Load(this->sdf->GetElement("geometry")->GetFirstElement());
  else
    gzwarn << "No shape has been specified. Error!!!\n";

  if (!this->shape->HasType(Base::MULTIRAY_SHAPE) &&
      !this->shape->HasType(Base::RAY_SHAPE))
  {
    this->visPub->Publish(this->CreateCollisionVisual());
  }

  // Force max correcting velocity to zero for certain collision entities
  if (this->IsStatic() || this->shape->HasType(Base::HEIGHTMAP_SHAPE) ||
      this->shape->HasType(Base::MAP_SHAPE))
  {
    this->surface->maxVel = 0.0;
  }

  this->inertial->SetCoG(this->GetRelativePose().pos);

  // Get the mass of the shape
  double mass = 0.0;
  if (this->sdf->HasElement("mass"))
    mass = this->sdf->GetValueDouble("mass");
  else if (this->sdf->HasElement("density"))
    mass = this->shape->GetMass(this->sdf->HasElement("density"));

  this->shape->GetInertial(mass, this->inertial);
}

//////////////////////////////////////////////////
void Collision::Init()
{
  this->SetRelativePose(
    this->sdf->GetOrCreateElement("origin")->GetValuePose("pose"));

  this->shape->Init();
}

//////////////////////////////////////////////////
void Collision::SetCollision(bool _placeable)
{
  this->placeable = _placeable;

  if (this->IsStatic())
  {
    this->SetCategoryBits(GZ_FIXED_COLLIDE);
    this->SetCollideBits(~GZ_FIXED_COLLIDE);
  }
  else
  {
    // collide with all
    this->SetCategoryBits(GZ_ALL_COLLIDE);
    this->SetCollideBits(GZ_ALL_COLLIDE);
  }
}

//////////////////////////////////////////////////
bool Collision::IsPlaceable() const
{
  return this->placeable;
}


//////////////////////////////////////////////////
void Collision::SetLaserRetro(float _retro)
{
  this->sdf->GetElement("laser_retro")->Set(_retro);
}

//////////////////////////////////////////////////
float Collision::GetLaserRetro() const
{
  return this->sdf->GetValueDouble("laser_retro");
}

//////////////////////////////////////////////////
LinkPtr Collision::GetLink() const
{
  return this->link;
}

//////////////////////////////////////////////////
ModelPtr Collision::GetModel() const
{
  return this->link->GetModel();
}

//////////////////////////////////////////////////
unsigned int Collision::GetShapeType()
{
  return this->shape->GetType();
}

//////////////////////////////////////////////////
void Collision::SetShape(ShapePtr _shape)
{
  this->shape = _shape;
}

//////////////////////////////////////////////////
ShapePtr Collision::GetShape() const
{
  return this->shape;
}

//////////////////////////////////////////////////
void Collision::SetContactsEnabled(bool _enable)
{
  this->contactsEnabled = _enable;
}

//////////////////////////////////////////////////
bool Collision::GetContactsEnabled() const
{
  return this->contactsEnabled;
}

//////////////////////////////////////////////////
void Collision::AddContact(const Contact &_contact)
{
  if (!this->GetContactsEnabled() ||
      this->HasType(Base::RAY_SHAPE) ||
      this->HasType(Base::PLANE_SHAPE))
    return;

  this->contact(this->GetScopedName(), _contact);
}

//////////////////////////////////////////////////
math::Vector3 Collision::GetRelativeLinearVel() const
{
  if (this->link)
    return this->link->GetRelativeLinearVel();
  else
    return math::Vector3();
}

//////////////////////////////////////////////////
math::Vector3 Collision::GetWorldLinearVel() const
{
  if (this->link)
    return this->link->GetWorldLinearVel();
  else
    return math::Vector3();
}

//////////////////////////////////////////////////
math::Vector3 Collision::GetRelativeAngularVel() const
{
  if (this->link)
    return this->link->GetRelativeAngularVel();
  else
    return math::Vector3();
}

//////////////////////////////////////////////////
math::Vector3 Collision::GetWorldAngularVel() const
{
  if (this->link)
    return this->link->GetWorldAngularVel();
  else
    return math::Vector3();
}

//////////////////////////////////////////////////
math::Vector3 Collision::GetRelativeLinearAccel() const
{
  if (this->link)
    return this->link->GetRelativeLinearAccel();
  else
    return math::Vector3();
}

//////////////////////////////////////////////////
math::Vector3 Collision::GetWorldLinearAccel() const
{
  if (this->link)
    return this->link->GetWorldLinearAccel();
  else
    return math::Vector3();
}

//////////////////////////////////////////////////
math::Vector3 Collision::GetRelativeAngularAccel() const
{
  if (this->link)
    return this->link->GetRelativeAngularAccel();
  else
    return math::Vector3();
}

//////////////////////////////////////////////////
math::Vector3 Collision::GetWorldAngularAccel() const
{
  if (this->link)
    return this->link->GetWorldAngularAccel();
  else
    return math::Vector3();
}

//////////////////////////////////////////////////
void Collision::UpdateParameters(sdf::ElementPtr _sdf)
{
  Entity::UpdateParameters(_sdf);
}

//////////////////////////////////////////////////
void Collision::FillCollisionMsg(msgs::Collision &_msg)
{
  msgs::Set(_msg.mutable_pose(), this->GetRelativePose());
  _msg.set_id(this->GetId());
  _msg.set_name(this->GetScopedName());
  _msg.set_laser_retro(this->GetLaserRetro());
  this->shape->FillShapeMsg(*_msg.mutable_geometry());
  this->surface->FillSurfaceMsg(*_msg.mutable_surface());

  msgs::Set(this->visualMsg->mutable_pose(), this->GetRelativePose());
  _msg.add_visual()->CopyFrom(*this->visualMsg);
  _msg.add_visual()->CopyFrom(this->CreateCollisionVisual());
}

//////////////////////////////////////////////////
void Collision::ProcessMsg(const msgs::Collision &_msg)
{
  if (_msg.id() != this->GetId())
  {
    gzerr << "Incorrect ID\n";
    return;
  }

  this->SetName(_msg.name());
  if (_msg.has_laser_retro())
    this->SetLaserRetro(_msg.laser_retro());

  if (_msg.has_pose())
  {
    this->link->SetEnabled(true);
    this->SetRelativePose(msgs::Convert(_msg.pose()));
  }

  if (_msg.has_geometry())
  {
    this->link->SetEnabled(true);
    this->shape->ProcessMsg(_msg.geometry());
  }

  if (_msg.has_surface())
  {
    this->link->SetEnabled(true);
    this->surface->ProcessMsg(_msg.surface());
  }
}

/////////////////////////////////////////////////
msgs::Visual Collision::CreateCollisionVisual()
{
  msgs::Visual msg;
  msg.set_name(this->GetScopedName()+"__COLLISION_VISUAL__");
  msg.set_parent_name(this->parent->GetScopedName());
  msg.set_is_static(this->IsStatic());
  msg.set_cast_shadows(false);
  msgs::Set(msg.mutable_pose(), this->GetRelativePose());
  msg.mutable_material()->set_script("Gazebo/OrangeTransparent");
  msgs::Geometry *geom = msg.mutable_geometry();

  if (this->shape->HasType(BOX_SHAPE))
  {
    BoxShape *box = static_cast<BoxShape*>(this->shape.get());
    geom->set_type(msgs::Geometry::BOX);
    math::Vector3 size = box->GetSize();
    msgs::Set(geom->mutable_box()->mutable_size(), size);
  }
  else if (this->shape->HasType(CYLINDER_SHAPE))
  {
    CylinderShape *cyl = static_cast<CylinderShape*>(this->shape.get());
    msg.mutable_geometry()->set_type(msgs::Geometry::CYLINDER);
    geom->mutable_cylinder()->set_radius(cyl->GetRadius());
    geom->mutable_cylinder()->set_length(cyl->GetLength());
  }

  else if (this->shape->HasType(SPHERE_SHAPE))
  {
    SphereShape *sph = static_cast<SphereShape*>(this->shape.get());
    msg.mutable_geometry()->set_type(msgs::Geometry::SPHERE);
    geom->mutable_sphere()->set_radius(sph->GetRadius());
  }

  else if (this->shape->HasType(HEIGHTMAP_SHAPE))
  {
    HeightmapShape *hgt = static_cast<HeightmapShape*>(this->shape.get());
    geom->set_type(msgs::Geometry::HEIGHTMAP);

    msgs::Set(geom->mutable_heightmap()->mutable_image(),
              common::Image(hgt->GetFilename()));
    msgs::Set(geom->mutable_heightmap()->mutable_size(), hgt->GetSize());
    msgs::Set(geom->mutable_heightmap()->mutable_origin(), hgt->GetOrigin());
  }

  else if (this->shape->HasType(MAP_SHAPE))
  {
    msg.mutable_geometry()->set_type(msgs::Geometry::IMAGE);
  }

  else if (this->shape->HasType(PLANE_SHAPE))
  {
    msg.mutable_geometry()->set_type(msgs::Geometry::PLANE);
  }
  else if (this->shape->HasType(TRIMESH_SHAPE))
  {
    TrimeshShape *msh = static_cast<TrimeshShape*>(this->shape.get());
    msg.mutable_geometry()->set_type(msgs::Geometry::MESH);
    math::Vector3 size = msh->GetSize();
    msgs::Set(geom->mutable_mesh()->mutable_scale(), size);
    geom->mutable_mesh()->set_filename(msh->GetFilename());
  }
  else
    gzerr << "Unknown shape[" << this->shape->GetType() << "]\n";


  return msg;
}

/////////////////////////////////////////////////
CollisionState Collision::GetState()
{
  return CollisionState(
      boost::shared_static_cast<Collision>(shared_from_this()));
}

/////////////////////////////////////////////////
void Collision::SetState(const CollisionState &_state)
{
  this->SetWorldPose(_state.GetPose());
}

/////////////////////////////////////////////////
const InertialPtr Collision::GetInertial() const
{
  return this->inertial;
}

/////////////////////////////////////////////////
void Collision::SetInertial(InertialPtr _inertial)
{
  this->inertial = _inertial;
}
