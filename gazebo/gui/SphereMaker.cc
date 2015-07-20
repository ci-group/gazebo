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

#include <sstream>

#include "gazebo/msgs/msgs.hh"
#include "gazebo/gui/GuiEvents.hh"

#include "gazebo/common/Console.hh"
#include "gazebo/math/Quaternion.hh"

#include "gazebo/rendering/UserCamera.hh"

#include "gazebo/transport/Publisher.hh"

#include "gazebo/gui/SphereMaker.hh"

using namespace gazebo;
using namespace gui;

unsigned int SphereMaker::counter = 0;

/////////////////////////////////////////////////
SphereMaker::SphereMaker()
  : EntityMaker()
{
  this->visualMsg = new msgs::Visual();
  this->visualMsg->mutable_geometry()->set_type(msgs::Geometry::SPHERE);

  this->visualMsg->mutable_material()->mutable_script()->add_uri(
      "gazebo://media/materials/scripts/gazebo.material");
  this->visualMsg->mutable_material()->mutable_script()->set_name(
      "Gazebo/TurquoiseGlowOutline");
  msgs::Set(this->visualMsg->mutable_pose()->mutable_orientation(),
      ignition::math::Quaterniond());
}

/////////////////////////////////////////////////
SphereMaker::~SphereMaker()
{
  this->camera.reset();
  delete this->visualMsg;
}

/////////////////////////////////////////////////
void SphereMaker::Start(const rendering::UserCameraPtr _camera)
{
  this->camera = _camera;

  std::ostringstream stream;
  stream << "__GZ_USER_sphere_" << counter++;
  this->visualMsg->set_name(stream.str());
}

/////////////////////////////////////////////////
void SphereMaker::Stop()
{
  msgs::Request *msg = msgs::CreateRequest("entity_delete",
                                           this ->visualMsg->name());

  this->requestPub->Publish(*msg);
  delete msg;

  gui::Events::moveMode(true);
}

/////////////////////////////////////////////////
std::string SphereMaker::GetSDFString()
{
  msgs::Model model;
  {
    std::ostringstream modelName;
    modelName << "unit_sphere_" << counter;
    model.set_name(modelName.str());
  }
  msgs::Set(model.mutable_pose(), ignition::math::Pose3d(0, 0, 0.5, 0, 0, 0));
  msgs::AddSphereLink(model, 1.0, 0.5);
  model.mutable_link(0)->set_name("link");

  return "<sdf version='" + std::string(SDF_VERSION) + "'>"
         + msgs::ModelToSDF(model)->ToString("")
         + "</sdf>";
}


/////////////////////////////////////////////////
void SphereMaker::CreateTheEntity()
{
  msgs::Factory msg;
  msg.set_sdf(this->GetSDFString());

  msgs::Request *requestMsg = msgs::CreateRequest("entity_delete",
      this->visualMsg->name());
  this->requestPub->Publish(*requestMsg);
  delete requestMsg;

  this->makerPub->Publish(msg);
  this->camera.reset();
}
