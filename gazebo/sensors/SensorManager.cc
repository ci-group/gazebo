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
 * Desc: Class to manager all sensors
 * Author: Nate Koenig
 * Date: 18 Dec 2009
 */
#include "gazebo/common/Assert.hh"
#include "gazebo/common/Time.hh"

#include "gazebo/physics/Physics.hh"
#include "gazebo/physics/PhysicsEngine.hh"
#include "gazebo/physics/World.hh"
#include "gazebo/sensors/Sensor.hh"
#include "gazebo/sensors/SensorFactory.hh"
#include "gazebo/sensors/SensorManager.hh"

using namespace gazebo;
using namespace sensors;

//////////////////////////////////////////////////
SensorManager::SensorManager()
  : initialized(false)
{
  // sensors::IMAGE container
  this->sensorContainers.push_back(new ImageSensorContainer());

  // sensors::RAY container
  this->sensorContainers.push_back(new SensorContainer());

  // sensors::OTHER container
  this->sensorContainers.push_back(new SensorContainer());
}

//////////////////////////////////////////////////
SensorManager::~SensorManager()
{
  // Clean up the sensors.
  for (SensorContainer_V::iterator iter = this->sensorContainers.begin();
       iter != this->sensorContainers.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "Sensor Constainer is NULL");
    (*iter)->Stop();
    (*iter)->RemoveSensors();
    delete (*iter);
  }
  this->sensorContainers.clear();

  this->initSensors.clear();
}

//////////////////////////////////////////////////
void SensorManager::Run()
{
  this->RunThreads();
}

//////////////////////////////////////////////////
void SensorManager::RunThreads()
{
  // Start the non-image sensor containers. The first item in the
  // sensorsContainers list are the image-based sensors, which rely on the
  // rendering engine, which in turn requires that they run in the main
  // thread.
  for (SensorContainer_V::iterator iter = ++this->sensorContainers.begin();
       iter != this->sensorContainers.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "Sensor Constainer is NULL");
    (*iter)->Run();
  }
}

//////////////////////////////////////////////////
void SensorManager::Stop()
{
  // Start all the sensor containers.
  for (SensorContainer_V::iterator iter = this->sensorContainers.begin();
       iter != this->sensorContainers.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "Sensor Constainer is NULL");
    (*iter)->Stop();
  }
}

//////////////////////////////////////////////////
void SensorManager::Update(bool _force)
{
  Sensor_V::iterator iter;

  {
    boost::recursive_mutex::scoped_lock lock(this->mutex);

    // in case things are spawn, sensors length changes
    for (iter = this->initSensors.begin();
         iter != this->initSensors.end(); ++iter)
    {
      GZ_ASSERT((*iter) != NULL, "Sensor pointer is NULL");
      GZ_ASSERT((*iter)->GetCategory() < 0 ||
          (*iter)->GetCategory() < CATEGORY_COUNT, "Sensor category is empty");
      GZ_ASSERT(this->sensorContainers[(*iter)->GetCategory()] != NULL,
                "Sensor container is NULL");

      (*iter)->Init();
      this->sensorContainers[(*iter)->GetCategory()]->AddSensor(*iter);
    }
    this->initSensors.clear();
  }

  // Only update if there are sensors
  if (this->sensorContainers[sensors::IMAGE]->sensors.size() > 0)
    this->sensorContainers[sensors::IMAGE]->Update(_force);
}

//////////////////////////////////////////////////
bool SensorManager::SensorsInitialized()
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);
  bool result = this->initSensors.empty();
  return result;
}

//////////////////////////////////////////////////
void SensorManager::Init()
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);

  // Initialize all the sensor containers.
  for (SensorContainer_V::iterator iter = this->sensorContainers.begin();
       iter != this->sensorContainers.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "SensorContainer is NULL");
    (*iter)->Init();
  }

  this->initialized = true;
}

//////////////////////////////////////////////////
void SensorManager::Fini()
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);

  // Finalize all the sensor containers.
  for (SensorContainer_V::iterator iter = this->sensorContainers.begin();
       iter != this->sensorContainers.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "SensorContainer is NULL");
    (*iter)->Fini();
  }

  this->initialized = false;
}

