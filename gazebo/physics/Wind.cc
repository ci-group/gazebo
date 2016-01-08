/*
 * Copyright (C) 2012-2015 Open Source Robotics Foundation
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

#include "gazebo/physics/World.hh"
#include "gazebo/physics/Entity.hh"
#include "gazebo/transport/TransportIface.hh"
#include "gazebo/physics/WindPrivate.hh"
#include "gazebo/physics/Wind.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
Wind::Wind(WorldPtr _world)
  : dataPtr(new WindPrivate)
{
  this->dataPtr->world = _world;

  this->dataPtr->sdf.reset(new sdf::Element);
  sdf::initFile("wind.sdf", this->dataPtr->sdf);

  this->dataPtr->node = transport::NodePtr(new transport::Node());
  this->dataPtr->node->Init(this->dataPtr->world->GetName());
  this->dataPtr->windSub = this->dataPtr->node->Subscribe("~/wind",
      &Wind::OnWindMsg, this);

  this->dataPtr->responsePub =
    this->dataPtr->node->Advertise<msgs::Response>("~/response");

  this->dataPtr->requestSub = this->dataPtr->node->Subscribe("~/request",
                                           &Wind::OnRequest, this);

  this->SetLinearVelFunc(std::bind(&Wind::LinearVelDefault, this,
        std::placeholders::_1, std::placeholders::_2));
}

//////////////////////////////////////////////////
Wind::~Wind()
{
  this->dataPtr->sdf->Reset();
  this->dataPtr->sdf.reset();
  this->dataPtr->responsePub.reset();
  this->dataPtr->requestSub.reset();
  this->dataPtr->node.reset();

  delete this->dataPtr;
  this->dataPtr = NULL;
}

//////////////////////////////////////////////////
void Wind::Init(void)
{
}

//////////////////////////////////////////////////
ignition::math::Vector3d Wind::LinearVelDefault(const WindPtr &_wind,
                                                const Entity * /*_entity*/)
{
  return _wind->LinearVel();
}

//////////////////////////////////////////////////
ignition::math::Vector3d Wind::WorldLinearVel(const Entity *_entity)
{
  return this->dataPtr->linearVelFunc(shared_from_this(), _entity);
}

//////////////////////////////////////////////////
ignition::math::Vector3d Wind::RelativeLinearVel(const Entity *_entity)
{
  return _entity->GetWorldPose().Ign().Rot().Inverse().RotateVector(
      this->WorldLinearVel(_entity));
}

//////////////////////////////////////////////////
void Wind::SetLinearVel(const ignition::math::Vector3d& _vel)
{
  this->dataPtr->linearVel = _vel;
}

//////////////////////////////////////////////////
const ignition::math::Vector3d& Wind::LinearVel(void) const
{
  return this->dataPtr->linearVel;
}

//////////////////////////////////////////////////
bool Wind::SetParam(const std::string &_key,
    const boost::any &_value)
{
  try
  {
    if (_key == "linear_velocity")
    {
      ignition::math::Vector3d vel =
          boost::any_cast<ignition::math::Vector3d>(_value);
      this->dataPtr->sdf->GetElement("linear_velocity")->Set(vel);
      this->SetLinearVel(vel);
    }
    else
    {
      gzwarn << "SetParam failed for [" << _key << "] in wind " << std::endl;
      return false;
    }
  }
  catch(boost::bad_any_cast &_e)
  {
    gzerr << "Caught bad any_cast in Wind::SetParam: " << _e.what()
          << std::endl;
    return false;
  }
  catch(boost::bad_lexical_cast &_e)
  {
    gzerr << "Caught bad lexical_cast in Wind::SetParam: " << _e.what()
          << std::endl;
    return false;
  }
  return true;
}

//////////////////////////////////////////////////
boost::any Wind::Param(const std::string &_key) const
{
  boost::any value;
  this->Param(_key, value);
  return value;
}

//////////////////////////////////////////////////
bool Wind::Param(const std::string &_key,
    boost::any &_value) const
{
  if (_key == "linear_velocity")
    _value = this->LinearVel();
  else
  {
    gzwarn << "Param failed for [" << _key << "] in wind " << std::endl;
    return false;
  }

  return true;
}

//////////////////////////////////////////////////
void Wind::Fini()
{
  this->dataPtr->world.reset();
  this->dataPtr->node->Fini();
}

//////////////////////////////////////////////////
void Wind::Load(sdf::ElementPtr _sdf)
{
  this->dataPtr->sdf->Copy(_sdf);

  if (this->dataPtr->sdf->HasElement("linear_velocity"))
  {
    this->SetLinearVel(
        this->dataPtr->sdf->Get<ignition::math::Vector3d>("linear_velocity"));
  }
}

/////////////////////////////////////////////////
void Wind::OnRequest(ConstRequestPtr &_msg)
{
  msgs::Response response;
  response.set_id(_msg->id());
  response.set_request(_msg->request());
  response.set_response("success");
  std::string *serializedData = response.mutable_serialized_data();

  if (_msg->request() == "wind_info")
  {
    msgs::Wind windMsg;
    windMsg.mutable_linear_velocity()->CopyFrom(
      msgs::Convert(this->dataPtr->linearVel));
    windMsg.set_enable_wind(this->dataPtr->world->EnableWind());

    response.set_type(windMsg.GetTypeName());
    windMsg.SerializeToString(serializedData);
    this->dataPtr->responsePub->Publish(response);
  }
}

/////////////////////////////////////////////////
void Wind::OnWindMsg(ConstWindPtr &_msg)
{
  if (_msg->has_linear_velocity())
    this->SetLinearVel(msgs::ConvertIgn(_msg->linear_velocity()));

  if (_msg->has_enable_wind())
    this->dataPtr->world->EnableWind(_msg->enable_wind());
}

//////////////////////////////////////////////////
sdf::ElementPtr Wind::SDF() const
{
  return this->dataPtr->sdf;
}

/////////////////////////////////////////////////
void Wind::SetLinearVelFunc(
  std::function<ignition::math::Vector3d (const WindPtr &,
    const Entity *_entity)> _linearVelFunc)
{
  this->dataPtr->linearVelFunc = _linearVelFunc;
}
