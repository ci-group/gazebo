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

#include "physics/physics.h"
#include "transport/transport.h"
#include "plugins/JointTestPlugin.hh"

using namespace gazebo;
GZ_REGISTER_MODEL_PLUGIN(JointTestPlugin)

/////////////////////////////////////////////////
JointTestPlugin::JointTestPlugin()
{
  for (int i = 0; i < NUM_JOINTS; i++)
  {
    this->jointPIDs[i] = common::PID(1, 0.1, 0.01, 1, -1);
    this->jointPositions[i] = 0;
    this->jointVelocities[i] = 0;
    this->jointMaxEfforts[i] = 100;
  }
}

/////////////////////////////////////////////////
void JointTestPlugin::Load(physics::ModelPtr _model,
                           sdf::ElementPtr _sdf)
{
  this->model = _model;

  this->node = transport::NodePtr(new transport::Node());
  this->node->Init(this->model->GetWorld()->GetName());

  this->velSub = this->node->Subscribe(std::string("~/") +
      this->model->GetName() + "/joint_test_cmd",
      &JointTestPlugin::OnJointCmdMsg, this);

  if (!_sdf->HasElement("steer"))
    gzerr << "JointTest plugin missing <steer> element\n";

  // get all joints
  this->joints[0] = _model->GetJoint(
    _sdf->GetElement("steer")->GetValueString());
  this->jointPIDs[0] = common::PID(
    _sdf->GetElement("steer_pid")->GetValueVector3().x,
    _sdf->GetElement("steer_pid")->GetValueVector3().y,
    _sdf->GetElement("steer_pid")->GetValueVector3().z,
    _sdf->GetElement("steer_ilim")->GetValueVector2d().y,
    _sdf->GetElement("steer_ilim")->GetValueVector2d().x);
  this->jointPositions[0] =
    _sdf->GetElement("steer_pos")->GetValueDouble();
  this->jointVelocities[0] =
    _sdf->GetElement("steer_vel")->GetValueDouble();
  this->jointMaxEfforts[0] =
    _sdf->GetElement("steer_eff")->GetValueDouble();

  this->joints[1] = _model->GetJoint(
    _sdf->GetElement("right")->GetValueString());
  this->jointPIDs[1] = common::PID(
    _sdf->GetElement("right_pid")->GetValueVector3().x,
    _sdf->GetElement("right_pid")->GetValueVector3().y,
    _sdf->GetElement("right_pid")->GetValueVector3().z,
    _sdf->GetElement("right_ilim")->GetValueVector2d().y,
    _sdf->GetElement("right_ilim")->GetValueVector2d().x);
  this->jointPositions[1] =
    _sdf->GetElement("right_pos")->GetValueDouble();
  this->jointVelocities[1] =
    _sdf->GetElement("right_vel")->GetValueDouble();
  this->jointMaxEfforts[1] =
    _sdf->GetElement("right_eff")->GetValueDouble();

  this->joints[2] = _model->GetJoint(
    _sdf->GetElement("left")->GetValueString());
  this->jointPIDs[2] = common::PID(
    _sdf->GetElement("left_pid")->GetValueVector3().x,
    _sdf->GetElement("left_pid")->GetValueVector3().y,
    _sdf->GetElement("left_pid")->GetValueVector3().z,
    _sdf->GetElement("left_ilim")->GetValueVector2d().y,
    _sdf->GetElement("left_ilim")->GetValueVector2d().x);
  this->jointPositions[2] =
    _sdf->GetElement("left_pos")->GetValueDouble();
  this->jointVelocities[2] =
    _sdf->GetElement("left_vel")->GetValueDouble();
  this->jointMaxEfforts[2] =
    _sdf->GetElement("left_eff")->GetValueDouble();

  this->updateConnection = event::Events::ConnectWorldUpdateStart(
          boost::bind(&JointTestPlugin::OnUpdate, this));
}

/////////////////////////////////////////////////
void JointTestPlugin::Init()
{
  // physics::EntityPtr parent = boost::shared_dynamic_cast<physics::Entity>(
  //   this->joints[0]->GetChild());
}

/////////////////////////////////////////////////
void JointTestPlugin::OnJointCmdMsg(ConstJointCmdPtr &_msg)
{
  // update joint velocities
  for (int i = 0; i < NUM_JOINTS; i++)
  {
    // ignore everything else, get position and force only
    this->jointPositions[i] = _msg->position();
    this->jointMaxEfforts[i] = _msg->force();
  }
}

/////////////////////////////////////////////////
void JointTestPlugin::OnUpdate()
{
  common::Time currTime = this->model->GetWorld()->GetSimTime();
  common::Time stepTime = currTime - this->prevUpdateTime;
  this->prevUpdateTime = currTime;


  for (int i = 0; i < 1; i++)
  {
    // first joint, set position
    double pos_target = this->jointPositions[i];
    double pos_curr = this->joints[i]->GetAngle(0).GetAsRadian();
    double max_cmd = this->jointMaxEfforts[i];

    double pos_err = pos_curr - pos_target;

    double effort_cmd = this->jointPIDs[i].Update(pos_err, stepTime);
    effort_cmd = effort_cmd > max_cmd ? max_cmd :
      (effort_cmd < -max_cmd ? -max_cmd : effort_cmd);
    this->joints[i]->SetForce(0, effort_cmd);
    gzdbg << "steer [" << pos_curr << "] [" << pos_target << "]";
  }

  for (int i = 1; i < NUM_JOINTS; i++)
  {
    double tmp_t = currTime.Double();
    double eff;
    // custom test
    if (tmp_t < 10)      eff = 0;
    else if (tmp_t < 20) eff = this->jointMaxEfforts[i];
    else if (tmp_t < 30) eff = -this->jointMaxEfforts[i];
    else if (tmp_t < 40) eff = -this->jointMaxEfforts[i];
    else if (tmp_t < 50) eff = this->jointMaxEfforts[i];
    else if (tmp_t < 60)
    {
      /* pid to velocity */
      double vel_target = this->jointVelocities[i];
      double vel_curr = this->joints[i]->GetVelocity(0);
      double max_cmd = 100.0;  // this->jointMaxEfforts[i];

      double vel_err = vel_curr - vel_target;

      eff = this->jointPIDs[i].Update(vel_err, stepTime);
      eff = eff > max_cmd ? max_cmd :
        (eff < -max_cmd ? -max_cmd : eff);
    }
    else
    {
      // hold wheel positions
      double pos_target = this->jointPositions[i];
      double pos_curr = this->joints[i]->GetAngle(0).GetAsRadian();
      double max_cmd = 100;  // this->jointMaxEfforts[i];

      double pos_err = pos_curr - pos_target;

      eff = this->jointPIDs[i].Update(pos_err, stepTime);
      eff = eff > max_cmd ? max_cmd :
        (eff < -max_cmd ? -max_cmd : eff);
      // gzdbg << "wheel pos [" << pos_curr << "] tar [" << pos_target << "]\n";
    }

    gzdbg << " wheel pos ["
          << this->joints[i]->GetAngle(0).GetAsRadian()
          << "] vel ["
          << this->joints[i]->GetVelocity(0)
          << "] effort [" << eff << "]";
    this->joints[i]->SetForce(0, eff);
  }
  gzdbg << "\n";
}