//////////////////////////////////////////////////
void SensorManager::GetSensorTypes(std::vector<std::string> &_types) const
{
  sensors::SensorFactory::GetSensorTypes(_types);
}

//////////////////////////////////////////////////
std::string SensorManager::CreateSensor(sdf::ElementPtr _elem,
                                        const std::string &_worldName,
                                        const std::string &_parentName)
{
  std::string type = _elem->GetValueString("type");
  SensorPtr sensor = sensors::SensorFactory::NewSensor(type);

  if (!sensor)
  {
    gzerr << "Unable to create sensor of type[" << type << "]\n";
    return std::string();
  }

  // Must come before sensor->Load
  sensor->SetParent(_parentName);

  // Load the sensor
  sensor->Load(_worldName, _elem);

  // If the SensorManager has not been initialized, then it's okay to push
  // the sensor into one of the sensor vectors because the sensor will get
  // initialized in SensorManager::Init
  if (!this->initialized)
  {
    this->sensorContainers[sensor->GetCategory()]->AddSensor(sensor);
  }
  // Otherwise the SensorManager is already running, and the sensor will get
  // initialized during the next SensorManager::Update call.
  else
  {
    boost::recursive_mutex::scoped_lock lock(this->mutex);
    this->initSensors.push_back(sensor);
  }

  return sensor->GetScopedName();
}

//////////////////////////////////////////////////
SensorPtr SensorManager::GetSensor(const std::string &_name) const
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);

  SensorContainer_V::const_iterator iter;
  SensorPtr result;

  // Try to find the sensor in all of the containers
  for (iter = this->sensorContainers.begin();
       iter != this->sensorContainers.end() && !result; ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "SensorContainer is NULL");
    result = (*iter)->GetSensor(_name);
  }

  // If the sensor was not found, then try to find based on an unscoped
  // name.
  // If multiple sensors exist with the same name, then an error occurs
  // because we don't know which sensor is correct.
  if (!result)
  {
    SensorPtr tmpSensor;
    for (iter = this->sensorContainers.begin();
         iter != this->sensorContainers.end(); ++iter)
    {
      GZ_ASSERT((*iter) != NULL, "SensorContainer is NULL");
      tmpSensor = (*iter)->GetSensor(_name, true);

      if (!tmpSensor)
        continue;

      if (!result)
      {
        result = tmpSensor;
        GZ_ASSERT(result != NULL, "SensorContainer contains a NULL Sensor");
      }
      else
      {
        gzerr << "Unable to get a sensor, multiple sensors with the same "
          << "name[" << _name << "]. Use a scoped name instead, "
          << "world_name::model_name::link_name::sensor_name.\n";
        result.reset();
        break;
      }
    }
  }

  return result;
}

//////////////////////////////////////////////////
Sensor_V SensorManager::GetSensors() const
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);
  Sensor_V result;

  // Copy the sensor pointers
  for (SensorContainer_V::const_iterator iter = this->sensorContainers.begin();
       iter != this->sensorContainers.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "SensorContainer is NULL");
    std::copy((*iter)->sensors.begin(), (*iter)->sensors.end(),
              std::back_inserter(result));
  }

  return result;
}

//////////////////////////////////////////////////
void SensorManager::RemoveSensor(const std::string &_name)
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);
  SensorPtr sensor = this->GetSensor(_name);

  if (!sensor)
  {
    gzerr << "Unable to remove sensor[" << _name << "] because it "
          << "does not exist.\n";
  }
  else
  {
    bool removed = false;

    std::string scopedName = sensor->GetScopedName();
    for (SensorContainer_V::iterator iter = this->sensorContainers.begin();
         iter != this->sensorContainers.end() && !removed; ++iter)
    {
      GZ_ASSERT((*iter) != NULL, "SensorContainer is NULL");
      removed = (*iter)->RemoveSensor(scopedName);
    }

    if (!removed)
    {
      gzerr << "RemoveSensor failed. The SensorManager's list of sensors "
            << "changed during sensor removal. This is bad, and should "
            << "never happen.\n";
    }
  }
}

//////////////////////////////////////////////////
void SensorManager::RemoveSensors()
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);

  for (SensorContainer_V::iterator iter = this->sensorContainers.begin();
      iter != this->sensorContainers.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "SensorContainer is NULL");
    (*iter)->RemoveSensors();
  }
  this->initSensors.clear();
}


