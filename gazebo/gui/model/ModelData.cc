/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#include <boost/thread/recursive_mutex.hpp>

#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/ogre_gazebo.h"

#include "gazebo/gui/model/LinkInspector.hh"
#include "gazebo/gui/model/VisualConfig.hh"
#include "gazebo/gui/model/LinkConfig.hh"
#include "gazebo/gui/model/CollisionConfig.hh"

#include "gazebo/gui/model/ModelData.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
std::string ModelData::GetTemplateSDFString()
{
  std::ostringstream newModelStr;
  newModelStr << "<sdf version ='" << SDF_VERSION << "'>"
    << "<model name='template_model'>"
    << "<pose>0 0 0.0 0 0 0</pose>"
    << "<link name ='link'>"
    <<   "<visual name ='visual'>"
    <<     "<pose>0 0 0.0 0 0 0</pose>"
    <<     "<geometry>"
    <<       "<box>"
    <<         "<size>1.0 1.0 1.0</size>"
    <<       "</box>"
    <<     "</geometry>"
    <<     "<material>"
    <<       "<lighting>true</lighting>"
    <<       "<ambient>0.3 0.3 0.3 1</ambient>"
    <<       "<diffuse>0.7 0.7 0.7 1</diffuse>"
    <<       "<specular>0.01 0.01 0.01 1</specular>"
    <<       "<script>"
    <<         "<uri>file://media/materials/scripts/gazebo.material</uri>"
    <<         "<name>Gazebo/Grey</name>"
    <<       "</script>"
    <<     "</material>"
    <<   "</visual>"
    << "</link>"
    << "<static>true</static>"
    << "</model>"
    << "</sdf>";

  return newModelStr.str();
}

/////////////////////////////////////////////////
double ModelData::GetEditTransparency()
{
  return 0.4;
}

/////////////////////////////////////////////////
PartData::PartData()
{
  this->partSDF.reset(new sdf::Element);
  sdf::initFile("link.sdf", this->partSDF);

  this->scale = math::Vector3::One;

  this->inspector = new LinkInspector();
  this->inspector->setModal(false);
  connect(this->inspector, SIGNAL(Applied()), this, SLOT(OnApply()));
  connect(this->inspector->GetVisualConfig(),
      SIGNAL(VisualAdded(const std::string &)),
      this, SLOT(OnAddVisual(const std::string &)));

  connect(this->inspector->GetCollisionConfig(),
      SIGNAL(CollisionAdded(const std::string &)),
      this, SLOT(OnAddCollision(const std::string &)));

  connect(this->inspector->GetVisualConfig(),
      SIGNAL(VisualRemoved(const std::string &)), this,
      SLOT(OnRemoveVisual(const std::string &)));

  connect(this->inspector->GetCollisionConfig(),
      SIGNAL(CollisionRemoved(const std::string &)),
      this, SLOT(OnRemoveCollision(const std::string &)));

  // note the destructor removes this connection with the assumption that it is
  // the first one in the vector
  this->connections.push_back(event::Events::ConnectPreRender(
      boost::bind(&PartData::Update, this)));
  this->updateMutex = new boost::recursive_mutex();
}

/////////////////////////////////////////////////
PartData::~PartData()
{
  event::Events::DisconnectPreRender(this->connections[0]);
  delete this->inspector;
}

/////////////////////////////////////////////////
std::string PartData::GetName() const
{
  return this->partSDF->Get<std::string>("name");
}

/////////////////////////////////////////////////
void PartData::SetName(const std::string &_name)
{
  this->partSDF->GetAttribute("name")->Set(_name);
  this->inspector->SetName(_name);
}

/////////////////////////////////////////////////
math::Pose PartData::GetPose() const
{
  return this->partSDF->Get<math::Pose>("pose");
}

/////////////////////////////////////////////////
void PartData::SetPose(const math::Pose &_pose)
{
  this->partSDF->GetElement("pose")->Set(_pose);

  LinkConfig *linkConfig = this->inspector->GetLinkConfig();
  linkConfig->SetPose(_pose);
}

