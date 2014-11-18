/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#include "SimEventsPlugin.hh"

using namespace gazebo;
using namespace sdf;
using namespace physics;

////////////////////////////////////////////////////////////////////////////////
void SimEventsPlugin::OnModelInfo(ConstModelPtr &_msg)
{
  std::string modelName = _msg->name();
  // only if the model is not already in the set...
  if (models.insert(modelName).second)
  {
    // notify everyone!
    SimEventsEvents::spawnModel(modelName, true);
  }
}

////////////////////////////////////////////////////////////////////////////////
SimEventsPlugin::~SimEventsPlugin()
{
  this->events.clear();
}

////////////////////////////////////////////////////////////////////////////////
void SimEventsPlugin::OnRequest(ConstRequestPtr &_msg)
{
  if (_msg->request() == "entity_delete")
  {
    std::string modelName = _msg->data();
    if (models.erase(modelName) == 1)
    {
      // notify everyone!
      SimEventsEvents::spawnModel(modelName, false);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
void SimEventsPlugin::Load(physics::WorldPtr _parent, sdf::ElementPtr _sdf)
{
  this->world = _parent;
  this->sdf = _sdf;

  // Create a new transport node
  transport::NodePtr node(new transport::Node());
  // Initialize the node with the world name
  node->Init(_parent->GetName());
  // Create a publisher on the Rest plugin topic
  pub = node->Advertise<gazebo::msgs::SimEvent>("/gazebo/sim_events");
  // Subscribe to model spawning
  spawnSub = node->Subscribe("~/model/info", &SimEventsPlugin::OnModelInfo,
                             this);
  // detect model deletion
  requestSub = node->Subscribe("~/request", &SimEventsPlugin::OnRequest, this);

  // regions are defined outside of events, so that they can be shared
  // between events....
  // and we read them first
  sdf::ElementPtr child = this->sdf->GetElement("region");
  while (child)
  {
    Region* r = new Region;
    r->Load(child);
    RegionPtr region;
    region.reset(r);

    this->regions[region->name] = region;
    child = child->GetNextElement("region");
  }

  // Reading events
  child = this->sdf->GetElement("event");
  while (child)
  { 
    // get name and type of each event
    std::string eventName = child->GetElement("name")->Get<std::string>();
    std::string eventType = child->GetElement("type")->Get<std::string>();

    // this is more or less a factory for event loading, dispatching
    // the correct ctor based on the type of the event
    EventSourcePtr event;
    if (eventType == "sim_state")
    {
      event.reset(new SimStateEventSource(this->pub, this->world));
    }
    else if (eventType == "inclusion")
    {
      event.reset(new InRegionEventSource(this->pub, this->world,
                                          this->regions));
    }
    else if (eventType == "existence" )
    {
      event.reset(new ExistenceEventSource(this->pub, this->world)); 
    }
    else
    {
      std::string m;
      m = "Unknown event type: \"" + eventType + "\" in scoring plugin"; 
      throw SimEventsException(m.c_str());
    }
    if (event)
    {
      event->Load(child);
      events.push_back(event);
    }
    child = child->GetNextElement("event");
  }
  // Sim events are now loaded
}

////////////////////////////////////////////////////////////////////////////////
void SimEventsPlugin::Init()
{
  for (unsigned int i = 0; i < events.size(); ++i)
  {
    events[i]->Init();
  }
  // seed the map with the initial models
  for (unsigned int i=0; i < world->GetModelCount(); ++i)
  {
    std::string name = world->GetModel(i)->GetName();
    models.insert(name);
  } 
}

// Register this plugin with the simulator
GZ_REGISTER_WORLD_PLUGIN(SimEventsPlugin)
