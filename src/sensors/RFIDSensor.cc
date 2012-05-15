/*  Copyright (C)
 *     Jonas Mellin & Zakiruz Zaman
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
/* Desc: RFID Sensor
 * Author: Jonas Mellin & Zakiruz Zaman 
 * Date: 6th December 2011
 */

#include "common/Exception.hh"

#include "transport/Node.hh"
#include "transport/Publisher.hh"

#include "msgs/msgs.h"

#include "math/Vector3.hh"

#include "sensors/SensorFactory.hh"
#include "sensors/RFIDSensor.hh"
#include "sensors/RFIDTagManager.hh"

using namespace gazebo;
using namespace sensors;

GZ_REGISTER_STATIC_SENSOR("rfid", RFIDSensor)

/////////////////////////////////////////////////
RFIDSensor::RFIDSensor()
  : Sensor()
{
  this->active = false;
  this->node = transport::NodePtr(new transport::Node());
}

/////////////////////////////////////////////////
RFIDSensor::~RFIDSensor()
{
  // this->link.reset();
}

/////////////////////////////////////////////////
void RFIDSensor::Load(const std::string &_worldName, sdf::ElementPtr _sdf )
{
  Sensor::Load(_worldName, _sdf);
}

/////////////////////////////////////////////////
void RFIDSensor::Load(const std::string &_worldName)
{
  Sensor::Load(_worldName);

  // std::cout << "load rfid sensor" << std::endl;

  if (this->sdf->GetElement("topic"))
  {
    this->node->Init(this->world->GetName());
    this->scanPub = this->node->Advertise<msgs::Pose>(
        this->sdf->GetElement("topic")->GetValueString());
  }
  this->entity = this->world->GetEntity(this->parentName);

  this->rtm = RFIDTagManager::Instance();

  // this->sdf->PrintDescription("something");
  /*std::cout << " setup ray" << std::endl;
  physics::PhysicsEnginePtr physicsEngine = world->GetPhysicsEngine();

  //trying to use just "ray" gives a seg fault
  this->laserCollision = physicsEngine->CreateCollision("multiray",
      this->parentName);

  this->laserCollision->SetName("rfid_sensor_collision");
  this->laserCollision->SetRelativePose(this->pose);

  this->laserShape = boost::dynamic_pointer_cast<physics::RayShape>(
      this->laserCollision->GetShape());

  this->laserShape->Load(this->sdf);

  this->laserShape->Init();
  */

  /*** Tried to use rendering, but says rendering engine isnt initialized
       which is understandable */

  /**
    rendering::ScenePtr scene = rendering::get_scene(this->world->GetName());
    if (!scene)
    {
    scene = rendering::create_scene(this->world->GetName(), false);
    }

    manager = rendering::get_scene(this->world->GetName())->GetManager();

    query = manager->createRayQuery(Ogre::Ray()); 

*/
}

/////////////////////////////////////////////////
void RFIDSensor::Fini()
{
  Sensor::Fini();
}

//////////////////////////////////////////////////
void RFIDSensor::Init()
{
  Sensor::Init();
}

//////////////////////////////////////////////////
void RFIDSensor::UpdateImpl(bool /*_force*/)
{
  this->EvaluateTags();

  if (this->scanPub)
  {
    msgs::Pose msg;
    msgs::Set(&msg, entity->GetWorldPose());
    this->scanPub->Publish(msg);
  }
}

//////////////////////////////////////////////////
void RFIDSensor::EvaluateTags()
{
  std::vector<RFIDTag*> tags = this->rtm->GetTags();
  std::vector<RFIDTag*>::const_iterator ci;

  // iterate through the tags contained given rfid tag manager
  for (ci = tags.begin(); ci != tags.end(); ++ci)
  {
    math::Pose pos = (*ci)->GetTagPose();
    // std::cout << "link: " << tagModelPtr->GetName() << std::endl;
    // std::cout << "link pos: x" << pos.pos.x
    //     << " y:" << pos.pos.y
    //     << " z:" << pos.pos.z << std::endl;
    this->CheckTagRange(pos);
  }
}

//////////////////////////////////////////////////
bool RFIDSensor::CheckTagRange(const math::Pose &_pose)
{
  // copy sensor vector pos into a temp var
  math::Vector3 v;
  v = _pose.pos - this->entity->GetWorldPose().pos;

  // std::cout << v.GetLength() << std::endl;

  if (v.GetLength() <= 5.0)
  {
    // std::cout << "detected " <<  v.GetLength() << std::endl;
    return true;
  }

  // this->CheckRayIntersection(link);
  return false;
}

//////////////////////////////////////////////////
bool RFIDSensor::CheckRayIntersection(const math::Pose &/*_pose*/)
{
  /** rendering code below to check for intersections

    math::Vector3 d;
  //calculate direction, by adding 2 vectors?
  d = _pose.pos + entity->GetWorldPose().pos;

  Ogre::Ray ray(rendering::Conversions::Convert(entity->GetWorldPose().pos),rendering::Conversions::Convert(d));
  query->setRay(ray);
  Ogre::RaySceneQueryResult &result = query->execute();
  return false;
   **/
  return false;
}
