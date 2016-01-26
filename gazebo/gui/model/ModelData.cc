/*
 * Copyright (C) 2015-2016 Open Source Robotics Foundation
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

#include <boost/thread/recursive_mutex.hpp>

#include "gazebo/rendering/Material.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/ogre_gazebo.h"

#include "gazebo/gui/model/LinkInspector.hh"
#include "gazebo/gui/model/ModelPluginInspector.hh"
#include "gazebo/gui/model/VisualConfig.hh"
#include "gazebo/gui/model/LinkConfig.hh"
#include "gazebo/gui/model/CollisionConfig.hh"

#include "gazebo/gui/GuiIface.hh"
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
void ModelData::UpdateRenderGroup(rendering::VisualPtr _visual)
{
  // fix for transparency alpha compositing
  if (_visual->GetSceneNode()->numAttachedObjects() <= 0)
    return;

  Ogre::MovableObject *obj = _visual->GetSceneNode()->
      getAttachedObject(0);
  obj->setRenderQueueGroup(obj->getRenderQueueGroup()+1);
}

/////////////////////////////////////////////////
void NestedModelData::SetName(const std::string &_name)
{
  this->modelSDF->GetAttribute("name")->Set(_name);
}

/////////////////////////////////////////////////
void NestedModelData::SetPose(const ignition::math::Pose3d &_pose)
{
  this->modelSDF->GetElement("pose")->Set(_pose);
}

/////////////////////////////////////////////////
ignition::math::Pose3d NestedModelData::Pose() const
{
  return this->modelSDF->Get<ignition::math::Pose3d>("pose");
}

/////////////////////////////////////////////////
int NestedModelData::Depth() const
{
  if (!this->modelVisual)
    return -1;

  return this->modelVisual->GetDepth();
}

/////////////////////////////////////////////////
LinkData::LinkData()
{
  this->linkSDF.reset(new sdf::Element);
  sdf::initFile("link.sdf", this->linkSDF);

  this->scale = ignition::math::Vector3d::One;
  this->inertiaIxx = 0;
  this->inertiaIyy = 0;
  this->inertiaIzz = 0;
  this->mass = 0;
  this->nested = false;

  this->inspector = new LinkInspector();
  this->inspector->setModal(false);
  connect(this->inspector, SIGNAL(Applied()), this, SLOT(OnApply()));
  connect(this->inspector, SIGNAL(Accepted()), this, SLOT(OnAccept()));
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
      boost::bind(&LinkData::Update, this)));
  this->updateMutex = new boost::recursive_mutex();
}

/////////////////////////////////////////////////
LinkData::~LinkData()
{
  event::Events::DisconnectPreRender(this->connections[0]);
  this->connections.clear();
  delete this->inspector;
  delete this->updateMutex;
}

/////////////////////////////////////////////////
std::string LinkData::GetName() const
{
  return this->linkSDF->Get<std::string>("name");
}

/////////////////////////////////////////////////
void LinkData::SetName(const std::string &_name)
{
  this->linkSDF->GetAttribute("name")->Set(_name);
  this->inspector->SetName(_name);
}

/////////////////////////////////////////////////
ignition::math::Pose3d LinkData::Pose() const
{
  return this->linkSDF->Get<ignition::math::Pose3d>("pose");
}

/////////////////////////////////////////////////
void LinkData::SetPose(const ignition::math::Pose3d &_pose)
{
  this->linkSDF->GetElement("pose")->Set(_pose);

  LinkConfig *linkConfig = this->inspector->GetLinkConfig();
  linkConfig->SetPose(_pose);
}

/////////////////////////////////////////////////
void LinkData::SetScale(const ignition::math::Vector3d &_scale)
{
  VisualConfig *visualConfig = this->inspector->GetVisualConfig();

  ignition::math::Vector3d dScale = _scale / this->scale;
  for (auto const &it : this->visuals)
  {
    std::string name = it.first->GetName();
    std::string linkName = this->linkVisual->GetName();
    std::string leafName =
        name.substr(name.find(linkName)+linkName.size()+2);
    ignition::math::Vector3d visOldSize;
    std::string uri;
    visualConfig->Geometry(leafName,  visOldSize, uri);
    ignition::math::Vector3d visNewSize = it.first->GetGeometrySize();
    visualConfig->SetGeometry(leafName, visNewSize);
  }

  std::map<std::string, ignition::math::Vector3d> colOldSizes;
  std::map<std::string, ignition::math::Vector3d> colNewSizes;
  CollisionConfig *collisionConfig = this->inspector->GetCollisionConfig();
  for (auto const &it : this->collisions)
  {
    std::string name = it.first->GetName();
    std::string linkName = this->linkVisual->GetName();
    std::string leafName =
        name.substr(name.find(linkName)+linkName.size()+2);

    ignition::math::Vector3d colOldSize;
    std::string uri;
    collisionConfig->Geometry(leafName,  colOldSize, uri);
    ignition::math::Vector3d colNewSize = it.first->GetGeometrySize();
    collisionConfig->SetGeometry(leafName, colNewSize);
    colOldSizes[name] = colOldSize;
    colNewSizes[name] = colNewSize;
  }

  if (this->collisions.empty())
    return;

  // update link inertial values - assume uniform density
  LinkConfig *linkConfig = this->inspector->GetLinkConfig();
  sdf::ElementPtr inertialElem = this->linkSDF->GetElement("inertial");

  // update mass
  // density = mass / volume
  // assume fixed density and scale mass based on volume changes.
  double volumeRatio = 1;
  double newVol = 0;
  double oldVol = 0;
  for (auto const &it : this->collisions)
  {
    ignition::math::Vector3d oldSize = colOldSizes[it.first->GetName()];
    ignition::math::Vector3d newSize = colNewSizes[it.first->GetName()];
    std::string geomStr = it.first->GetGeometryType();
    if (geomStr == "sphere")
    {
      double r = oldSize.X() * 0.5;
      double r3 = r*r*r;
      double newR = newSize.X() * 0.5;
      double newR3 = newR*newR*newR;
      // sphere volume: 4/3 * PI * r^3
      oldVol += 4.0 / 3.0 * IGN_PI * r3;
      newVol += 4.0 / 3.0 * IGN_PI * newR3;
    }
    else if (geomStr == "cylinder")
    {
      double r = oldSize.X() * 0.5;
      double r2 = r*r;
      double newR = newSize.X() * 0.5;
      double newR2 = newR*newR;
      // cylinder volume: PI * r^2 * height
      oldVol += IGN_PI * r2 * oldSize.Z();
      newVol += IGN_PI * newR2 * newSize.Z();
    }
    else
    {
      // box, mesh, and other geometry types - use bounding box
      oldVol += oldSize.X() * oldSize.Y() * oldSize.Z();
      newVol += newSize.X() * newSize.Y() * newSize.Z();
    }
  }

  if (oldVol < 1e-10)
  {
    gzerr << "Volume is too small to compute accurate inertial values"
        << std::endl;
    return;
  }

  volumeRatio = newVol / oldVol;

  // set new mass
  double oldMass = this->mass;
  double newMass = this->mass * volumeRatio;
  this->mass = newMass;
  linkConfig->SetMass(newMass);

  // scale the inertia values
  // 1) compute inertia size based on current inertia matrix and geometry
  // 2) apply scale to inertia size
  // 3) compute new inertia values based on new size

  // get current inertia values
  double ixx = this->inertiaIxx;
  double iyy = this->inertiaIyy;
  double izz = this->inertiaIzz;

  double newIxx = ixx;
  double newIyy = iyy;
  double newIzz = izz;

  ignition::math::Vector3d dInertiaScale;

  // we can compute better estimates of inertia values if the link only has
  // one collision made up of a simple shape
  // otherwise assume box geom
  bool boxInertia = false;
  if (this->collisions.size() == 1u)
  {
    auto const &it = this->collisions.begin();
    std::string geomStr = it->first->GetGeometryType();
    dInertiaScale = colNewSizes[it->first->GetName()] /
        colOldSizes[it->first->GetName()];
    if (geomStr == "sphere")
    {
      // solve for r^2
      double r2 = ixx / (oldMass * 0.4);

      // compute new inertia values based on new mass and radius
      newIxx = newMass * 0.4 * (dInertiaScale.X() * dInertiaScale.X()) * r2;
      newIyy = newIxx;
      newIzz = newIxx;
    }
    else if (geomStr == "cylinder")
    {
      // solve for r^2 and l^2
      double r2 = izz / (oldMass * 0.5);
      double l2 = (ixx / oldMass - 0.25 * r2) * 12.0;

      // compute new inertia values based on new mass, radius and length
      newIxx = newMass * (0.25 * (dInertiaScale.X() * dInertiaScale.X() * r2) +
          (dInertiaScale.Z() * dInertiaScale.Z() * l2) / 12.0);
      newIyy = newIxx;
      newIzz = newMass * 0.5 * (dInertiaScale.X() * dInertiaScale.X() * r2);
    }
    else
    {
      boxInertia = true;
    }
  }
  else
  {
    boxInertia = true;
  }

  if (boxInertia)
  {
    // solve for box inertia size: dx^2, dy^2, dz^2,
    // assuming solid box with uniform density
    double mc = 12.0 / oldMass;
    double ixxMc = ixx * mc;
    double iyyMc = iyy * mc;
    double izzMc = izz * mc;
    double dz2 = (iyyMc - izzMc + ixxMc) * 0.5;
    double dx2 = izzMc - (ixxMc - dz2);
    double dy2 = ixxMc - dz2;

    // scale inertia size
    double newDx2 = dInertiaScale.X() * dInertiaScale.X() * dx2;
    double newDy2 = dInertiaScale.Y() * dInertiaScale.Y() * dy2;
    double newDz2 = dInertiaScale.Z() * dInertiaScale.Z() * dz2;

    // compute new inertia values based on new inertia size
    double newMassConstant = newMass / 12.0;
    newIxx = newMassConstant * (newDy2 + newDz2);
    newIyy = newMassConstant * (newDx2 + newDz2);
    newIzz = newMassConstant * (newDx2 + newDy2);
  }

  // update inspector inertia
  linkConfig->SetInertiaMatrix(newIxx, newIyy, newIzz, 0, 0, 0);

  // update local inertal variables
  this->inertiaIxx = newIxx;
  this->inertiaIyy = newIyy;
  this->inertiaIzz = newIzz;

  // update sdf
  sdf::ElementPtr inertiaElem = inertialElem->GetElement("inertia");
  sdf::ElementPtr ixxElem = inertiaElem->GetElement("ixx");
  sdf::ElementPtr iyyElem = inertiaElem->GetElement("iyy");
  sdf::ElementPtr izzElem = inertiaElem->GetElement("izz");
  ixxElem->Set(newIxx);
  iyyElem->Set(newIyy);
  izzElem->Set(newIzz);

  sdf::ElementPtr massElem = inertialElem->GetElement("mass");
  massElem->Set(newMass);

  sdf::ElementPtr inertialPoseElem = inertialElem->GetElement("pose");
  ignition::math::Pose3d newPose =
      inertialPoseElem->Get<ignition::math::Pose3d>();
  newPose.Pos() *= dScale;

  inertialPoseElem->Set(newPose);
  linkConfig->SetInertialPose(newPose);

  this->scale = _scale;
}

/////////////////////////////////////////////////
ignition::math::Vector3d LinkData::Scale() const
{
  return this->scale;
}

/////////////////////////////////////////////////
void LinkData::Load(sdf::ElementPtr _sdf)
{
  LinkConfig *linkConfig = this->inspector->GetLinkConfig();

  this->SetName(_sdf->Get<std::string>("name"));
  this->SetPose(_sdf->Get<ignition::math::Pose3d>("pose"));

  msgs::LinkPtr linkMsgPtr(new msgs::Link);
  if (_sdf->HasElement("inertial"))
  {
    sdf::ElementPtr inertialElem = _sdf->GetElement("inertial");
    this->linkSDF->GetElement("inertial")->Copy(inertialElem);

    msgs::Inertial *inertialMsg = linkMsgPtr->mutable_inertial();

    if (inertialElem->HasElement("mass"))
    {
      this->mass = inertialElem->Get<double>("mass");
      inertialMsg->set_mass(this->mass);
    }

    if (inertialElem->HasElement("pose"))
    {
      ignition::math::Pose3d inertialPose =
        inertialElem->Get<ignition::math::Pose3d>("pose");
      msgs::Set(inertialMsg->mutable_pose(), inertialPose);
    }

    if (inertialElem->HasElement("inertia"))
    {
      sdf::ElementPtr inertiaElem = inertialElem->GetElement("inertia");
      this->inertiaIxx = inertiaElem->Get<double>("ixx");
      this->inertiaIyy = inertiaElem->Get<double>("iyy");
      this->inertiaIzz = inertiaElem->Get<double>("izz");
      inertialMsg->set_ixx(this->inertiaIxx);
      inertialMsg->set_iyy(this->inertiaIyy);
      inertialMsg->set_izz(this->inertiaIzz);
      inertialMsg->set_ixy(inertiaElem->Get<double>("ixy"));
      inertialMsg->set_ixz(inertiaElem->Get<double>("ixz"));
      inertialMsg->set_iyz(inertiaElem->Get<double>("iyz"));
    }
  }
  if (_sdf->HasElement("self_collide"))
  {
    sdf::ElementPtr selfCollideSDF = _sdf->GetElement("self_collide");
    linkMsgPtr->set_self_collide(selfCollideSDF->Get<bool>(""));
    this->linkSDF->InsertElement(selfCollideSDF->Clone());
  }
  if (_sdf->HasElement("kinematic"))
  {
    sdf::ElementPtr kinematicSDF = _sdf->GetElement("kinematic");
    linkMsgPtr->set_kinematic(kinematicSDF->Get<bool>());
    this->linkSDF->InsertElement(kinematicSDF->Clone());
  }
  if (_sdf->HasElement("must_be_base_link"))
  {
    sdf::ElementPtr baseLinkSDF = _sdf->GetElement("must_be_base_link");
    // TODO link.proto is missing the must_be_base_link field.
    // linkMsgPtr->set_must_be_base_link(baseLinkSDF->Get<bool>());
    this->linkSDF->InsertElement(baseLinkSDF->Clone());
  }
  if (_sdf->HasElement("velocity_decay"))
  {
    sdf::ElementPtr velocityDecaySDF = _sdf->GetElement("velocity_decay");
    // TODO link.proto is missing the velocity_decay field.
    // linkMsgPtr->set_velocity_decay(velocityDecaySDF->Get<double>());
    this->linkSDF->InsertElement(velocityDecaySDF->Clone());
  }
  linkConfig->Update(linkMsgPtr);

  if (_sdf->HasElement("sensor"))
  {
    sdf::ElementPtr sensorElem = _sdf->GetElement("sensor");
    while (sensorElem)
    {
      this->linkSDF->InsertElement(sensorElem->Clone());
      sensorElem = sensorElem->GetNextElement("sensor");
    }
  }
}

/////////////////////////////////////////////////
void LinkData::UpdateConfig()
{
  // set new geom size if scale has changed.
  VisualConfig *visualConfig = this->inspector->GetVisualConfig();
  for (auto &it : this->visuals)
  {
    std::string name = it.first->GetName();
    std::string leafName = name;
    size_t idx = name.rfind("::");
    if (idx != std::string::npos)
      leafName = name.substr(idx+2);
    visualConfig->SetGeometry(leafName, it.first->GetGeometrySize(),
        it.first->GetMeshName());

    msgs::Visual *updateMsg = visualConfig->GetData(leafName);
    msgs::Visual visualMsg = it.second;
    updateMsg->clear_scale();
    msgs::Material *matMsg = updateMsg->mutable_material();
    // clear empty colors so they are not used by visual updates
    common::Color emptyColor;
    if (msgs::Convert(matMsg->ambient()) == emptyColor)
      matMsg->clear_ambient();
    if (msgs::Convert(matMsg->diffuse()) == emptyColor)
      matMsg->clear_diffuse();
    if (msgs::Convert(matMsg->specular()) == emptyColor)
      matMsg->clear_specular();
    if (msgs::Convert(matMsg->emissive()) == emptyColor)
      matMsg->clear_emissive();

    if (matMsg->has_diffuse())
      matMsg->mutable_diffuse()->set_a(1.0-updateMsg->transparency());

    visualMsg.CopyFrom(*updateMsg);
    it.second = visualMsg;
  }
  CollisionConfig *collisionConfig = this->inspector->GetCollisionConfig();
  for (auto &colIt : this->collisions)
  {
    std::string name = colIt.first->GetName();
    std::string leafName = name;
    size_t idx = name.rfind("::");
    if (idx != std::string::npos)
      leafName = name.substr(idx+2);
    collisionConfig->SetGeometry(leafName, colIt.first->GetGeometrySize(),
        colIt.first->GetMeshName());

    msgs::Collision *updateMsg = collisionConfig->GetData(leafName);
    msgs::Collision collisionMsg = colIt.second;
    collisionMsg.CopyFrom(*updateMsg);
    colIt.second = collisionMsg;
  }
}

/////////////////////////////////////////////////
void LinkData::AddVisual(rendering::VisualPtr _visual)
{
  VisualConfig *visualConfig = this->inspector->GetVisualConfig();
  msgs::Visual visualMsg = msgs::VisualFromSDF(_visual->GetSDF());

  this->visuals[_visual] = visualMsg;

  std::string visName = _visual->GetName();
  std::string leafName = visName;
  size_t idx = visName.rfind("::");
  if (idx != std::string::npos)
    leafName = visName.substr(idx+2);

  visualConfig->AddVisual(leafName, &visualMsg);
}

/////////////////////////////////////////////////
void LinkData::AddCollision(rendering::VisualPtr _collisionVis,
    const msgs::Collision *_msg)
{
  CollisionConfig *collisionConfig = this->inspector->GetCollisionConfig();

  sdf::ElementPtr collisionSDF(new sdf::Element);
  sdf::initFile("collision.sdf", collisionSDF);

  std::string visName = _collisionVis->GetName();
  std::string leafName = visName;
  size_t idx = visName.rfind("::");
  if (idx != std::string::npos)
    leafName = visName.substr(idx+2);

  msgs::Collision collisionMsg;
  // Use input message
  if (_msg)
  {
    collisionMsg = *_msg;
  }
  // Get data from input visual
  else
  {
    msgs::Visual visualMsg = msgs::VisualFromSDF(_collisionVis->GetSDF());
    collisionMsg.set_name(leafName);
    msgs::Geometry *geomMsg = collisionMsg.mutable_geometry();
    geomMsg->CopyFrom(visualMsg.geometry());
    msgs::Pose *poseMsg = collisionMsg.mutable_pose();
    poseMsg->CopyFrom(visualMsg.pose());
  }

  this->collisions[_collisionVis] = collisionMsg;
  collisionConfig->AddCollision(leafName, &collisionMsg);
}

/////////////////////////////////////////////////
LinkData *LinkData::Clone(const std::string &_newName)
{
  LinkData *cloneLink = new LinkData();

  cloneLink->Load(this->linkSDF);
  cloneLink->SetName(_newName);

  std::string linkVisualName = this->linkVisual->GetName();
  std::string cloneVisName = _newName;
  size_t linkIdx = linkVisualName.find("::");
  if (linkIdx != std::string::npos)
    cloneVisName = linkVisualName.substr(0, linkIdx+2) + _newName;

  // clone linkVisual;
  rendering::VisualPtr linkVis(new rendering::Visual(cloneVisName,
      this->linkVisual->GetParent()));
  linkVis->Load();

  cloneLink->linkVisual = linkVis;

  for (auto &visIt : this->visuals)
  {
    std::string newVisName = visIt.first->GetName();
    size_t idx = newVisName.rfind("::");
    std::string leafName = newVisName.substr(idx+2);
    if (idx != std::string::npos)
      newVisName = cloneVisName + "::" + leafName;
    else
      newVisName = cloneVisName + "::" + newVisName;

    rendering::VisualPtr cloneVis =
        visIt.first->Clone(newVisName, cloneLink->linkVisual);

    // store the leaf name in sdf not the full scoped name
    cloneVis->GetSDF()->GetAttribute("name")->Set(leafName);

    // override transparency
    cloneVis->SetTransparency(visIt.second.transparency());
    cloneLink->AddVisual(cloneVis);
    cloneVis->SetTransparency(visIt.second.transparency() *
        (1-ModelData::GetEditTransparency()-0.1)
        + ModelData::GetEditTransparency());
  }

  for (auto &colIt : this->collisions)
  {
    std::string newColName = colIt.first->GetName();
    size_t idx = newColName.rfind("::");
    std::string leafName = newColName.substr(idx+2);
    if (idx != std::string::npos)
      newColName = cloneVisName + "::" + leafName;
    else
      newColName = cloneVisName + "::" + newColName;
    rendering::VisualPtr collisionVis = colIt.first->Clone(newColName,
        cloneLink->linkVisual);

    // store the leaf name in sdf not the full scoped name
    collisionVis->GetSDF()->GetAttribute("name")->Set(leafName);

    collisionVis->SetTransparency(
       ignition::math::clamp(ModelData::GetEditTransparency() * 2.0, 0.0, 0.8));
    ModelData::UpdateRenderGroup(collisionVis);
    cloneLink->AddCollision(collisionVis);
  }
  return cloneLink;
}

/////////////////////////////////////////////////
void LinkData::OnAccept()
{
  if (this->Apply())
    this->inspector->accept();
}

/////////////////////////////////////////////////
void LinkData::OnApply()
{
  this->Apply();
}

/////////////////////////////////////////////////
bool LinkData::Apply()
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
  LinkConfig *linkConfig = this->inspector->GetLinkConfig();

  msgs::Link *linkMsg = linkConfig->GetData();

  // update link sdf
  this->linkSDF = msgs::LinkToSDF(*linkMsg, this->linkSDF);

  // update internal variables
  msgs::Inertial *inertialMsg = linkMsg->mutable_inertial();
  this->mass = inertialMsg->mass();
  this->inertiaIxx = inertialMsg->ixx();
  this->inertiaIyy = inertialMsg->iyy();
  this->inertiaIzz = inertialMsg->izz();

  // update link visual pose
  this->linkVisual->SetPose(this->Pose());

  std::vector<msgs::Visual *> visualUpdateMsgsTemp;
  std::vector<msgs::Collision *> collisionUpdateMsgsTemp;

  // update visuals
  if (!this->visuals.empty())
  {
    VisualConfig *visualConfig = this->inspector->GetVisualConfig();
    for (auto &it : this->visuals)
    {
      std::string name = it.first->GetName();
      std::string leafName = name;
      size_t idx = name.rfind("::");
      if (idx != std::string::npos)
        leafName = name.substr(idx+2);
      msgs::Visual *updateMsg = visualConfig->GetData(leafName);
      if (updateMsg)
      {
        msgs::Visual visualMsg = it.second;

        // check if the geometry is valid
        msgs::Geometry *geomMsg = updateMsg->mutable_geometry();

        // warnings when changing from/to polyline
        for (auto &vis : this->visuals)
        {
          if (vis.second.name() != updateMsg->name())
            continue;

          if (vis.second.mutable_geometry()->type() != geomMsg->type())
          {
            // Changing from polyline, give option to cancel
            if (vis.second.mutable_geometry()->type() ==
                msgs::Geometry::POLYLINE)
            {
              std::string msg =
                  "Once you change the geometry, you can't go "
                  "back to polyline.\n\n"
                  "Do you wish to continue?\n";

              QMessageBox msgBox(QMessageBox::Warning,
                  QString("Changing polyline geometry"), QString(msg.c_str()));

              QPushButton *cancelButton =
                  msgBox.addButton("Cancel", QMessageBox::RejectRole);
              QPushButton *saveButton = msgBox.addButton("Ok",
                  QMessageBox::AcceptRole);
              msgBox.setDefaultButton(saveButton);
              msgBox.setEscapeButton(cancelButton);
              msgBox.exec();
              if (msgBox.clickedButton() != saveButton)
                return false;
            }
            // Changing to polyline: not allowed
            else if (geomMsg->type() == msgs::Geometry::POLYLINE)
            {
              std::string msg =
                  "It's not possible to change into polyline.\n"
                  "Please select another geometry type for ["
                  + leafName + "].";

              QMessageBox::warning(linkConfig, QString(
                  "Invalid geometry conversion"), QString(msg.c_str()),
                  QMessageBox::Ok, QMessageBox::Ok);
              return false;
            }
          }
        }

        if (geomMsg->type() == msgs::Geometry::MESH)
        {
          msgs::MeshGeom *meshGeom = geomMsg->mutable_mesh();
          QFileInfo info(QString::fromStdString(meshGeom->filename()));
          if (!info.isFile() || (info.completeSuffix().toLower() != "dae" &&
              info.completeSuffix().toLower() != "stl"))
          {
            std::string msg = "\"" + meshGeom->filename() +
                "\" is not a valid mesh file.\nPlease select another file for ["
                + leafName + "].";

            QMessageBox::warning(linkConfig, QString("Invalid Mesh File"),
                QString(msg.c_str()), QMessageBox::Ok, QMessageBox::Ok);
            return false;
          }
        }

        // update the visualMsg that will be used to generate the sdf.
        updateMsg->clear_scale();
        msgs::Material *matMsg = updateMsg->mutable_material();
        msgs::Material::Script *scriptMsg = matMsg->mutable_script();

        common::Color emptyColor;
        common::Color matAmbient;
        common::Color matDiffuse;
        common::Color matSpecular;
        common::Color matEmissive;
        rendering::Material::GetMaterialAsColor(scriptMsg->name(), matAmbient,
            matDiffuse, matSpecular, matEmissive);

        common::Color ambient = msgs::Convert(matMsg->ambient());
        common::Color diffuse = msgs::Convert(matMsg->diffuse());
        common::Color specular = msgs::Convert(matMsg->specular());
        common::Color emissive = msgs::Convert(matMsg->emissive());

        if (ambient == emptyColor)
        {
          matMsg->clear_ambient();
          ambient = matAmbient;
        }
        if (diffuse == emptyColor)
        {
          matMsg->clear_diffuse();
          diffuse = matDiffuse;
        }
        if (specular == emptyColor)
        {
          matMsg->clear_specular();
          specular = matSpecular;
        }
        if (emissive == emptyColor)
        {
          matMsg->clear_emissive();
          emissive = matEmissive;
        }

        visualConfig->SetMaterial(leafName, scriptMsg->name(), ambient,
            diffuse, specular, emissive);

        visualMsg.CopyFrom(*updateMsg);
        it.second = visualMsg;

        visualUpdateMsgsTemp.push_back(updateMsg);
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
      std::string leafName = name;
      size_t idx = name.rfind("::");
      if (idx != std::string::npos)
        leafName = name.substr(idx+2);
      msgs::Collision *updateMsg = collisionConfig->GetData(leafName);
      if (updateMsg)
      {
        msgs::Collision collisionMsg = it.second;

        // check if the geometry is valid
        msgs::Geometry *geomMsg = updateMsg->mutable_geometry();

        // warnings when changing from/to polyline
        for (auto &col : this->collisions)
        {
          if (col.second.name() != updateMsg->name())
            continue;

          if (col.second.mutable_geometry()->type() != geomMsg->type())
          {
            // Changing from polyline, give option to cancel
            if (col.second.mutable_geometry()->type() ==
                msgs::Geometry::POLYLINE)
            {
              std::string msg =
                  "Once you change the geometry, you can't go "
                  "back to polyline.\n\n"
                  "Do you wish to continue?\n";

              QMessageBox msgBox(QMessageBox::Warning,
                  QString("Changing polyline geometry"), QString(msg.c_str()));

              QPushButton *cancelButton =
                  msgBox.addButton("Cancel", QMessageBox::RejectRole);
              QPushButton *saveButton = msgBox.addButton("Ok",
                  QMessageBox::AcceptRole);
              msgBox.setDefaultButton(saveButton);
              msgBox.setEscapeButton(cancelButton);
              msgBox.exec();
              if (msgBox.clickedButton() != saveButton)
                return false;
            }
            // Changing to polyline: not allowed
            else if (geomMsg->type() == msgs::Geometry::POLYLINE)
            {
              std::string msg =
                  "It's not possible to change into polyline.\n"
                  "Please select another geometry type for ["
                  + leafName + "].";

              QMessageBox::warning(linkConfig, QString(
                  "Invalid geometry conversion"), QString(msg.c_str()),
                  QMessageBox::Ok, QMessageBox::Ok);
              return false;
            }
          }
        }

        if (geomMsg->type() == msgs::Geometry::MESH)
        {
          msgs::MeshGeom *meshGeom = geomMsg->mutable_mesh();
          QFileInfo info(QString::fromStdString(meshGeom->filename()));
          if (!info.isFile() || (info.completeSuffix().toLower() != "dae" &&
              info.completeSuffix().toLower() != "stl"))
          {
            std::string msg = "\"" + meshGeom->filename() +
                "\" is not a valid mesh file.\nPlease select another file for ["
                + leafName + "].";

            QMessageBox::warning(linkConfig, QString("Invalid Mesh File"),
                QString(msg.c_str()), QMessageBox::Ok, QMessageBox::Ok);
            return false;
          }
        }

        collisionMsg.CopyFrom(*updateMsg);
        it.second = collisionMsg;

        collisionUpdateMsgsTemp.push_back(updateMsg);
      }
    }
  }

  // Only send update messages if all visuals and collisions are valid
  this->visualUpdateMsgs.insert(this->visualUpdateMsgs.end(),
      visualUpdateMsgsTemp.begin(), visualUpdateMsgsTemp.end());
  this->collisionUpdateMsgs.insert(this->collisionUpdateMsgs.end(),
      collisionUpdateMsgsTemp.begin(), collisionUpdateMsgsTemp.end());
  return true;
}

/////////////////////////////////////////////////
void LinkData::OnAddVisual(const std::string &_name)
{
  // add a visual when the user adds a visual via the inspector's visual tab
  VisualConfig *visualConfig = this->inspector->GetVisualConfig();

  std::ostringstream visualName;
  visualName << this->linkVisual->GetName() << "::" << _name;

  rendering::VisualPtr visVisual;
  rendering::VisualPtr refVisual;
  if (!this->visuals.empty())
  {
    // add new visual by cloning last instance
    refVisual = this->visuals.rbegin()->first;
    visVisual = refVisual->Clone(visualName.str(), this->linkVisual);
  }
  else
  {
    // create new visual based on sdf template (box)
    sdf::SDFPtr modelTemplateSDF(new sdf::SDF);
    modelTemplateSDF->SetFromString(
        ModelData::GetTemplateSDFString());

    visVisual.reset(new rendering::Visual(visualName.str(),
        this->linkVisual));
    sdf::ElementPtr visualElem =  modelTemplateSDF->Root()
        ->GetElement("model")->GetElement("link")->GetElement("visual");
    visVisual->Load(visualElem);
  }

  msgs::Visual visualMsg = msgs::VisualFromSDF(visVisual->GetSDF());
  // store the correct transparency setting
  if (refVisual)
    visualMsg.set_transparency(this->visuals[refVisual].transparency());

  msgs::VisualPtr visualMsgPtr(new msgs::Visual);
  visualMsgPtr->CopyFrom(visualMsg);
  visualConfig->UpdateVisual(_name, visualMsgPtr);
  this->visuals[visVisual] = visualMsg;
  visVisual->SetTransparency(visualMsg.transparency() *
      (1-ModelData::GetEditTransparency()-0.1)
      + ModelData::GetEditTransparency());
}

/////////////////////////////////////////////////
void LinkData::OnAddCollision(const std::string &_name)
{
  // add a collision when the user adds a collision via the inspector's
  // collision tab
  CollisionConfig *collisionConfig = this->inspector->GetCollisionConfig();

  std::stringstream collisionName;
  collisionName << this->linkVisual->GetName() << "::" << _name;

  rendering::VisualPtr collisionVis;
  if (!this->collisions.empty())
  {
    // add new collision by cloning last instance
    collisionVis = this->collisions.rbegin()->first->Clone(collisionName.str(),
        this->linkVisual);
  }
  else
  {
    // create new collision based on sdf template (box)
    sdf::SDFPtr modelTemplateSDF(new sdf::SDF);
    modelTemplateSDF->SetFromString(
        ModelData::GetTemplateSDFString());

    collisionVis.reset(new rendering::Visual(collisionName.str(),
        this->linkVisual));
    sdf::ElementPtr collisionElem =  modelTemplateSDF->Root()
        ->GetElement("model")->GetElement("link")->GetElement("visual");
    collisionVis->Load(collisionElem);
    collisionVis->SetMaterial("Gazebo/Orange");
  }

  msgs::Visual visualMsg = msgs::VisualFromSDF(collisionVis->GetSDF());
  msgs::Collision collisionMsg;
  collisionMsg.set_name(_name);
  msgs::Geometry *geomMsg = collisionMsg.mutable_geometry();
  geomMsg->CopyFrom(visualMsg.geometry());

  msgs::CollisionPtr collisionMsgPtr(new msgs::Collision);
  collisionMsgPtr->CopyFrom(collisionMsg);
  collisionConfig->UpdateCollision(_name, collisionMsgPtr);
  this->collisions[collisionVis] = collisionMsg;

  collisionVis->SetTransparency(
      ignition::math::clamp(ModelData::GetEditTransparency() * 2.0, 0.0, 0.8));
  ModelData::UpdateRenderGroup(collisionVis);
}

/////////////////////////////////////////////////
void LinkData::OnRemoveVisual(const std::string &_name)
{
  // find and remove visual when the user removes it in the
  // inspector's visual tab
  std::ostringstream name;
  name << this->linkVisual->GetName() << "::" << _name;
  std::string visualName = name.str();

  for (auto it = this->visuals.begin(); it != this->visuals.end(); ++it)
  {
    if (visualName == it->first->GetName())
    {
      this->linkVisual->DetachVisual(it->first);
      this->linkVisual->GetScene()->RemoveVisual(it->first);
      this->visuals.erase(it);
      break;
    }
  }
}

/////////////////////////////////////////////////
void LinkData::OnRemoveCollision(const std::string &_name)
{
  // find and remove collision visual when the user removes it in the
  // inspector's collision tab
  std::ostringstream name;
  name << this->linkVisual->GetName() << "::" << _name;
  std::string collisionName = name.str();

  for (auto it = this->collisions.begin(); it != this->collisions.end(); ++it)
  {
    if (collisionName == it->first->GetName())
    {
      this->linkVisual->DetachVisual(it->first);
      this->linkVisual->GetScene()->RemoveVisual(it->first);
      this->collisions.erase(it);
      break;
    }
  }
}

/////////////////////////////////////////////////
void LinkData::Update()
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

/////////////////////////////////////////////////
ModelPluginData::ModelPluginData()
{
  // Initialize SDF
  this->modelPluginSDF.reset(new sdf::Element);
  sdf::initFile("plugin.sdf", this->modelPluginSDF);

  // Inspector
  this->inspector = new ModelPluginInspector();
}

/////////////////////////////////////////////////
ModelPluginData::~ModelPluginData()
{
  delete this->inspector;
}

/////////////////////////////////////////////////
void ModelPluginData::Load(sdf::ElementPtr _pluginElem)
{
  this->modelPluginSDF = _pluginElem;

  // Convert SDF to msg
  msgs::Plugin pluginMsg = msgs::PluginFromSDF(_pluginElem);
  msgs::PluginPtr pluginPtr(new msgs::Plugin);
  pluginPtr->CopyFrom(pluginMsg);

  // Update inspector
  this->inspector->Update(pluginPtr);
}
