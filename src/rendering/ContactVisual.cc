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
/* Desc: Contact Visualization Class
 * Author: Nate Koenig
 */

#include "rendering/ogre.h"
#include "common/MeshManager.hh"
#include "transport/Node.hh"
#include "transport/Subscriber.hh"
#include "msgs/msgs.h"
#include "rendering/Conversions.hh"
#include "rendering/Scene.hh"
#include "rendering/DynamicLines.hh"
#include "rendering/ContactVisual.hh"

using namespace gazebo;
using namespace rendering;

/////////////////////////////////////////////////
ContactVisual::ContactVisual(const std::string &_name, VisualPtr _vis,
                             const std::string &_topicName)
: Visual(_name, _vis)
{
  this->node = transport::NodePtr(new transport::Node());
  this->node->Init(this->scene->GetName());

  this->contactsSub = this->node->Subscribe(_topicName,
      &ContactVisual::OnContact, this);

  common::MeshManager::Instance()->CreateSphere("contact_sphere", 0.05, 10, 10);

  // Add the mesh into OGRE
  if (!this->sceneNode->getCreator()->hasEntity("contact_sphere") &&
      common::MeshManager::Instance()->HasMesh("contact_sphere"))
  {
    const common::Mesh *mesh =
      common::MeshManager::Instance()->GetMesh("contact_sphere");
    this->InsertMesh(mesh);
  }

  for (unsigned int i = 0; i < 10; i++)
  {
    std::string objName = this->GetName() +
        "_contactpoint_" + boost::lexical_cast<std::string>(i);
    Ogre::Entity *obj = this->scene->GetManager()->createEntity(
        objName, "contact_sphere");
    obj->setMaterialName("Gazebo/BlueLaser");

    ContactVisual::ContactPoint *cp = new ContactVisual::ContactPoint();
    cp->sceneNode = this->sceneNode->createChildSceneNode(objName + "_node");
    cp->sceneNode->attachObject(obj);

    cp->normal = new DynamicLines(RENDERING_LINE_LIST);
    cp->depth = new DynamicLines(RENDERING_LINE_LIST);

    cp->normal->AddPoint(math::Vector3(0, 0, 0));
    cp->normal->AddPoint(math::Vector3(0, 0, 0.1));

    cp->depth->AddPoint(math::Vector3(0, 0, 0));
    cp->depth->AddPoint(math::Vector3(0, 0, -1));
    cp->sceneNode->attachObject(cp->depth);
    cp->sceneNode->attachObject(cp->normal);
    cp->sceneNode->setVisible(false);

    this->points.push_back(cp);
  }

  this->connections.push_back(
      event::Events::ConnectPreRender(
        boost::bind(&ContactVisual::Update, this)));
}

/////////////////////////////////////////////////
ContactVisual::~ContactVisual()
{
}

/////////////////////////////////////////////////
void ContactVisual::Update()
{
  int c = 0;

  if (!this->contactsMsg)
    return;

  for (int i = 0; i < this->contactsMsg->contact_size(); i++)
  {
    for (int j = 0;
        c < 10 && j < this->contactsMsg->contact(i).position_size(); j++)
    {
      math::Vector3 pos = msgs::Convert(
          this->contactsMsg->contact(i).position(j));
      math::Vector3 normal = msgs::Convert(
          this->contactsMsg->contact(i).normal(j));
      double depth = this->contactsMsg->contact(i).depth(j);

      this->points[c]->sceneNode->setVisible(true);
      this->points[c]->sceneNode->setPosition(Conversions::Convert(pos));

      this->points[c]->normal->SetPoint(1, normal*0.1);
      this->points[c]->depth->SetPoint(1, normal*-depth*10);

      this->points[c]->normal->setMaterial("Gazebo/LightOn");
      this->points[c]->depth->setMaterial("Gazebo/LightOff");
      this->points[c]->depth->Update();
      this->points[c]->normal->Update();
      c++;
    }
  }

  for ( ; c < 10; c++)
    this->points[c]->sceneNode->setVisible(false);
}

/////////////////////////////////////////////////
void ContactVisual::OnContact(ConstContactsPtr &_msg)
{
  this->contactsMsg = _msg;
}