//////////////////////////////////////////////////
SensorManager::SensorContainer::SensorContainer()
{
  this->stop = true;
  this->initialized = false;
  this->runThread = NULL;
}

//////////////////////////////////////////////////
SensorManager::SensorContainer::~SensorContainer()
{
  this->sensors.clear();
}

//////////////////////////////////////////////////
void SensorManager::SensorContainer::Init()
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);

  Sensor_V::iterator iter;
  for (iter = this->sensors.begin(); iter != this->sensors.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "Sensor is NULL");
    (*iter)->Init();
  }

  this->initialized = true;
}

//////////////////////////////////////////////////
void SensorManager::SensorContainer::Fini()
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);

  Sensor_V::iterator iter;

  // Finialize each sensor in the current sensor vector
  for (iter = this->sensors.begin(); iter != this->sensors.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "Sensor is NULL");
    (*iter)->Fini();
  }

  // Remove all the sensors from the current sensor vector.
  this->sensors.clear();

  this->initialized = false;
}

//////////////////////////////////////////////////
void SensorManager::SensorContainer::Run()
{
  this->runThread = new boost::thread(
      boost::bind(&SensorManager::SensorContainer::RunLoop, this));

  GZ_ASSERT(this->runThread, "Unable to create boost::thread.");
}

//////////////////////////////////////////////////
void SensorManager::SensorContainer::Stop()
{
  this->stop = true;
  this->runCondition.notify_all();
  if (this->runThread)
  {
    this->runThread->interrupt();
    this->runThread->join();
    delete this->runThread;
    this->runThread = NULL;
  }
}

//////////////////////////////////////////////////
void SensorManager::SensorContainer::RunLoop()
{
  this->stop = false;

  physics::WorldPtr world = physics::get_world();
  GZ_ASSERT(world != NULL, "Pointer to World is NULL");

  physics::PhysicsEnginePtr engine = world->GetPhysicsEngine();
  GZ_ASSERT(engine != NULL, "Pointer to PhysicsEngine is NULL");

  engine->InitForThread();

  common::Time sleepTime, startTime, eventTime;
  double maxUpdateRate = GZ_DBL_MIN;

  boost::mutex tmpMutex;
  boost::mutex::scoped_lock lock2(tmpMutex);

  {
    // Wait for a sensor to be added.
    if (this->sensors.size() == 0)
    {
      this->runCondition.wait(lock2);
      if (this->stop)
        return;
    }

    boost::recursive_mutex::scoped_lock lock(this->mutex);
    // Get the minimum update rate from the sensors.
    for (Sensor_V::iterator iter = this->sensors.begin();
        iter != this->sensors.end() && !this->stop; ++iter)
    {
      GZ_ASSERT((*iter) != NULL, "Sensor is NULL");
      maxUpdateRate = std::max((*iter)->GetUpdateRate(), maxUpdateRate);
    }
  }

  // Calculate an appropriate sleep time.
  if (maxUpdateRate > 0)
    sleepTime.Set(1.0 / (maxUpdateRate));
  else
    sleepTime.Set(0, 1e6);

  while (!this->stop)
  {
    // Get the start time of the update.
    startTime = world->GetSimTime();
    this->Update(false);

    // Compute the time it took to update the sensors.
    eventTime = sleepTime - (world->GetSimTime() - startTime);

    if (eventTime > common::Time::Zero)
    {
      // Add an event to trigger when the appropriate simulation time has been
      // reached.
      SimTimeEventHandler::Instance()->AddRelativeEvent(eventTime,
          &this->runCondition);
      this->runCondition.wait(lock2);
    }
  }
}

//////////////////////////////////////////////////
void SensorManager::SensorContainer::Update(bool _force)
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);

  if (this->sensors.size() == 0)
    gzerr << "Updating a sensor containing without any sensors.\n";

  // Update all the sensors in this container.
  for (Sensor_V::iterator iter = this->sensors.begin();
       iter != this->sensors.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "Sensor is NULL");
    (*iter)->Update(_force);
  }
}

