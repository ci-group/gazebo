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

#include "common/Console.hh"

#include "transport/Transport.hh"

#include "rendering/Rendering.hh"

#include "sensors/SensorFactory.hh"
#include "sensors/SensorManager.hh"
#include "sensors/Sensor.hh"
#include "sensors/Sensors.hh"

using namespace gazebo;

/////////////////////////////////////////////////
bool sensors::load()
{
  // Register all the sensor types
  sensors::SensorFactory::RegisterAll();

  // Load the rendering system
  if (!gazebo::rendering::load())
    gzthrow("Unable to load the rendering engine");

  return true;
}

/////////////////////////////////////////////////
bool sensors::init()
{
  // The rendering engine will run headless
  if (!gazebo::rendering::init())
  {
    gzthrow("Unable to intialize the rendering engine");
    return false;
  }

  sensors::SensorManager::Instance()->Init();

  return true;
}

/////////////////////////////////////////////////
bool sensors::fini()
{
  sensors::SensorManager::Instance()->Fini();
  rendering::fini();
  return true;
}

/////////////////////////////////////////////////
std::string sensors::create_sensor(sdf::ElementPtr _elem,
                                   const std::string &_worldName,
                                   const std::string &_parentName)
{
  return sensors::SensorManager::Instance()->CreateSensor(_elem, _worldName,
                                                          _parentName);
}

/////////////////////////////////////////////////
void sensors::remove_sensor(const std::string &_sensorName)
{
  sensors::SensorManager::Instance()->RemoveSensor(_sensorName);
}

/////////////////////////////////////////////////
void sensors::run()
{
  sensors::SensorManager::Instance()->Run();
}

/////////////////////////////////////////////////
void sensors::run_once(bool _force)
{
  sensors::SensorManager::Instance()->Update(_force);
}

/////////////////////////////////////////////////
void sensors::stop()
{
  sensors::SensorManager::Instance()->Stop();
}

/////////////////////////////////////////////////
bool sensors::remove_sensors()
{
  sensors::SensorManager::Instance()->RemoveSensors();
  return true;
}

/////////////////////////////////////////////////
sensors::SensorPtr sensors::get_sensor(const std::string &_name)
{
  return sensors::SensorManager::Instance()->GetSensor(_name);
}
