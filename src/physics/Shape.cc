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
#include "physics/Collision.hh"
#include "physics/Shape.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
Shape::Shape(CollisionPtr _p)
  : Base(_p)
{
  this->AddType(Base::SHAPE);
  this->SetName("shape");

  if (_p)
    this->collisionParent = _p;
}

//////////////////////////////////////////////////
Shape::~Shape()
{
  if (this->collisionParent)
    this->collisionParent->SetShape(ShapePtr());
}

//////////////////////////////////////////////////
void Shape::GetInertial(double /*_mass*/, InertialPtr /*_inertial*/) const
{
  return;
}
