/*
 * Copyright 2011 Nate Koenig & Andrew Howard
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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
#include "physics/World.hh"
#include "physics/CollisionState.hh"

using namespace gazebo;
using namespace physics;

/////////////////////////////////////////////////
CollisionState::CollisionState()
: State()
{
}

/////////////////////////////////////////////////
CollisionState::CollisionState(const CollisionPtr _collision)
: State(_collision->GetName(), _collision->GetWorld()->GetRealTime(),
        _collision->GetWorld()->GetSimTime())
{
  this->pose = _collision->GetWorldPose();
}

/////////////////////////////////////////////////
CollisionState::~CollisionState()
{
}

/////////////////////////////////////////////////
void CollisionState::Load(sdf::ElementPtr /*_elem*/)
{
}

/////////////////////////////////////////////////
math::Pose CollisionState::GetPose() const
{
  return this->pose;
}
