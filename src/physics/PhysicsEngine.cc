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
/* Desc: The base class for all physics engines
 * Author: Nate Koenig
 * Date: 11 June 2007
 * SVN: $Id$
 */

#include <boost/thread/recursive_mutex.hpp>

#include "common/Messages.hh"
#include "common/GazeboError.hh"
#include "common/GazeboMessage.hh"
#include "common/Events.hh"

#include "transport/TopicManager.hh"

#include "physics/World.hh"
#include "physics/PhysicsEngine.hh"

using namespace gazebo;
using namespace physics;


////////////////////////////////////////////////////////////////////////////////
// Constructor
PhysicsEngine::PhysicsEngine(World *world)
  : world(world)
{
  this->vis_pub = transport::TopicManager::Instance()->Advertise<msgs::Visual>("/gazebo/visual");

  common::Param::Begin(&this->parameters);
  this->gravityP = new common::ParamT<common::Vector3>("gravity",common::Vector3(0.0, -9.80665, 0.0), 0);
  this->gravityP->Callback(&PhysicsEngine::SetGravity, this);

  this->updateRateP = new common::ParamT<double>("update_rate", 0.0, 0);
  this->stepTimeP = new common::ParamT<common::Time>("step_time",0.025,0);
  common::Param::End();

  this->mutex = new boost::recursive_mutex();
  this->locked = false;

  {
    /*this->visualMsg = new VisualMsg();
    this->visualMsg->parentId.clear();
    this->visualMsg->id = "physics_engine_visual";
    this->visualMsg->visible = false;
    //this->visualMsg->rtShader = false;
    this->visualMsg->castShadows = false;
    this->visualMsg->action = VisualMsg::UPDATE;
    */

    //Simulator::Instance()->SendMessage( *this->visualMsg );

    /* NATY: put this back in
    this->contactLines.resize(1);

    Material *mat = new Material();
    mat->SetName("ContactPointsMaterial");
    mat->SetPointSize(10);
    mat->SetAmbient(Color(1,0,0,1));
    mat->SetDiffuse(Color(1,0,0,1));
    mat->SetEmissive(Color(1,0,0,1));
    std::string matName = OgreCreator::CreateMaterial(mat);

    unsigned int i=0;
    for (this->contactLinesIter = this->contactLines.begin();
         this->contactLinesIter != this->contactLines.end(); 
         this->contactLinesIter++, i++)
    {
      (*this->contactLinesIter) = this->visual->AddDynamicLine(RENDERING_LINE_LIST);
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->AddPoint(common::Vector3(0,0,0));
      (*this->contactLinesIter)->setMaterial(matName);
    }
    */

    this->showContactConnection = event::Events::ConnectShowContactsSignal( boost::bind(&PhysicsEngine::ShowContacts, this, _1) );

    // NATY: put this back in
    //this->contactLinesIter = this->contactLines.begin();
    //delete mat;
  }

}

