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
/* Desc: Base class for all sensors
 * Author: Nathan Koenig
 * Date: 25 May 2007
 */

#include "sdf/sdf.h"
#include "transport/transport.h"

#include "physics/Physics.hh"
#include "physics/World.hh"

#include "common/Timer.hh"
#include "common/Console.hh"
#include "common/Exception.hh"
#include "common/Plugin.hh"

#include "sensors/CameraSensor.hh"

#include "sensors/Sensor.hh"
#include "sensors/SensorManager.hh"

using namespace gazebo;
using namespace sensors;

//////////////////////////////////////////////////
Sensor::Sensor()
{
  this->sdf.reset(new sdf::Element);
  sdf::initFile("sdf/sensor.sdf", this->sdf);

  this->active = false;

  this->node = transport::NodePtr(new transport::Node());

  this->updatePeriod = common::Time(0.0);
}

//////////////////////////////////////////////////
Sensor::~Sensor()
{
  this->sdf->Reset();
  this->sdf.reset();
  this->node.reset();
  this->connections.clear();
}

//////////////////////////////////////////////////
void Sensor::Load(const std::string &_worldName, sdf::ElementPtr _sdf)
{
  this->sdf->Copy(_sdf);
  this->Load(_worldName);
}

//////////////////////////////////////////////////
void Sensor::Load(const std::string &_worldName)
{
  if (this->sdf->HasElement("origin"))
  {
    this->pose = this->sdf->GetElement("origin")->GetValuePose("pose");
  }

  if (this->sdf->GetValueBool("always_on"))
    this->SetActive(true);

  this->world = physics::get_world(_worldName);
  this->lastUpdateTime = common::Time(0.0);  // loaded, but not updated

  this->node->Init(this->world->GetName());
  this->sensorPub = this->node->Advertise<msgs::Sensor>("~/sensor");
}

//////////////////////////////////////////////////
void Sensor::Init()
{
  this->SetUpdateRate(this->sdf->GetValueDouble("update_rate"));

  // Load the plugins
  if (this->sdf->HasElement("plugin"))
  {
    sdf::ElementPtr pluginElem = this->sdf->GetElement("plugin");
    while (pluginElem)
    {
      this->LoadPlugin(pluginElem);
      pluginElem = pluginElem->GetNextElement("plugin");
    }
  }

  msgs::Sensor msg;
  this->FillMsg(msg);
  this->sensorPub->Publish(msg);
}

//////////////////////////////////////////////////
void Sensor::SetParent(const std::string &_name)
{
  this->parentName = _name;
}

//////////////////////////////////////////////////
std::string Sensor::GetParentName() const
{
  return this->parentName;
}

//////////////////////////////////////////////////
void Sensor::Update(bool _force)
{
  if (this->IsActive())
  {
    if (this->world->GetSimTime() - this->lastUpdateTime >= this->updatePeriod)
    {
      this->UpdateImpl(_force);
    }
  }
}

//////////////////////////////////////////////////
void Sensor::Fini()
{
  this->plugins.clear();
}

//////////////////////////////////////////////////
std::string Sensor::GetName() const
{
  return this->sdf->GetValueString("name");
}

//////////////////////////////////////////////////
void Sensor::LoadPlugin(sdf::ElementPtr _sdf)
{
  std::string name = _sdf->GetValueString("name");
  std::string filename = _sdf->GetValueString("filename");
  gazebo::SensorPluginPtr plugin = gazebo::SensorPlugin::Create(filename, name);

  if (plugin)
  {
    SensorPtr myself = shared_from_this();
    plugin->Load(myself, _sdf);
    this->plugins.push_back(plugin);
  }
}

//////////////////////////////////////////////////
void Sensor::SetActive(bool _value)
{
  this->active = _value;
}

//////////////////////////////////////////////////
bool Sensor::IsActive()
{
  return this->active;
}

//////////////////////////////////////////////////
math::Pose Sensor::GetPose() const
{
  return this->pose;
}

//////////////////////////////////////////////////
void Sensor::SetUpdateRate(double _hz)
{
  if (_hz > 0.0)
    this->updatePeriod = 1.0/_hz;
  else
    this->updatePeriod = 0.0;
}

//////////////////////////////////////////////////
common::Time Sensor::GetLastUpdateTime()
{
  return this->lastUpdateTime;
}

//////////////////////////////////////////////////
std::string Sensor::GetType() const
{
  return this->sdf->GetValueString("type");
}

//////////////////////////////////////////////////
bool Sensor::GetVisualize() const
{
  return this->sdf->GetValueBool("visualize");
}

//////////////////////////////////////////////////
std::string Sensor::GetTopic() const
{
  std::string result;
  if (this->sdf->HasElement("topic"))
    result = this->sdf->GetElement("topic")->GetValueString();
  return result;
}

//////////////////////////////////////////////////
void Sensor::FillMsg(msgs::Sensor &_msg)
{
  _msg.set_name(this->GetName());
  _msg.set_type(this->GetType());
  _msg.set_parent(this->GetParentName());
  msgs::Set(_msg.mutable_pose(), this->GetPose());

  _msg.set_visualize(this->GetVisualize());
  _msg.set_topic(this->GetTopic());

  if (this->GetType() == "camera")
  {
    CameraSensor *camSensor = static_cast<CameraSensor*>(this);
    msgs::CameraSensor *camMsg = _msg.mutable_camera();
    camMsg->mutable_image_size()->set_x(camSensor->GetImageWidth());
    camMsg->mutable_image_size()->set_y(camSensor->GetImageHeight());
  }
}

//////////////////////////////////////////////////
std::string Sensor::GetWorldName() const
{
  return this->world->GetName();
}
