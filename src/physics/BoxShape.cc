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

#include "math/Vector3.hh"
#include "BoxShape.hh"

using namespace gazebo;
using namespace physics;


//////////////////////////////////////////////////
BoxShape::BoxShape(CollisionPtr _parent) : Shape(_parent)
{
  this->AddType(Base::BOX_SHAPE);
}

//////////////////////////////////////////////////
BoxShape::~BoxShape()
{
}

//////////////////////////////////////////////////
void BoxShape::Init()
{
  this->SetSize(this->sdf->GetValueVector3("size"));
}

//////////////////////////////////////////////////
void BoxShape::SetSize(const math::Vector3 &_size)
{
  this->sdf->GetAttribute("size")->Set(_size);
}

//////////////////////////////////////////////////
math::Vector3 BoxShape::GetSize() const
{
  return this->sdf->GetValueVector3("size");
}

//////////////////////////////////////////////////
void BoxShape::FillShapeMsg(msgs::Geometry &_msg)
{
  _msg.set_type(msgs::Geometry::BOX);
  msgs::Set(_msg.mutable_box()->mutable_size(), this->GetSize());
}

//////////////////////////////////////////////////
void BoxShape::ProcessMsg(const msgs::Geometry &_msg)
{
  this->SetSize(msgs::Convert(_msg.box().size()));
}

//////////////////////////////////////////////////
double BoxShape::GetMass(double _density) const
{
  math::Vector3 size = this->GetSize();
  return size.x * size.y * size.z * _density;
}

//////////////////////////////////////////////////
void BoxShape::GetInertial(double _mass, InertialPtr _inertial) const
{
  math::Vector3 size = this->GetSize();

  _inertial->SetMass(_mass);
  _inertial->SetIXX(_mass / 12.0 * (size.y * size.y + size.z * size.z));
  _inertial->SetIYY(_mass / 12.0 * (size.x * size.x + size.z * size.z));
  _inertial->SetIZZ(_mass / 12.0 * (size.x * size.x + size.y * size.y));
}
