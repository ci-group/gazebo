/*
 * Copyright (C) 2013-2016 Open Source Robotics Foundation
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
#ifdef _WIN32
  // Ensure that Winsock2.h is included before Windows.h, which can get
  // pulled in by anybody (e.g., Boost).
  #include <Winsock2.h>
#endif

#include "gazebo/physics/PhysicsIface.hh"
#include "BreakableJointPlugin.hh"

using namespace gazebo;

// Register this plugin with the simulator
GZ_REGISTER_SENSOR_PLUGIN(BreakableJointPlugin)

/////////////////////////////////////////////////
BreakableJointPlugin::BreakableJointPlugin()
  : breakingForce(1.0)
{
}

/////////////////////////////////////////////////
BreakableJointPlugin::~BreakableJointPlugin()
{
}

/////////////////////////////////////////////////
void BreakableJointPlugin::Load(sensors::SensorPtr _parent,
    sdf::ElementPtr _sdf)
{
  ForceTorquePlugin::Load(_parent, _sdf);

  std::string paramName = "breaking_force_N";
  if (_sdf->HasElement(paramName))
    this->breakingForce = _sdf->Get<double>(paramName);

  this->parentJoint = this->parentSensor->Joint();
}

/////////////////////////////////////////////////
void BreakableJointPlugin::OnUpdate(msgs::WrenchStamped _msg)
{
  if (this->parentJoint)
  {
    ignition::math::Vector3d force = msgs::ConvertIgn(_msg.wrench().force());
    if (force.Length() > this->breakingForce)
    {
      this->worldConnection = event::Events::ConnectWorldUpdateBegin(
        boost::bind(&BreakableJointPlugin::OnWorldUpdate, this));
    }
  }
}

/////////////////////////////////////////////////
void BreakableJointPlugin::OnWorldUpdate()
{
  this->parentSensor->SetActive(false);
  this->parentJoint->Detach();
  this->parentJoint->SetProvideFeedback(false);
  event::Events::DisconnectWorldUpdateBegin(this->worldConnection);
}