////////////////////////////////////////////////////////////////////////////////
// Destructor
PhysicsEngine::~PhysicsEngine()
{
  if (!this->visual.empty())
  {
    msgs::Visual msg;
    common::Message::Init(msg, this->visual);
    msg.set_action( msgs::Visual::DELETE );
    this->vis_pub->Publish(msg);
  }

  delete this->gravityP;
  delete this->updateRateP;
  delete this->stepTimeP;

  delete this->mutex;
  this->mutex = NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// Update the physics engine
void PhysicsEngine::UpdatePhysics()
{
}

////////////////////////////////////////////////////////////////////////////////
/// Return the gavity vector
common::Vector3 PhysicsEngine::GetGravity() const
{
  return this->gravityP->GetValue();
}

////////////////////////////////////////////////////////////////////////////////
/// Get the time between each update cycle
double PhysicsEngine::GetUpdateRate() const
{
  return this->updateRateP->GetValue();
}

////////////////////////////////////////////////////////////////////////////////
/// Set the time between each update cycle
void PhysicsEngine::SetUpdateRate(double rate) const
{
  this->updateRateP->SetValue(rate);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the time between each update cycle
common::Time PhysicsEngine::GetStepTime() const
{
  return **this->stepTimeP;
}

////////////////////////////////////////////////////////////////////////////////
/// Set the time between each update cycle
void PhysicsEngine::SetStepTime(common::Time time)
{
  this->stepTimeP->SetValue(time);
}

////////////////////////////////////////////////////////////////////////////////
/// Lock the physics engine mutex
void PhysicsEngine::LockMutex()
{
  this->mutex->lock();
  this->locked = true;
}

////////////////////////////////////////////////////////////////////////////////
/// Lock the physics engine mutex
void PhysicsEngine::UnlockMutex()
{
  this->mutex->unlock();
  this->locked = false;
}

////////////////////////////////////////////////////////////////////////////////
/// Add a contact visual
void PhysicsEngine::AddContactVisual(common::Vector3 pos, common::Vector3 norm)
{
  // NATY: put back in
 /* if (!RenderState::GetShowContacts())
    return;

  common::Vector3 e1 = norm.GetPerpendicular(); e1.Normalize();
  common::Vector3 e2 = norm.GetCrossProd(e1); e2.Normalize();

  (*this->contactLinesIter)->SetPoint( 0, pos);
  (*this->contactLinesIter)->SetPoint( 1, pos+(norm*0.2)+(e1*0.05)+(e2*0.05));

  (*this->contactLinesIter)->SetPoint( 2, pos);
  (*this->contactLinesIter)->SetPoint( 3, pos+(norm*0.2)+(e1*0.05)-(e2*0.05));

  (*this->contactLinesIter)->SetPoint( 4, pos);
  (*this->contactLinesIter)->SetPoint( 5, pos+(norm*0.2)-(e1*0.05)+(e2*0.05));

  (*this->contactLinesIter)->SetPoint( 6, pos);
  (*this->contactLinesIter)->SetPoint( 7, pos+(norm*0.2)-(e1*0.05)-(e2*0.05));

  (*this->contactLinesIter)->SetPoint( 8, pos+(norm*0.2)+(e1*0.05)+(e2*0.05));
  (*this->contactLinesIter)->SetPoint( 9, pos+(norm*0.2)-(e1*0.05)+(e2*0.05));

  (*this->contactLinesIter)->SetPoint(10, pos+(norm*0.2)-(e1*0.05)+(e2*0.05));
  (*this->contactLinesIter)->SetPoint(11, pos+(norm*0.2)-(e1*0.05)-(e2*0.05));

  (*this->contactLinesIter)->SetPoint(12, pos+(norm*0.2)-(e1*0.05)-(e2*0.05));
  (*this->contactLinesIter)->SetPoint(13, pos+(norm*0.2)+(e1*0.05)-(e2*0.05));

  (*this->contactLinesIter)->SetPoint(14, pos+(norm*0.2)+(e1*0.05)-(e2*0.05));
  (*this->contactLinesIter)->SetPoint(15, pos+(norm*0.2)+(e1*0.05)+(e2*0.05));

  this->contactLinesIter++;

  if (this->contactLinesIter == this->contactLines.end())
    this->contactLinesIter = this->contactLines.begin();
    */
}

////////////////////////////////////////////////////////////////////////////////
// Set whether to show contacts
void PhysicsEngine::ShowContacts(const bool &show)
{
  msgs::Visual msg;
  common::Message::Init(msg, this->visual);
  msg.set_visible( show );
  this->vis_pub->Publish(msg);

  /* NATY put back in
  if (show)
    this->contactLinesIter = this->contactLines.begin();
    */
}

////////////////////////////////////////////////////////////////////////////////
/// Get the count of the parameters
unsigned int PhysicsEngine::GetParamCount() const
{
  return this->parameters.size();
}

////////////////////////////////////////////////////////////////////////////////
/// Get a param by index
common::Param *PhysicsEngine::GetParam(unsigned int index) const
{
  if (index < this->parameters.size())
    return this->parameters[index];
  else
    gzerr(0) << "Invalid index[" << index << "]\n";
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// Get a parameter by name
common::Param *PhysicsEngine::GetParam(const std::string &key) const
{
  std::vector<common::Param*>::const_iterator iter;
  common::Param *result = NULL;

  for (iter = this->parameters.begin(); iter != this->parameters.end(); iter++)
  {
    if ((*iter)->GetKey() == key)
    {
      result = *iter;
      break;
    }
  }

  if (result == NULL)
    gzerr(0) << "Unable to find common::Param using key[" << key << "]\n";

  return result;

}

////////////////////////////////////////////////////////////////////////////////
/// Set a parameter by name
void PhysicsEngine::SetParam(const std::string &key, const std::string &value)
{
  std::vector<common::Param*>::const_iterator iter;
  common::Param *result = NULL;

  for (iter = this->parameters.begin(); iter != this->parameters.end(); iter++)
  {
    if ((*iter)->GetKey() == key)
    {
      result = *iter;
      break;
    }
  }

  if (result == NULL)
    gzerr(0) << "Unable to find common::Param using key[" << key << "]\n";
  else
    result->SetFromString( value, true );
}
