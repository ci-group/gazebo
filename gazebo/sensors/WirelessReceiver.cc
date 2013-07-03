/*
 * Copyright 2013 Open Source Robotics Foundation
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
/* Desc: Wireless receiver
 * Author: Carlos Agüero 
 * Date: 24 June 2013
 */

#include "gazebo/math/Rand.hh"
#include "gazebo/msgs/msgs.hh"
#include "gazebo/sensors/SensorFactory.hh"
#include "gazebo/sensors/SensorManager.hh"
#include "gazebo/sensors/WirelessReceiver.hh"
#include "gazebo/sensors/WirelessTransmitter.hh"
#include "gazebo/transport/Node.hh"
#include "gazebo/transport/Publisher.hh"

using namespace gazebo;
using namespace sensors;

GZ_REGISTER_STATIC_SENSOR("wirelessReceiver", WirelessReceiver)

const double WirelessReceiver::N = 4.5;
const double WirelessReceiver::C = 300000000;

/////////////////////////////////////////////////
WirelessReceiver::WirelessReceiver()
  : Sensor(sensors::OTHER)
{
  this->active = false;
}

/////////////////////////////////////////////////
WirelessReceiver::~WirelessReceiver()
{
}

//////////////////////////////////////////////////                              
std::string WirelessReceiver::GetTopic() const                               
{                                                                               
  std::string topicName = "~/";                                                 
  topicName += this->parentName + "/" + this->GetName() + "/receiver";          
  boost::replace_all(topicName, "::", "/");                                     
                                                                                
  return topicName;                                                             
}

/////////////////////////////////////////////////
void WirelessReceiver::Load(const std::string &_worldName)
{
  Sensor::Load(_worldName);

  this->pub = this->node->Advertise<msgs::WirelessNodes>(this->GetTopic(), 30);
  this->entity = this->world->GetEntity(this->parentName);

  if (this->sdf->HasElement("transceiver"))
  {
    sdf::ElementPtr transElem = this->sdf->GetElement("transceiver");

    if (transElem->HasElement("frequency"))
    {
      this->freq = transElem->GetValueDouble("frequency");
    }

    if (transElem->HasElement("power"))
    {
      this->power = transElem->GetValueDouble("power");
    }

    if (transElem->HasElement("gain"))
    {
      this->gain = transElem->GetValueDouble("gain");
    }
  }
}

/////////////////////////////////////////////////
void WirelessReceiver::Fini()
{
  Sensor::Fini();
}

//////////////////////////////////////////////////
void WirelessReceiver::Init()
{
  Sensor::Init();
}

//////////////////////////////////////////////////
void WirelessReceiver::UpdateImpl(bool /*_force*/)
{
  if (this->pub)                                                           
  {
    std::string id;                                                                           
    msgs::WirelessNodes msg;
    double rxPower;
    double txFreq;
    double txGain;
    math::Pose txPos;
    double txPow;
    double x;
    double wavelength;

    Sensor_V sensors = SensorManager::Instance()->GetSensors();
    for (Sensor_V::iterator it = sensors.begin(); it != sensors.end(); ++it)
    {
      if ((*it)->GetType() == "wirelessTransmitter")
      {
        id = boost::dynamic_pointer_cast<WirelessTransmitter>(*it)->
            GetESSID();
        txFreq = boost::dynamic_pointer_cast<WirelessTransmitter>(*it)->
            GetFreq();
        txPow = boost::dynamic_pointer_cast<WirelessTransmitter>(*it)->
            GetPower();
        txGain = boost::dynamic_pointer_cast<WirelessTransmitter>(*it)->
            GetGain();
        txPos = boost::dynamic_pointer_cast<WirelessTransmitter>(*it)->
            GetPose();

        msgs::WirelessNode *wirelessNode = msg.add_node();
        wirelessNode->set_essid(id);
        wirelessNode->set_frequency(txFreq);

        math::Pose myPos = entity->GetWorldPose();
        double distance = myPos.pos.Distance(txPos.pos);
        
        x = math::Rand::GetDblNormal(0.0, 10.0);
        wavelength = C / txFreq;

        rxPower = txPow + txGain + this->gain - x + 20 * log10(wavelength) -
                  20 * log10(4 * M_PI) - 10 * N * log10(distance);
        wirelessNode->set_signal_level(rxPower);
      }
    }
                                                                                
    this->pub->Publish(msg);                                               
  }
}

/////////////////////////////////////////////////
double WirelessReceiver::GetFreq()
{
  return this->freq;
}

/////////////////////////////////////////////////
double WirelessReceiver::GetPower()
{
  return this->power;
}

/////////////////////////////////////////////////
double WirelessReceiver::GetGain()
{
  return this->gain;
}
