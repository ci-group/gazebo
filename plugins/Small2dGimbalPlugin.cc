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

#include "gazebo/physics/physics.hh"
#include "gazebo/transport/transport.hh"
#include "Small2dGimbalPlugin.hh"

using namespace gazebo;
using namespace std;

GZ_REGISTER_MODEL_PLUGIN(Small2dGimbalPlugin)

/////////////////////////////////////////////////
Small2dGimbalPlugin::Small2dGimbalPlugin()
  :status("closed"), command(false)
{
  this->pid.Init(1, 0, 0, 0, 0, 1.0, -1.0);
  this->command = M_PI/2.0;
}

/////////////////////////////////////////////////
void Small2dGimbalPlugin::Load(physics::ModelPtr _model,
  sdf::ElementPtr /*_sdf*/)
{
  this->model = _model;
  std::string jointName = "gimbal_small_2d::tilt_joint";
  this->tiltJoint = this->model->GetJoint(jointName);
  if (!this->tiltJoint)
  {
    gzerr << "Small2dGimbalPlugin::Load ERROR! Can't get joint '"
          << jointName << "' " << endl;
  }
}

/////////////////////////////////////////////////
void Small2dGimbalPlugin::Init()
{
  this->node = transport::NodePtr(new transport::Node());
  this->node->Init(this->model->GetWorld()->GetName());

  this->lastUpdateTime = this->model->GetWorld()->GetSimTime();

  std::string topic = std::string("~/") +  this->model->GetName() +
    "/gimbal_tilt_cmd";
  this->sub = this->node->Subscribe(topic,
                                       &Small2dGimbalPlugin::OnStringMsg,
                                       this);

  this->connections.push_back(event::Events::ConnectWorldUpdateBegin(
          boost::bind(&Small2dGimbalPlugin::OnUpdate, this)));

  topic = std::string("~/") +  this->model->GetName() + "/gimbal_tilt_status";
  this->pub = node->Advertise<gazebo::msgs::GzString>(topic);

  gzmsg << "Small2dGimbalPlugin::Init" << std::endl;
}

/////////////////////////////////////////////////
void Small2dGimbalPlugin::OnStringMsg(ConstGzStringPtr &_msg)
{
  gzmsg << "Command received " << _msg->data() << std::endl;
  this->command = atof(_msg->data().c_str());
}

/////////////////////////////////////////////////
void Small2dGimbalPlugin::OnUpdate()
{
  if (!this->tiltJoint)
    return;

  double angle = this->tiltJoint->GetAngle(0).Radian();

  common::Time time = this->model->GetWorld()->GetSimTime();
  if (time < this->lastUpdateTime)
  {
    gzerr << "time reset event\n";
    this->lastUpdateTime = time;
    return;
  }
  else if (time > this->lastUpdateTime)
  {
    double dt = (this->lastUpdateTime - time).Double();
    double error = angle - this->command;
    double force = this->pid.Update(error, dt);
    this->tiltJoint->SetForce(0, force);
    this->lastUpdateTime = time;
  }

  static int i =1000;
  if (++i>100)
  {
    i = 0;
    std::stringstream ss;
    ss << angle;
    gazebo::msgs::GzString m;
    m.set_data(ss.str());
    this->pub->Publish(m);
  }
}


