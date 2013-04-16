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
/*
 * Desc: Factory for creating physics engine
 * Author: Nate Koenig
 * Date: 21 May 2009
 */

#include "physics/World.hh"
#include "physics/PhysicsEngine.hh"
#include "physics/PhysicsFactory.hh"
#include "common/Console.hh"
#include "gazebo_config.h"

void RegisterODEPhysics();

#ifdef HAVE_BULLET
  void RegisterBulletPhysics();
#endif

using namespace gazebo;
using namespace physics;


std::map<std::string, PhysicsFactoryFn> PhysicsFactory::engines;

//////////////////////////////////////////////////
void PhysicsFactory::RegisterAll()
{
  RegisterODEPhysics();

#ifdef HAVE_BULLET
  RegisterBulletPhysics();
#endif
}

//////////////////////////////////////////////////
void PhysicsFactory::RegisterPhysicsEngine(std::string classname,
                                     PhysicsFactoryFn factoryfn)
{
  engines[classname] = factoryfn;
}

//////////////////////////////////////////////////
PhysicsEnginePtr PhysicsFactory::NewPhysicsEngine(const std::string &_classname,
    WorldPtr _world)
{
  PhysicsEnginePtr result;

  std::map<std::string, PhysicsFactoryFn>::iterator iter =
    engines.find(_classname);
  if (iter != engines.end())
    result = (iter->second)(_world);
  else
    gzerr << "Invalid Physics Type[" << _classname << "]\n";

  return result;
}

//////////////////////////////////////////////////
bool PhysicsFactory::IsRegistered(const std::string &_name)
{
  return (engines.count(_name) > 0);
}