/////////////////////////////////////////////////
void PartData::SetScale(const math::Vector3 &_scale)
{
  VisualConfig *visualConfig = this->inspector->GetVisualConfig();
  std::string uri;
  for (auto it = this->visuals.begin(); it != this->visuals.end(); ++it)
  {
    std::string name = it->first->GetName();
    std::string partName = this->partVisual->GetName();
    std::string leafName =
        name.substr(name.find(partName)+partName.size()+1);
    visualConfig->SetGeometry(leafName, it->first->GetScale());
  }

  CollisionConfig *collisionConfig = this->inspector->GetCollisionConfig();
  for (auto it = this->collisions.begin(); it != this->collisions.end(); ++it)
  {
    std::string name = it->first->GetName();
    std::string partName = this->partVisual->GetName();
    std::string leafName =
        name.substr(name.find(partName)+partName.size()+1);
    collisionConfig->SetGeometry(leafName,  it->first->GetScale());
  }

  this->scale = _scale;
}

/////////////////////////////////////////////////
math::Vector3 PartData::GetScale() const
{
  return this->scale;
}

/////////////////////////////////////////////////
void PartData::UpdateConfig()
{
  // set new geom size if scale has changed.
  VisualConfig *visualConfig = this->inspector->GetVisualConfig();
  for (auto &it : this->visuals)
  {
    std::string name = it.first->GetName();
    std::string partName = this->partVisual->GetName();
    std::string leafName =
        name.substr(name.find(partName)+partName.size()+1);
    visualConfig->SetGeometry(leafName, it.first->GetScale(),
        it.first->GetMeshName());

    msgs::Visual *updateMsg = visualConfig->GetData(leafName);
    msgs::Visual visualMsg = it.second;
    updateMsg->clear_scale();
    msgs::Color *diffuse = updateMsg->mutable_material()->mutable_diffuse();
    diffuse->set_a(1.0-updateMsg->transparency());
    visualMsg.CopyFrom(*updateMsg);
    it.second = visualMsg;
  }
  CollisionConfig *collisionConfig = this->inspector->GetCollisionConfig();
  for (auto &colIt : this->collisions)
  {
    std::string name = colIt.first->GetName();
    std::string partName = this->partVisual->GetName();
    std::string leafName =
        name.substr(name.find(partName)+partName.size()+1);
    collisionConfig->SetGeometry(leafName, colIt.first->GetScale(),
        colIt.first->GetMeshName());

    msgs::Collision *updateMsg = collisionConfig->GetData(leafName);
    msgs::Collision collisionMsg = colIt.second;
    collisionMsg.CopyFrom(*updateMsg);
    colIt.second = collisionMsg;
  }
}

/////////////////////////////////////////////////
void PartData::AddVisual(rendering::VisualPtr _visual)
{
  VisualConfig *visualConfig = this->inspector->GetVisualConfig();
  msgs::Visual visualMsg = msgs::VisualFromSDF(_visual->GetSDF());

  // override transparency value
  visualMsg.set_transparency(0.0);
  this->visuals[_visual] = visualMsg;

  std::string partName = this->partVisual->GetName();
  std::string visName = _visual->GetName();
  std::string leafName =
      visName.substr(visName.find(partName)+partName.size()+1);

  visualConfig->AddVisual(leafName, &visualMsg);
}

/////////////////////////////////////////////////
void PartData::AddCollision(rendering::VisualPtr _collisionVis)
{
  CollisionConfig *collisionConfig = this->inspector->GetCollisionConfig();
  msgs::Visual visualMsg = msgs::VisualFromSDF(_collisionVis->GetSDF());

  sdf::ElementPtr collisionSDF(new sdf::Element);
  sdf::initFile("collision.sdf", collisionSDF);

  msgs::Collision collisionMsg;
  collisionMsg.set_name(_collisionVis->GetName());
  msgs::Geometry *geomMsg = collisionMsg.mutable_geometry();
  geomMsg->CopyFrom(visualMsg.geometry());

  this->collisions[_collisionVis] = collisionMsg;

  std::string partName = this->partVisual->GetName();
  std::string visName = _collisionVis->GetName();
  std::string leafName =
      visName.substr(visName.find(partName)+partName.size()+1);

  collisionConfig->AddCollision(leafName, &collisionMsg);
}

