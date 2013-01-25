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
/* Desc: The base class for all physics engines
 * Author: Nate Koenig
 */

#include "sdf/sdf.hh"

#include "gazebo/msgs/msgs.hh"
#include "gazebo/common/Exception.hh"
#include "gazebo/common/Console.hh"
#include "gazebo/common/Events.hh"

#include "gazebo/transport/Transport.hh"
#include "gazebo/transport/Node.hh"

#include "gazebo/math/Rand.hh"

#include "gazebo/physics/ContactManager.hh"
#include "gazebo/physics/Link.hh"
#include "gazebo/physics/World.hh"
#include "gazebo/physics/PhysicsEngine.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
PhysicsEngine::PhysicsEngine(WorldPtr _world)
  : world(_world)
{
  this->sdf.reset(new sdf::Element);
  sdf::initFile("physics.sdf", this->sdf);

  this->node = transport::NodePtr(new transport::Node());
  this->node->Init(this->world->GetName());
  this->physicsSub = this->node->Subscribe("~/physics",
      &PhysicsEngine::OnPhysicsMsg, this);

  this->responsePub =
    this->node->Advertise<msgs::Response>("~/response");

  this->requestSub = this->node->Subscribe("~/request",
                                           &PhysicsEngine::OnRequest, this);

  this->physicsUpdateMutex = new boost::recursive_mutex();

//  this->updateRateDouble = 0.0;

  // Create and initialized the contact manager.
  this->contactManager = new ContactManager();
  this->contactManager->Init(this->world);

  this->SetSeed(math::Rand::GetSeed());
}

//////////////////////////////////////////////////
void PhysicsEngine::Load(sdf::ElementPtr _sdf)
{
  this->sdf->Copy(_sdf);
//  if (this->sdf->HasElement("update_rate"))
//    this->SetUpdateRate(this->sdf->GetValueDouble("update_rate"));
}

//////////////////////////////////////////////////
void PhysicsEngine::Fini()
{
  this->world.reset();
  this->node->Fini();
}

//////////////////////////////////////////////////
PhysicsEngine::~PhysicsEngine()
{
  this->sdf->Reset();
  this->sdf.reset();
  delete this->physicsUpdateMutex;
  this->physicsUpdateMutex = NULL;
  this->responsePub.reset();
  this->requestSub.reset();
  this->node.reset();

  delete this->contactManager;
}

//////////////////////////////////////////////////
math::Vector3 PhysicsEngine::GetGravity() const
{
  return this->sdf->GetValueVector3("gravity");
}

//////////////////////////////////////////////////
CollisionPtr PhysicsEngine::CreateCollision(const std::string &_shapeType,
                                            const std::string &_linkName)
{
  CollisionPtr result;
  LinkPtr link =
    boost::shared_dynamic_cast<Link>(this->world->GetEntity(_linkName));

  if (!link)
    gzerr << "Unable to find link[" << _linkName << "]\n";
  else
    result = this->CreateCollision(_shapeType, link);

  return result;
}

//////////////////////////////////////////////////
void PhysicsEngine::SetUpdateRate(double _value)
{
  this->world->SetRealTimeUpdateRate(_value);
}

//////////////////////////////////////////////////
double PhysicsEngine::GetUpdateRate()
{
  return this->world->GetRealTimeUpdateRate();
}

//////////////////////////////////////////////////
double PhysicsEngine::GetUpdatePeriod()
{
  double updateRate = this->world->GetRealTimeUpdateRate();
  if (updateRate > 0)
    return 1.0/updateRate;
  else
    return 0;
}

//////////////////////////////////////////////////
void PhysicsEngine::SetStepTime(double _value)
{
  this->world->SetMaxStepSize(_value);
}

//////////////////////////////////////////////////
double PhysicsEngine::GetStepTime()
{
  return this->world->GetMaxStepSize();
}

//////////////////////////////////////////////////
void PhysicsEngine::SetWorldCFM(double /*_cfm*/)
{
}

//////////////////////////////////////////////////
void PhysicsEngine::SetWorldERP(double /*_erp*/)
{
}

//////////////////////////////////////////////////
void PhysicsEngine::SetAutoDisableFlag(bool /*_autoDisable*/)
{
}

//////////////////////////////////////////////////
void PhysicsEngine::SetSORPGSPreconIters(unsigned int /*_iters*/)
{
}

//////////////////////////////////////////////////
void PhysicsEngine::SetSORPGSIters(unsigned int /*_iters*/)
{
}

//////////////////////////////////////////////////
void PhysicsEngine::SetSORPGSW(double /*_w*/)
{
}

//////////////////////////////////////////////////
void PhysicsEngine::SetContactMaxCorrectingVel(double /*_vel*/)
{
}

//////////////////////////////////////////////////
void PhysicsEngine::SetMaxContacts(double /*_maxContacts*/)
{
}

//////////////////////////////////////////////////
void PhysicsEngine::OnRequest(ConstRequestPtr &/*_msg*/)
{
}

//////////////////////////////////////////////////
void PhysicsEngine::OnPhysicsMsg(ConstPhysicsPtr &/*_msg*/)
{
}

//////////////////////////////////////////////////
void PhysicsEngine::SetContactSurfaceLayer(double /*_layerDepth*/)
{
}

//////////////////////////////////////////////////
ContactManager *PhysicsEngine::GetContactManager() const
{
  return this->contactManager;
}

//////////////////////////////////////////////////
void PhysicsEngine::SetSeed(uint32_t /*_seed*/)
{
}
