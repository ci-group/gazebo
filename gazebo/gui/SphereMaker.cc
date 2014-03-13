/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
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
#include <sstream>

#include "gazebo/msgs/msgs.hh"
#include "gazebo/gui/GuiEvents.hh"

#include "gazebo/common/Console.hh"
#include "gazebo/common/MouseEvent.hh"
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
  this->state = 0;
  this->leftMousePressed = false;
  this->visualMsg = new msgs::Visual();
  this->visualMsg->mutable_geometry()->set_type(msgs::Geometry::SPHERE);

  this->visualMsg->mutable_material()->mutable_script()->add_uri(
      "gazebo://media/materials/scripts/gazebo.material");
  this->visualMsg->mutable_material()->mutable_script()->set_name(
      "Gazebo/TurquoiseGlowOutline");
  msgs::Set(this->visualMsg->mutable_pose()->mutable_orientation(),
      math::Quaternion());
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
  this->state = 1;
}

/////////////////////////////////////////////////
void SphereMaker::Stop()
{
  msgs::Request *msg = msgs::CreateRequest("entity_delete",
                                           this ->visualMsg->name());

  this->requestPub->Publish(*msg);
  delete msg;

  this->state = 0;
  gui::Events::moveMode(true);
}

/////////////////////////////////////////////////
bool SphereMaker::IsActive() const
{
  return this->state > 0;
}

/////////////////////////////////////////////////
void SphereMaker::OnMousePush(const common::MouseEvent &_event)
{
  if (this->state == 0)
    return;

  this->mousePushPos = _event.pressPos;
}

/////////////////////////////////////////////////
void SphereMaker::OnMouseRelease(const common::MouseEvent &/*_event*/)
{
  if (this->state == 0)
    return;

  this->state++;

  if (this->state == 2)
  {
    this->CreateTheEntity();
    this->Stop();
  }
}

/////////////////////////////////////////////////
void SphereMaker::OnMouseDrag(const common::MouseEvent &_event)
{
  if (this->state == 0)
    return;

  math::Vector3 norm;
  math::Vector3 p1, p2;

  norm.Set(0, 0, 1);

  if (!this->camera->GetWorldPointOnPlane(this->mousePushPos.x,
                                          this->mousePushPos.y,
                                          math::Plane(norm), p1))
  {
    gzerr << "Invalid mouse point\n";
    return;
  }

  p1 = this->GetSnappedPoint(p1);

  if (!this->camera->GetWorldPointOnPlane(
        _event.pos.x, _event.pos.y, math::Plane(norm), p2))
  {
    gzerr << "Invalid mouse point\n";
    return;
  }

  p2 = this->GetSnappedPoint(p2);

  msgs::Set(this->visualMsg->mutable_pose()->mutable_position(), p1);

  double scale = p1.Distance(p2);
  math::Vector3 p(this->visualMsg->pose().position().x(),
                  this->visualMsg->pose().position().y(),
                  this->visualMsg->pose().position().z());

  p.z = scale;

  msgs::Set(this->visualMsg->mutable_pose()->mutable_position(), p);
  this->visualMsg->mutable_geometry()->mutable_sphere()->set_radius(scale);
  this->visPub->Publish(*this->visualMsg);
}

/////////////////////////////////////////////////
std::string SphereMaker::GetSDFString()
{
  std::ostringstream newModelStr;
  newModelStr << "<sdf version ='" << SDF_VERSION << "'>"
    << "<model name='unit_sphere_" << counter << "'>"
    << "  <pose>0 0 0.5 0 0 0</pose>"
    << "  <link name='link'>"
    << "    <inertial><mass>1.0</mass></inertial>"
    << "    <collision name='collision'>"
    << "      <geometry>"
    << "        <sphere><radius>0.5</radius></sphere>"
    << "      </geometry>"
    << "    </collision>"
    << "    <visual name ='visual'>"
    << "      <geometry>"
    << "        <sphere><radius>0.5</radius></sphere>"
    << "      </geometry>"
    << "      <material>"
    << "        <script>"
    << "          <uri>file://media/materials/scripts/gazebo.material</uri>"
    << "          <name>Gazebo/Grey</name>"
    << "        </script>"
    << "      </material>"
    << "    </visual>"
    << "  </link>"
    << "  </model>"
    << "</sdf>";

  return newModelStr.str();
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
