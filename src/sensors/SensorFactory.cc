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
/*
 * Desc: Factory for creating sensor
 * Author: Andrew Howard
 * Date: 18 May 2003
 */

#include "sensors/SensorManager.hh"
#include "sensors/SensorFactory.hh"
#include "sensors/Sensor.hh"

void RegisterCameraSensor();
void RegisterDepthCameraSensor();
void RegisterGpuRaySensor();
void RegisterRaySensor();
void RegisterContactSensor();
void RegisterRFIDSensor();
void RegisterRFIDTag();

using namespace gazebo;
using namespace sensors;

std::map<std::string, SensorFactoryFn> SensorFactory::sensorMap;

/////////////////////////////////////////////////
void SensorFactory::RegisterAll()
{
  RegisterCameraSensor();
  RegisterDepthCameraSensor();
  RegisterGpuRaySensor();
  RegisterRaySensor();
  RegisterContactSensor();
  RegisterRFIDSensor();
  RegisterRFIDTag();
}

/////////////////////////////////////////////////
void SensorFactory::RegisterSensor(const std::string &classname,
                                   SensorFactoryFn factoryfn)
{
  sensorMap[classname] = factoryfn;
}

/////////////////////////////////////////////////
SensorPtr SensorFactory::NewSensor(const std::string &classname)
{
  SensorPtr sensor;

  if (sensorMap[classname])
  {
    sensor.reset((sensorMap[classname]) ());
  }

  return sensor;
}

/////////////////////////////////////////////////
void SensorFactory::GetSensorTypes(std::vector<std::string> &_types)
{
  _types.clear();

  std::map<std::string, SensorFactoryFn>::const_iterator iter;
  for (iter = sensorMap.begin(); iter != sensorMap.end(); ++iter)
  {
    _types.push_back(iter->first);
  }
}
