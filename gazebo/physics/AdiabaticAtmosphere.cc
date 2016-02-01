/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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

#include <sdf/sdf.hh>

#include "gazebo/transport/Publisher.hh"

#include "gazebo/physics/AdiabaticAtmosphere.hh"
#include "gazebo/physics/AdiabaticAtmospherePrivate.hh"
#include "gazebo/physics/AtmosphereFactory.hh"
#include "gazebo/physics/PhysicsEngine.hh"
#include "gazebo/physics/World.hh"

using namespace gazebo;
using namespace physics;

GZ_REGISTER_ATMOSPHERE_MODEL("adiabatic", AdiabaticAtmosphere)

const double AdiabaticAtmosphere::MOLAR_MASS = 0.0289644;
const double AdiabaticAtmosphere::IDEAL_GAS_CONSTANT_R = 8.3144621;

//////////////////////////////////////////////////
AdiabaticAtmosphere::AdiabaticAtmosphere(WorldPtr _world)
  : Atmosphere(_world), dataPtr(new AdiabaticAtmospherePrivate)
{
}

//////////////////////////////////////////////////
AdiabaticAtmosphere::~AdiabaticAtmosphere()
{
}

//////////////////////////////////////////////////
void AdiabaticAtmosphere::Load(sdf::ElementPtr _sdf)
{
  Atmosphere::Load(_sdf);
}

//////////////////////////////////////////////////
void AdiabaticAtmosphere::Init(void)
{
  this->dataPtr->adiabaticPower = MOLAR_MASS *
      -this->World()->GetPhysicsEngine()->GetGravity().z /
      (-Atmosphere::TemperatureGradient() * IDEAL_GAS_CONSTANT_R);
}

//////////////////////////////////////////////////
std::string AdiabaticAtmosphere::Type() const
{
  return "adiabatic";
}

/////////////////////////////////////////////////
void AdiabaticAtmosphere::OnRequest(ConstRequestPtr &_msg)
{
  msgs::Response response;
  response.set_id(_msg->id());
  response.set_request(_msg->request());
  response.set_response("success");
  std::string *serializedData = response.mutable_serialized_data();

  if (_msg->request() == "atmosphere_info")
  {
    msgs::Atmosphere atmosphereMsg;
    atmosphereMsg.set_type(msgs::Atmosphere::ADIABATIC);
    atmosphereMsg.set_enable_atmosphere(
            this->World()->AtmosphereEnabled());
    atmosphereMsg.set_temperature(Atmosphere::Temperature());
    atmosphereMsg.set_pressure(Atmosphere::Pressure());
    atmosphereMsg.set_mass_density(Atmosphere::MassDensity());

    response.set_type(atmosphereMsg.GetTypeName());
    atmosphereMsg.SerializeToString(serializedData);
    this->Publish(response);
  }
}

/////////////////////////////////////////////////
void AdiabaticAtmosphere::OnAtmosphereMsg(ConstAtmospherePtr &_msg)
{
  // Parent class handles many generic parameters
  // This should be done first so that the profile settings
  // can be over-ridden by other message parameters.
  Atmosphere::OnAtmosphereMsg(_msg);

  if (_msg->has_enable_atmosphere())
    this->World()->SetAtmosphereEnabled(_msg->enable_atmosphere());

  if (_msg->has_temperature())
    this->SetTemperature(_msg->temperature());

  if (_msg->has_pressure())
    this->SetPressure(_msg->pressure());

  if (_msg->has_mass_density())
    this->SetMassDensity(_msg->mass_density());
}

//////////////////////////////////////////////////
void AdiabaticAtmosphere::SetTemperatureGradient(const double _gradient)
{
  Atmosphere::SetTemperatureGradient(_gradient);
  this->Init();
}

//////////////////////////////////////////////////
double AdiabaticAtmosphere::Temperature(const double _altitude) const
{
  double temperature = Atmosphere::Temperature() +
      Atmosphere::TemperatureGradient() * _altitude;
  return temperature;
}

//////////////////////////////////////////////////
double AdiabaticAtmosphere::Pressure(const double _altitude) const
{
  if (!ignition::math::equal(Atmosphere::Temperature(), 0.0, 1e-6))
  {
    // See https://en.wikipedia.org/wiki/Density_of_air#Altitude
    return Atmosphere::Pressure() *
        pow(1 + (Atmosphere::TemperatureGradient() * _altitude) /
            Atmosphere::Temperature(), this->dataPtr->adiabaticPower);
  }
  else
  {
    return 0;
  }
}

//////////////////////////////////////////////////
double AdiabaticAtmosphere::MassDensity(const double _altitude) const
{
  if (!ignition::math::equal(Atmosphere::Temperature(), 0.0, 1e-6))
  {
    // See https://en.wikipedia.org/wiki/Density_of_air#Altitude
    return Atmosphere::MassDensity() *
      pow(1 + (Atmosphere::TemperatureGradient() * _altitude) /
          Atmosphere::Temperature(), this->dataPtr->adiabaticPower - 1);
  }
  else
  {
    return 0;
  }
}