/////////////////////////////////////////////////
void PartData::OnApply()
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
  LinkConfig *linkConfig = this->inspector->GetLinkConfig();

  this->partSDF = msgs::LinkToSDF(*linkConfig->GetData(), this->partSDF);
  this->partVisual->SetWorldPose(this->GetPose());

  // update visuals
  if (!this->visuals.empty())
  {
    VisualConfig *visualConfig = this->inspector->GetVisualConfig();
    for (auto &it : this->visuals)
    {
      std::string name = it.first->GetName();
      std::string partName = this->partVisual->GetName();
      std::string leafName =
          name.substr(name.find(partName)+partName.size()+1);

      msgs::Visual *updateMsg = visualConfig->GetData(leafName);
      if (updateMsg)
      {
        msgs::Visual visualMsg = it.second;

        // update the visualMsg that will be used to generate the sdf.
        updateMsg->clear_scale();
        msgs::Color *diffuse = updateMsg->mutable_material()->mutable_diffuse();
        diffuse->set_a(1.0-updateMsg->transparency());
        visualMsg.CopyFrom(*updateMsg);
        it.second = visualMsg;

        this->visualUpdateMsgs.push_back(updateMsg);
      }
    }
  }

  // update collisions
  if (!this->collisions.empty())
  {
    CollisionConfig *collisionConfig =
        this->inspector->GetCollisionConfig();
    for (auto &it : this->collisions)
    {
      std::string name = it.first->GetName();
      std::string partName = this->partVisual->GetName();
      std::string leafName =
          name.substr(name.find(partName)+partName.size()+1);

      msgs::Collision *updateMsg = collisionConfig->GetData(leafName);
      if (updateMsg)
      {
        msgs::Collision collisionMsg = it.second;
        collisionMsg.CopyFrom(*updateMsg);
        it.second = collisionMsg;

        this->collisionUpdateMsgs.push_back(updateMsg);
      }
    }
  }
}

/////////////////////////////////////////////////
void PartData::OnAddVisual(const std::string &_name)
{
  // add a visual when the user adds a visual via the inspector's visual tab
  VisualConfig *visualConfig = this->inspector->GetVisualConfig();

  std::ostringstream visualName;
  visualName << this->partVisual->GetName() << "_" << _name;

  rendering::VisualPtr visVisual;
  rendering::VisualPtr refVisual;
  if (!this->visuals.empty())
  {
    // add new visual by cloning last instance
    refVisual = this->visuals.rbegin()->first;
    visVisual = refVisual->Clone(visualName.str(), this->partVisual);
  }
  else
  {
    // create new visual based on sdf template (box)
    sdf::SDFPtr modelTemplateSDF(new sdf::SDF);
    modelTemplateSDF->SetFromString(
        ModelData::GetTemplateSDFString());

    visVisual.reset(new rendering::Visual(visualName.str(),
        this->partVisual));
    sdf::ElementPtr visualElem =  modelTemplateSDF->root
        ->GetElement("model")->GetElement("link")->GetElement("visual");
    visVisual->Load(visualElem);
    this->partVisual->GetScene()->AddVisual(visVisual);
  }

  msgs::Visual visualMsg = msgs::VisualFromSDF(visVisual->GetSDF());

  // store the correct transparency setting
  if (refVisual)
    visualMsg.set_transparency(this->visuals[refVisual].transparency());
  visualConfig->UpdateVisual(_name, &visualMsg);
  this->visuals[visVisual] = visualMsg;
  visVisual->SetTransparency(visualMsg.transparency() *
      (1-ModelData::GetEditTransparency()-0.1)
      + ModelData::GetEditTransparency());
}

/////////////////////////////////////////////////
void PartData::OnAddCollision(const std::string &_name)
{
  // add a collision when the user adds a collision via the inspector's
  // collision tab
  CollisionConfig *collisionConfig = this->inspector->GetCollisionConfig();

  std::ostringstream collisionName;
  collisionName << this->partVisual->GetName() << "_" << _name;

  rendering::VisualPtr collisionVis;
  if (!this->collisions.empty())
  {
    // add new collision by cloning last instance
    collisionVis = this->collisions.rbegin()->first->Clone(collisionName.str(),
        this->partVisual);
  }
  else
  {
    // create new collision based on sdf template (box)
    sdf::SDFPtr modelTemplateSDF(new sdf::SDF);
    modelTemplateSDF->SetFromString(
        ModelData::GetTemplateSDFString());

    collisionVis.reset(new rendering::Visual(collisionName.str(),
        this->partVisual));
    sdf::ElementPtr collisionElem =  modelTemplateSDF->root
        ->GetElement("model")->GetElement("link")->GetElement("visual");
    collisionVis->Load(collisionElem);
    // orange
    collisionVis->SetAmbient(common::Color(1.0, 0.5, 0.05));
    collisionVis->SetDiffuse(common::Color(1.0, 0.5, 0.05));
    collisionVis->SetSpecular(common::Color(0.5, 0.5, 0.5));
    this->partVisual->GetScene()->AddVisual(collisionVis);
  }

  msgs::Visual visualMsg = msgs::VisualFromSDF(collisionVis->GetSDF());
  msgs::Collision collisionMsg;
  collisionMsg.set_name(collisionVis->GetName());
  msgs::Geometry *geomMsg = collisionMsg.mutable_geometry();
  geomMsg->CopyFrom(visualMsg.geometry());

  collisionConfig->UpdateCollision(_name, &collisionMsg);
  this->collisions[collisionVis] = collisionMsg;

  collisionVis->SetTransparency(
      math::clamp(ModelData::GetEditTransparency() * 2.0, 0.0, 0.8));

  // fix for transparency alpha compositing
  Ogre::MovableObject *colObj = collisionVis->GetSceneNode()->
      getAttachedObject(0);
  colObj->setRenderQueueGroup(colObj->getRenderQueueGroup()+1);
}

