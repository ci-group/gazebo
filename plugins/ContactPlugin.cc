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
 * Desc: Contact plugin
 * Author: Nate Koenig mod by John Hsu
 */

#include "physics/physics.hh"
#include "ContactPlugin.hh"

using namespace gazebo;

// Register this plugin with the simulator
GZ_REGISTER_SENSOR_PLUGIN(ContactPlugin)

/////////////////////////////////////////////////
ContactPlugin::ContactPlugin()
{
}

/////////////////////////////////////////////////
ContactPlugin::~ContactPlugin()
{
  this->parentSensor.reset();
  this->world.reset();
}

/////////////////////////////////////////////////
void ContactPlugin::Load(sensors::SensorPtr _parent, sdf::ElementPtr /*_sdf*/)
{
  // Get then name of the parent sensor
  this->parentSensor =
    boost::shared_dynamic_cast<sensors::ContactSensor>(_parent);

  if (!this->parentSensor)
    gzthrow("ContactPlugin requires a Contact Sensor as its parent");

  this->world = physics::get_world(this->parentSensor->GetWorldName());
}