//////////////////////////////////////////////////
SensorPtr SensorManager::SensorContainer::GetSensor(const std::string &_name,
                                                    bool _useLeafName) const
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);

  SensorPtr result;

  // Look for a sensor with the correct name
  for (Sensor_V::const_iterator iter = this->sensors.begin();
       iter != this->sensors.end() && !result; ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "Sensor is NULL");

    // We match on the scoped name (model::link::sensor) because multiple
    // sensors with the name leaf name make exists in a world.
    if ((_useLeafName && (*iter)->GetName() == _name) ||
        (!_useLeafName && (*iter)->GetScopedName() == _name))
    {
      result = (*iter);
      break;
    }
  }

  return result;
}

//////////////////////////////////////////////////
void SensorManager::SensorContainer::AddSensor(SensorPtr _sensor)
{
  GZ_ASSERT(_sensor != NULL, "Sensor is NULL when passed to ::AddSensor");
  boost::recursive_mutex::scoped_lock lock(this->mutex);
  this->sensors.push_back(_sensor);

  // Tell the run loop that we have received a sensor
  this->runCondition.notify_one();
}

//////////////////////////////////////////////////
bool SensorManager::SensorContainer::RemoveSensor(const std::string &_name)
{
  boost::recursive_mutex::scoped_lock lock(this->mutex);

  Sensor_V::iterator iter;

  bool removed = false;

  // Find the correct sensor based on name, and remove it.
  for (iter = this->sensors.begin(); iter != this->sensors.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "Sensor is NULL");

    if ((*iter)->GetScopedName() == _name)
    {
      (*iter)->Fini();
      this->sensors.erase(iter);
      removed = true;
      break;
    }
  }

  return removed;
}

//////////////////////////////////////////////////
void SensorManager::SensorContainer::RemoveSensors()
{
  Sensor_V::iterator iter;

  // Remove all the sensors
  for (iter = this->sensors.begin(); iter != this->sensors.end(); ++iter)
  {
    GZ_ASSERT((*iter) != NULL, "Sensor is NULL");
    (*iter)->Fini();
  }

  this->sensors.clear();
}

//////////////////////////////////////////////////
void SensorManager::ImageSensorContainer::Update(bool _force)
{
  event::Events::preRender();

  // Tell all the cameras to render
  event::Events::render();

  event::Events::postRender();

  // Update the sensors, which will produce data messages.
  SensorContainer::Update(_force);
}




/////////////////////////////////////////////////
SimTimeEventHandler::SimTimeEventHandler()
{
  this->world = physics::get_world();
  GZ_ASSERT(this->world != NULL, "World pointer is NULL");

  this->updateConnection = event::Events::ConnectWorldUpdateBegin(
      boost::bind(&SimTimeEventHandler::OnUpdate, this, _1));
}

/////////////////////////////////////////////////
SimTimeEventHandler::~SimTimeEventHandler()
{
  // Cleanup the events.
  for (std::list<SimTimeEvent*>::iterator iter = this->events.begin();
       iter != this->events.end(); ++iter)
  {
    GZ_ASSERT(*iter != NULL, "SimTimeEvent is NULL");
    delete *iter;
  }
  this->events.clear();
}

/////////////////////////////////////////////////
void SimTimeEventHandler::AddRelativeEvent(const common::Time &_time,
                                           boost::condition_variable *_var)
{
  boost::mutex::scoped_lock lock(this->mutex);

  // Create the new event.
  SimTimeEvent *event = new SimTimeEvent;
  event->time = this->world->GetSimTime() + _time;
  event->condition = _var;

  // Add the event to the list.
  this->events.push_back(event);
}

/////////////////////////////////////////////////
void SimTimeEventHandler::OnUpdate(const common::UpdateInfo &_info)
{
  GZ_ASSERT(this->world != NULL, "World pointer is NULL");

  {
    boost::mutex::scoped_lock lock(this->mutex);

    // Iterate over all the events.
    for (std::list<SimTimeEvent*>::iterator iter = this->events.begin();
        iter != this->events.end();)
    {
      GZ_ASSERT(*iter != NULL, "SimTimeEvent is NULL");

      // Find events that have a time less than or equal to simulation
      // time.
      if ((*iter)->time <= _info.simTime)
      {
        // Notify the event by triggering its condition.
        (*iter)->condition->notify_all();

        // Remove the event.
        delete *iter;
        this->events.erase(iter++);
      }
      else
        ++iter;
    }
  }
}