/////////////////////////////////////////////////
void PartData::OnRemoveVisual(const std::string &_name)
{
  // find and remove visual when the user removes it in the
  // inspector's visual tab
  std::ostringstream name;
  name << this->partVisual->GetName() << "_" << _name;
  std::string visualName = name.str();

  for (auto it = this->visuals.begin(); it != this->visuals.end(); ++it)
  {
    if (visualName == it->first->GetName())
    {
      this->partVisual->DetachVisual(it->first);
      this->partVisual->GetScene()->RemoveVisual(it->first);
      this->visuals.erase(it);
      break;
    }
  }
}

/////////////////////////////////////////////////
void PartData::OnRemoveCollision(const std::string &_name)
{
  // find and remove collision visual when the user removes it in the
  // inspector's collision tab
  std::ostringstream name;
  name << this->partVisual->GetName() << "_" << _name;
  std::string collisionName = name.str();

  for (auto it = this->collisions.begin(); it != this->collisions.end(); ++it)
  {
    if (collisionName == it->first->GetName())
    {
      this->partVisual->DetachVisual(it->first);
      this->partVisual->GetScene()->RemoveVisual(it->first);
      this->collisions.erase(it);
      break;
    }
  }
}

/////////////////////////////////////////////////
void PartData::Update()
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);

  while (!this->visualUpdateMsgs.empty())
  {
    boost::shared_ptr<gazebo::msgs::Visual> updateMsgPtr;
    updateMsgPtr.reset(new msgs::Visual);
    updateMsgPtr->CopyFrom(*this->visualUpdateMsgs.front());

    this->visualUpdateMsgs.erase(this->visualUpdateMsgs.begin());
    for (auto &it : this->visuals)
    {
      if (it.second.name() == updateMsgPtr->name())
      {
        // make visual semi-transparent here
        // but generated sdf will use the correct transparency value
        it.first->UpdateFromMsg(updateMsgPtr);
        it.first->SetTransparency(updateMsgPtr->transparency() *
            (1-ModelData::GetEditTransparency()-0.1)
            + ModelData::GetEditTransparency());
        break;
      }
    }
  }

  while (!this->collisionUpdateMsgs.empty())
  {
    msgs::Collision collisionMsg = *this->collisionUpdateMsgs.front();
    this->collisionUpdateMsgs.erase(this->collisionUpdateMsgs.begin());
    for (auto &it : this->collisions)
    {
      if (it.second.name() == collisionMsg.name())
      {
        msgs::Visual collisionVisMsg;
        msgs::Geometry *geomMsg = collisionVisMsg.mutable_geometry();
        geomMsg->CopyFrom(collisionMsg.geometry());
        msgs::Pose *poseMsg = collisionVisMsg.mutable_pose();
        poseMsg->CopyFrom(collisionMsg.pose());

        boost::shared_ptr<gazebo::msgs::Visual> updateMsgPtr;
        updateMsgPtr.reset(new msgs::Visual);
        updateMsgPtr->CopyFrom(collisionVisMsg);
        std::string origGeomType = it.first->GetGeometryType();
        it.first->UpdateFromMsg(updateMsgPtr);

        // fix for transparency alpha compositing
        if (it.first->GetGeometryType() != origGeomType)
        {
          Ogre::MovableObject *colObj = it.first->GetSceneNode()->
              getAttachedObject(0);
          colObj->setRenderQueueGroup(colObj->getRenderQueueGroup()+1);
        }
        break;
      }
    }
  }
}