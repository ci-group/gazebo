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

#include <sstream>

#include "gazebo/common/Console.hh"
#include "gazebo/math/Box.hh"

#include "gazebo/physics/dart/dart_inc.h"
#include "gazebo/physics/dart/DARTLink.hh"
#include "gazebo/physics/dart/DARTCollision.hh"
#include "gazebo/physics/dart/DARTUtils.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
DARTCollision::DARTCollision(LinkPtr _link)
  : Collision(_link),
    dtBodyNode(NULL),
    dtCollisionShape(NULL)
{
  this->SetName("DART_Collision");
}

//////////////////////////////////////////////////
DARTCollision::~DARTCollision()
{
}

//////////////////////////////////////////////////
void DARTCollision::Load(sdf::ElementPtr _sdf)
{
  Collision::Load(_sdf);

  if (this->IsStatic())
  {
    this->SetCategoryBits(GZ_FIXED_COLLIDE);
    this->SetCollideBits(~GZ_FIXED_COLLIDE);
  }

  this->dtBodyNode
      = boost::shared_static_cast<DARTLink>(this->link)->GetBodyNode();
}

//////////////////////////////////////////////////
void DARTCollision::Init()
{
  Collision::Init();

  // Offset
  math::Pose relativePose = this->GetRelativePose();
  this->dtCollisionShape->setOffset(DARTTypes::ConvVec3(relativePose.pos));
}

//////////////////////////////////////////////////
void DARTCollision::Fini()
{
  Collision::Fini();
}

//////////////////////////////////////////////////
void DARTCollision::OnPoseChange()
{
  // Nothing to do in dart.
}

//////////////////////////////////////////////////
void DARTCollision::SetCategoryBits(unsigned int _bits)
{
  this->categoryBits = _bits;
}

//////////////////////////////////////////////////
void DARTCollision::SetCollideBits(unsigned int _bits)
{
  this->collideBits = _bits;
}

//////////////////////////////////////////////////
unsigned int DARTCollision::GetCategoryBits() const
{
  return this->categoryBits;
}

//////////////////////////////////////////////////
unsigned int DARTCollision::GetCollideBits() const
{
  return this->collideBits;
}

//////////////////////////////////////////////////
gazebo::math::Box DARTCollision::GetBoundingBox() const
{
  math::Box result;

  gzerr << "DART does not provide bounding box info.\n";

  return result;
}

//////////////////////////////////////////////////
dart::dynamics::BodyNode*DARTCollision::GetDARTBodyNode() const
{
  return dtBodyNode;
}

//////////////////////////////////////////////////
void DARTCollision::SetDARTCollisionShape(dart::dynamics::Shape* _shape, bool _placeable)
{
  Collision::SetCollision(_placeable);
  this->dtCollisionShape = _shape;
}

//////////////////////////////////////////////////
dart::dynamics::Shape* DARTCollision::GetDARTCollisionShape() const
{
  return dtCollisionShape;
}
