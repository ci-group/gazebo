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
#include <boost/bind.hpp>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/common/common.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/transport/transport.hh>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>

#include "model_move.hh"

using namespace gazebo;

void ModelMove::move(math::Vector3 *start, math::Vector3 *end,
                     math::Vector3 *translation)
{
  int duration = floor(start->Distance((*end).x, (*end).y, (*end).z));
  math::Vector3 diff = *end - *start;
  float x_step = diff.x / duration;
  float y_step = diff.y / duration;
  float z_step = diff.z / duration;
  int curr_frame = anim->GetKeyFrameCount();

  for (int i = 1; i <= duration; i++)
  {
    gazebo::common::PoseKeyFrame * key = anim->CreateKeyFrame(i+curr_frame);
    key->SetTranslation(math::Vector3(
         (*translation).x + x_step * i,
         (*translation).y + y_step * i,
         (*translation).z + z_step * i));
    key->SetRotation(math::Quaternion(0, 0, 0));
  }

  translation->Set((*translation).x + x_step*duration,
       (*translation).y + y_step*duration,
       (*translation).z + z_step*duration);
}

void ModelMove::initiateMove()
{
  float path_length = start_point.Distance(path->x, path->y, path->z);

  for (int i = 0; i < this->num_points-1; i++)
    path_length += path[i].Distance(path[i+1].x, path[i+1].y, path[i+1].z);

  // create the animation
  this->anim =
    gazebo::common::PoseAnimationPtr(
        new gazebo::common::PoseAnimation("test", path_length+1, false));

  gazebo::common::PoseKeyFrame *key;

  // set starting location of the box
  key = anim->CreateKeyFrame(0);
  key->SetTranslation(math::Vector3(0, 0, 0));
  key->SetRotation(math::Quaternion(0, 0, 0));

  math::Vector3 translation = math::Vector3(0, 0, 0);

  move(&start_point, path, &translation);
  for (int i = 0; i < this->num_points-1; i++)
    move(path+i, path+i+1, &translation);

  // set the animation
  this->model->SetAnimation(anim);
}

void ModelMove::getPathMsg(PoseAnimationPtr &msg)
{
  gzmsg << "[model_move] Received path message" << std::endl;

  this->num_points = msg->pose_size();
  this->path = new math::Vector3[this->num_points];

  for (int i = 0; i < this->num_points; i++)
    path[i] = gazebo::msgs::Convert(msg->pose(i)).pos;

  initiateMove();
}

void ModelMove::Load(physics::ModelPtr _parent, sdf::ElementPtr _sdf)
{
  // Store the pointer to the model
  this->model = _parent;

  this->node = transport::NodePtr(new transport::Node());
  this->node->Init(this->model->GetWorld()->GetName());

  // Either get parameters from sdf
  if (_sdf->HasElement("path") && _sdf->HasElement("n_points"))
  {
    gzmsg << "[model_move] Processing path defined in the SDF file" 
          << std::endl;

    sdf::Vector3 sdf_pose =
      _sdf->GetParent()->GetElement("pose")->Get<sdf::Pose>().pos;
    this->start_point =
      math::Vector3(sdf_pose.x, sdf_pose.y, sdf_pose.z);
    this->num_points =
      std::stoi(_sdf->GetElement("n_points")->Get<std::string>());
    this->path =
      new math::Vector3[this->num_points];

    std::stringstream stream(_sdf->GetElement("path")->Get<std::string>());
    for (int i = 0; i < this->num_points; i++)
    {
      float f1, f2, f3;
      stream >> f1 >> f2 >> f3;
      path[i] = math::Vector3(f1, f2, f3);
    }
    initiateMove();
  }

  std::string path_topic_name = std::string("~/") + _parent->GetName() + "/model_move";
  pathSubscriber = node->Subscribe(path_topic_name, &ModelMove::getPathMsg,
                                   this);
  gzmsg << "[model_move] Subscribed to receive paths in: "<< path_topic_name 
        << std::endl;
}
