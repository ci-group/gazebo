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
/* Desc: RFID Sensor Visualization Class
 * Author:
 * Date: 
 */

#include "transport/transport.h"
#include "rendering/Conversions.hh"
#include "rendering/Scene.hh"
#include "common/MeshManager.hh"

#include "rendering/RFIDVisual.hh"

using namespace gazebo;
using namespace rendering;

/////////////////////////////////////////////////
RFIDVisual::RFIDVisual(const std::string &_name, VisualPtr _vis,
                       const std::string &_topicName)
: Visual(_name, _vis)
{
  this->node = transport::NodePtr(new transport::Node());
  this->node->Init(this->scene->GetName());

  this->rfidSub = this->node->Subscribe(_topicName, &RFIDVisual::OnScan, this);

  common::MeshManager::Instance()->CreateSphere("rfid_sphere", 5.0, 20, 20);
  this->AttachMesh("rfid_sphere");
  this->SetMaterial("Gazebo/BlueTransparent");
}

/////////////////////////////////////////////////
RFIDVisual::~RFIDVisual()
{
}

/////////////////////////////////////////////////
void RFIDVisual::OnScan(ConstPosePtr &/*_msg*/)
{
  // math::Vector3 pt = msgs::Convert(_msg->position());
  // this->sceneNode->setPosition(Conversions::Convert(pt));
}
