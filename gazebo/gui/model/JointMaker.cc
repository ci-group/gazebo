/*
 * Copyright (C) 2014-2015 Open Source Robotics Foundation
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
#include <string>
#include <vector>

#include "gazebo/common/MouseEvent.hh"

#include "gazebo/rendering/ogre_gazebo.h"
#include "gazebo/rendering/DynamicLines.hh"
#include "gazebo/rendering/Visual.hh"
#include "gazebo/rendering/JointVisual.hh"
#include "gazebo/rendering/UserCamera.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/RenderTypes.hh"

#include "gazebo/gui/GuiIface.hh"
#include "gazebo/gui/KeyEventHandler.hh"
#include "gazebo/gui/MouseEventHandler.hh"
#include "gazebo/gui/GuiEvents.hh"
#include "gazebo/gui/MainWindow.hh"

#include "gazebo/gui/model/JointInspector.hh"
#include "gazebo/gui/model/ModelEditorEvents.hh"
#include "gazebo/gui/model/JointMaker.hh"

using namespace gazebo;
using namespace gui;

std::map<JointMaker::JointType, std::string> JointMaker::jointTypes;

/////////////////////////////////////////////////
JointMaker::JointMaker()
{
  this->UnitVectors.push_back(math::Vector3::UnitX);
  this->UnitVectors.push_back(math::Vector3::UnitY);
  this->UnitVectors.push_back(math::Vector3::UnitZ);

  this->newJointCreated = false;
  this->mouseJoint = NULL;
  this->modelSDF.reset();
  this->jointType = JointMaker::JOINT_NONE;
  this->jointCounter = 0;

  this->jointMaterials[JOINT_FIXED]     = "Gazebo/Red";
  this->jointMaterials[JOINT_HINGE]     = "Gazebo/Orange";
  this->jointMaterials[JOINT_HINGE2]    = "Gazebo/Yellow";
  this->jointMaterials[JOINT_SLIDER]    = "Gazebo/Green";
  this->jointMaterials[JOINT_SCREW]     = "Gazebo/Black";
  this->jointMaterials[JOINT_UNIVERSAL] = "Gazebo/Blue";
  this->jointMaterials[JOINT_BALL]      = "Gazebo/Purple";


  jointTypes[JOINT_FIXED]     = "fixed";
  jointTypes[JOINT_HINGE]     = "revolute";
  jointTypes[JOINT_HINGE2]    = "revolute2";
  jointTypes[JOINT_SLIDER]    = "prismatic";
  jointTypes[JOINT_SCREW]     = "screw";
  jointTypes[JOINT_UNIVERSAL] = "universal";
  jointTypes[JOINT_BALL]      = "ball";
  jointTypes[JOINT_NONE]      = "none";

  this->connections.push_back(
      event::Events::ConnectPreRender(
        boost::bind(&JointMaker::Update, this)));

  this->connections.push_back(
      gui::model::Events::ConnectOpenJointInspector(
      boost::bind(&JointMaker::OpenInspector, this, _1)));

  this->inspectAct = new QAction(tr("Open Joint Inspector"), this);
  connect(this->inspectAct, SIGNAL(triggered()), this, SLOT(OnOpenInspector()));

  this->updateMutex = new boost::recursive_mutex();
}

/////////////////////////////////////////////////
JointMaker::~JointMaker()
{
  if (this->mouseJoint)
  {
    delete this->mouseJoint;
    this->mouseJoint = NULL;
  }
  {
    boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
    while (this->joints.size() > 0)
    {
      this->RemoveJoint(this->joints.begin()->first);
    }
    this->joints.clear();
  }

  delete this->updateMutex;
}

/////////////////////////////////////////////////
void JointMaker::Reset()
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
  this->newJointCreated = false;
  if (this->mouseJoint)
  {
    delete this->mouseJoint;
    this->mouseJoint = NULL;
  }

  this->jointType = JointMaker::JOINT_NONE;
  this->selectedVis.reset();
  this->hoverVis.reset();
  this->prevHoverVis.reset();
  this->inspectVis.reset();
  this->selectedJoint.reset();

  this->scopedLinkedNames.clear();

  while (!this->joints.empty())
    this->RemoveJoint(this->joints.begin()->first);
  this->joints.clear();
}

/////////////////////////////////////////////////
void JointMaker::EnableEventHandlers()
{
  MouseEventHandler::Instance()->AddDoubleClickFilter("model_joint",
    boost::bind(&JointMaker::OnMouseDoubleClick, this, _1));

  MouseEventHandler::Instance()->AddReleaseFilter("model_joint",
      boost::bind(&JointMaker::OnMouseRelease, this, _1));

  MouseEventHandler::Instance()->AddPressFilter("model_joint",
      boost::bind(&JointMaker::OnMousePress, this, _1));

  KeyEventHandler::Instance()->AddPressFilter("model_joint",
      boost::bind(&JointMaker::OnKeyPress, this, _1));
}

/////////////////////////////////////////////////
void JointMaker::DisableEventHandlers()
{
  MouseEventHandler::Instance()->RemoveDoubleClickFilter("model_joint");
  MouseEventHandler::Instance()->RemoveReleaseFilter("model_joint");
  MouseEventHandler::Instance()->RemovePressFilter("model_joint");
  MouseEventHandler::Instance()->RemoveMoveFilter("model_joint");
  KeyEventHandler::Instance()->RemovePressFilter("model_joint");
}

/////////////////////////////////////////////////
void JointMaker::RemoveJoint(const std::string &_jointId)
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
  auto jointIt = this->joints.find(_jointId);
  if (jointIt != this->joints.end())
  {
    JointData *joint = jointIt->second;
    rendering::ScenePtr scene = joint->hotspot->GetScene();
    scene->GetManager()->destroyBillboardSet(joint->handles);
    scene->RemoveVisual(joint->hotspot);
    scene->RemoveVisual(joint->visual);
    joint->visual->Fini();
    if (joint->jointVisual)
    {
      rendering::JointVisualPtr parentAxisVis = joint->jointVisual
          ->GetParentAxisVisual();
      if (parentAxisVis)
      {
        parentAxisVis->GetParent()->DetachVisual(
            parentAxisVis->GetName());
        scene->RemoveVisual(parentAxisVis);
      }
      joint->jointVisual->GetParent()->DetachVisual(
          joint->jointVisual->GetName());
      scene->RemoveVisual(joint->jointVisual);
    }
    joint->hotspot.reset();
    joint->visual.reset();
    joint->jointVisual.reset();
    joint->parent.reset();
    joint->child.reset();
    joint->inspector->hide();
    delete joint->inspector;
    delete joint;
    this->joints.erase(jointIt);
    gui::model::Events::modelChanged();
    gui::model::Events::jointRemoved(_jointId);
  }
}

/////////////////////////////////////////////////
void JointMaker::RemoveJointsByLink(const std::string &_linkName)
{
  std::vector<std::string> toDelete;
  for (auto it : this->joints)
  {
    JointData *joint = it.second;

    if (joint->child->GetName() == _linkName ||
        joint->parent->GetName() == _linkName)
    {
      toDelete.push_back(it.first);
    }
  }

  for (unsigned i = 0; i < toDelete.size(); ++i)
    this->RemoveJoint(toDelete[i]);

  toDelete.clear();
}

/////////////////////////////////////////////////
std::vector<JointData *> JointMaker::GetJointDataByLink(
    const std::string &_linkName) const
{
  std::vector<JointData *> linkJoints;
  for (auto jointIt : this->joints)
  {
    JointData *jointData = jointIt.second;

    if (jointData->child->GetName() == _linkName ||
        jointData->parent->GetName() == _linkName)
    {
      linkJoints.push_back(jointData);
    }
  }
  return linkJoints;
}

/////////////////////////////////////////////////
bool JointMaker::OnMousePress(const common::MouseEvent &_event)
{
  rendering::UserCameraPtr camera = gui::get_active_camera();
  if (_event.button == common::MouseEvent::MIDDLE)
  {
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    camera->HandleMouseEvent(_event);
    return true;
  }
  else if (_event.button != common::MouseEvent::LEFT)
    return false;

  if (this->jointType != JointMaker::JOINT_NONE)
    return false;

  // intercept mouse press events when user clicks on the joint hotspot visual
  rendering::VisualPtr vis = camera->GetVisual(_event.pos);
  if (vis)
  {
    if (this->joints.find(vis->GetName()) != this->joints.end())
    {
      // stop event propagation as we don't want users to manipulate the
      // hotspot
      return true;
    }
  }
  return false;
}

/////////////////////////////////////////////////
bool JointMaker::OnMouseRelease(const common::MouseEvent &_event)
{
  rendering::UserCameraPtr camera = gui::get_active_camera();
  if (this->jointType == JointMaker::JOINT_NONE)
  {
    rendering::VisualPtr vis = camera->GetVisual(_event.pos);
    if (vis)
    {
      if (this->selectedJoint)
        this->selectedJoint->SetHighlighted(false);
      this->selectedJoint.reset();
      if (this->joints.find(vis->GetName()) != this->joints.end())
      {
        // trigger joint inspector on right click
        if (_event.button == common::MouseEvent::RIGHT)
        {
          this->inspectVis = vis;
          QMenu menu;
          menu.addAction(this->inspectAct);
          menu.exec(QCursor::pos());
        }
        else if (_event.button == common::MouseEvent::LEFT)
        {
          this->selectedJoint = vis;
          this->selectedJoint->SetHighlighted(true);
        }
      }
      return false;
    }
  }
  else
  {
    if (_event.button == common::MouseEvent::LEFT)
    {
      if (this->hoverVis)
      {
        if (this->hoverVis->IsPlane())
        {
          QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
          camera->HandleMouseEvent(_event);
          return true;
        }

        // Pressed parent link
        if (!this->selectedVis)
        {
          if (this->mouseJoint)
            return false;

          this->hoverVis->SetEmissive(common::Color(0, 0, 0));
          this->selectedVis = this->hoverVis;
          this->hoverVis.reset();

          // Create joint data with selected visual as parent
          // the child will be set on the second mouse release.
          this->mouseJoint = this->CreateJointLine("JOINT_LINE",
              this->selectedVis);
        }
        // Pressed child link
        else if (this->selectedVis != this->hoverVis)
        {
          if (this->hoverVis)
            this->hoverVis->SetEmissive(common::Color(0, 0, 0));
          if (this->selectedVis)
            this->selectedVis->SetEmissive(common::Color(0, 0, 0));

          this->mouseJoint->child = this->hoverVis;
          JointData *newJoint = this->CreateJoint(this->mouseJoint->parent,
              this->mouseJoint->child);
          this->Stop();
          this->mouseJoint = newJoint;
          this->newJointCreated = true;
          gui::model::Events::modelChanged();
        }
      }
    }

    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    camera->HandleMouseEvent(_event);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
JointData *JointMaker::CreateJointLine(const std::string &_name,
    rendering::VisualPtr _parent)
{
  rendering::VisualPtr jointVis(
      new rendering::Visual(_name, _parent->GetParent()));
  jointVis->Load();
  rendering::DynamicLines *jointLine =
      jointVis->CreateDynamicLine(rendering::RENDERING_LINE_LIST);
  math::Vector3 origin = _parent->GetWorldPose().pos
      - _parent->GetParent()->GetWorldPose().pos;
  jointLine->AddPoint(origin);
  jointLine->AddPoint(origin + math::Vector3(0, 0, 0.1));
  jointVis->GetSceneNode()->setInheritScale(false);
  jointVis->GetSceneNode()->setInheritOrientation(false);

  std::string jointVisName = jointVis->GetName();
  std::string leafName = jointVisName;
  size_t pIdx = jointVisName.find_last_of("::");
  if (pIdx != std::string::npos)
    leafName = jointVisName.substr(pIdx+1);

  JointData *jointData = new JointData();
  jointData->dirty = false;
  jointData->name = leafName;
  jointData->visual = jointVis;
  jointData->parent = _parent;
  jointData->line = jointLine;
  jointData->type = this->jointType;
  jointData->line->setMaterial(this->jointMaterials[jointData->type]);

  return jointData;
}

/////////////////////////////////////////////////
JointData *JointMaker::CreateJoint(rendering::VisualPtr _parent,
    rendering::VisualPtr _child)
{
  std::stringstream ss;
  ss << _parent->GetName() << "_JOINT_" << this->jointCounter++;

  JointData *jointData = this->CreateJointLine(ss.str(), _parent);
  jointData->child = _child;

  jointData->inspector = new JointInspector();
  jointData->inspector->setModal(false);
  connect(jointData->inspector, SIGNAL(Applied()),
      jointData, SLOT(OnApply()));

  MainWindow *mainWindow = gui::get_main_window();
  if (mainWindow)
  {
    connect(gui::get_main_window(), SIGNAL(Close()), jointData->inspector,
        SLOT(close()));
  }

  // setup the joint msg
  jointData->jointMsg.reset(new msgs::Joint);
  jointData->jointMsg->set_name(jointData->name);
  if (jointData->parent)
  {
    std::string jointParentName = jointData->parent->GetName();
    std::string leafName = jointParentName;
    size_t pIdx = jointParentName.find_last_of("::");
    if (pIdx != std::string::npos)
      leafName = jointParentName.substr(pIdx+1);

    jointData->jointMsg->set_parent(leafName);
    jointData->jointMsg->set_parent_id(jointData->parent->GetId());
  }
  if (jointData->child)
  {
    std::string jointChildName = jointData->child->GetName();
    std::string leafName = jointChildName;
    size_t pIdx = jointChildName.find_last_of("::");
    if (pIdx != std::string::npos)
      leafName = jointChildName.substr(pIdx+1);

    jointData->jointMsg->set_child(leafName);
    jointData->jointMsg->set_child_id(jointData->child->GetId());
  }
  msgs::Set(jointData->jointMsg->mutable_pose(), math::Pose::Zero);

  jointData->jointMsg->set_type(
      msgs::ConvertJointType(this->GetTypeAsString(jointData->type)));

  unsigned int axisCount = JointMaker::GetJointAxisCount(jointData->type);
  for (unsigned int i = 0; i < axisCount; ++i)
  {
    msgs::Axis *axisMsg;
    if (i == 0u)
    {
      axisMsg = jointData->jointMsg->mutable_axis1();
    }
    else if (i == 1u)
    {
      axisMsg = jointData->jointMsg->mutable_axis2();
    }
    else
    {
      gzerr << "Invalid axis index["
            << i
            << "]"
            << std::endl;
      continue;
    }
    msgs::Set(axisMsg->mutable_xyz(),
        this->UnitVectors[i%this->UnitVectors.size()]);
    axisMsg->set_use_parent_model_frame(false);
    axisMsg->set_limit_lower(-GZ_DBL_MAX);
    axisMsg->set_limit_upper(GZ_DBL_MAX);
    axisMsg->set_limit_effort(-1);
    axisMsg->set_limit_velocity(-1);
    axisMsg->set_damping(0);

    // Add angle field after we've checked that index i is valid
    jointData->jointMsg->add_angle(0);
  }
  jointData->jointMsg->set_limit_erp(0.2);
  jointData->jointMsg->set_suspension_erp(0.2);

  jointData->inspector->Update(jointData->jointMsg);
  return jointData;
}

/////////////////////////////////////////////////
JointMaker::JointType JointMaker::ConvertJointType(const std::string &_type)
{
  for (auto iter : jointTypes)
    if (iter.second == _type)
      return iter.first;

  return JOINT_NONE;
}

/////////////////////////////////////////////////
void JointMaker::AddJoint(const std::string &_type)
{
  this->AddJoint(this->ConvertJointType(_type));
}

/////////////////////////////////////////////////
void JointMaker::AddJoint(JointMaker::JointType _type)
{
  this->jointType = _type;
  if (_type != JointMaker::JOINT_NONE)
  {
    // Add an event filter, which allows the JointMaker to capture mouse events.
    MouseEventHandler::Instance()->AddMoveFilter("model_joint",
        boost::bind(&JointMaker::OnMouseMove, this, _1));
  }
  else
  {
    // Remove the event filters.
    MouseEventHandler::Instance()->RemoveMoveFilter("model_joint");

    // signal the end of a joint action.
    emit JointAdded();
  }
}

/////////////////////////////////////////////////
void JointMaker::Stop()
{
  if (this->jointType != JointMaker::JOINT_NONE)
  {
    this->newJointCreated = false;
    if (this->mouseJoint)
    {
      this->mouseJoint->visual->DeleteDynamicLine(this->mouseJoint->line);
      rendering::ScenePtr scene = this->mouseJoint->visual->GetScene();
      scene->RemoveVisual(this->mouseJoint->visual);
      this->mouseJoint->visual.reset();
      delete this->mouseJoint;
      this->mouseJoint = NULL;
    }
    this->AddJoint(JointMaker::JOINT_NONE);
    if (this->hoverVis)
      this->hoverVis->SetEmissive(common::Color(0, 0, 0));
    if (this->selectedVis)
      this->selectedVis->SetEmissive(common::Color(0, 0, 0));
    this->selectedVis.reset();
    this->hoverVis.reset();
  }
}

/////////////////////////////////////////////////
bool JointMaker::OnMouseMove(const common::MouseEvent &_event)
{
  // Get the active camera and scene.
  rendering::UserCameraPtr camera = gui::get_active_camera();

  if (_event.dragging)
  {
    // this enables the joint maker to pan while connecting joints
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    camera->HandleMouseEvent(_event);
    return true;
  }

  rendering::VisualPtr vis = camera->GetVisual(_event.pos);

  // Highlight visual on hover
  if (vis)
  {
    if (this->hoverVis && this->hoverVis != this->selectedVis)
      this->hoverVis->SetEmissive(common::Color(0.0, 0.0, 0.0));

    // only highlight editor links by making sure it's not an item in the
    // gui tree widget or a joint hotspot.
    rendering::VisualPtr rootVis = vis->GetRootVisual();
    if (rootVis->IsPlane())
      this->hoverVis = vis->GetParent();
    else if (!gui::get_entity_id(rootVis->GetName()) &&
        vis->GetName().find("_UNIQUE_ID_") == std::string::npos)
    {
      this->hoverVis = vis->GetParent();
      if (!this->selectedVis ||
           (this->selectedVis && this->hoverVis != this->selectedVis))
        this->hoverVis->SetEmissive(common::Color(0.5, 0.5, 0.5));
    }
  }

  // Case when a parent link is already selected and currently
  // extending the joint line to a child link
  if (this->selectedVis && this->hoverVis
      && this->mouseJoint && this->mouseJoint->line)
  {
    math::Vector3 parentPos;
    // Set end point to center of child link
    if (!this->hoverVis->IsPlane())
    {
      if (this->mouseJoint->parent)
      {
        parentPos =  this->GetLinkWorldCentroid(this->mouseJoint->parent)
            - this->mouseJoint->line->GetPoint(0);
        this->mouseJoint->line->SetPoint(1,
            this->GetLinkWorldCentroid(this->hoverVis) - parentPos);
      }
    }
    else
    {
      // Set end point to mouse plane intersection
      math::Vector3 pt;
      camera->GetWorldPointOnPlane(_event.pos.x, _event.pos.y,
          math::Plane(math::Vector3(0, 0, 1)), pt);
      if (this->mouseJoint->parent)
      {
        parentPos = this->GetLinkWorldCentroid(this->mouseJoint->parent)
            - this->mouseJoint->line->GetPoint(0);
        this->mouseJoint->line->SetPoint(1,
            this->GetLinkWorldCentroid(this->hoverVis) - parentPos + pt);
      }
    }
  }
  return true;
}

/////////////////////////////////////////////////
void JointMaker::OnOpenInspector()
{
  this->OpenInspector(this->inspectVis->GetName());
  this->inspectVis.reset();
}

/////////////////////////////////////////////////
void JointMaker::OpenInspector(const std::string &_jointId)
{
  JointData *joint = this->joints[_jointId];
  if (!joint)
  {
    gzerr << "Joint [" << _jointId << "] not found." << std::endl;
    return;
  }
  joint->OpenInspector();
}

/////////////////////////////////////////////////
bool JointMaker::OnMouseDoubleClick(const common::MouseEvent &_event)
{
  rendering::UserCameraPtr camera = gui::get_active_camera();
  rendering::VisualPtr vis = camera->GetVisual(_event.pos);

  if (vis)
  {
    if (this->joints.find(vis->GetName()) != this->joints.end())
    {
      this->OpenInspector(vis->GetName());
      return true;
    }
  }

  return false;
}

/////////////////////////////////////////////////
bool JointMaker::OnKeyPress(const common::KeyEvent &_event)
{
  if (_event.key == Qt::Key_Delete)
  {
    if (this->selectedJoint)
    {
      this->RemoveJoint(this->selectedJoint->GetName());
      this->selectedJoint.reset();
    }
  }
  return false;
}

/////////////////////////////////////////////////
void JointMaker::CreateHotSpot(JointData *_joint)
{
  if (!_joint)
    return;

  rendering::UserCameraPtr camera = gui::get_active_camera();

  // Joint hotspot visual name is the JointId for easy access when clicking
  std::string jointId = _joint->visual->GetName() + "_UNIQUE_ID_";
  rendering::VisualPtr hotspotVisual(
      new rendering::Visual(jointId, _joint->visual, false));

  // create a cylinder to represent the joint
  hotspotVisual->InsertMesh("unit_cylinder");
  Ogre::MovableObject *hotspotObj =
      (Ogre::MovableObject*)(camera->GetScene()->GetManager()->createEntity(
      _joint->visual->GetName(), "unit_cylinder"));
  hotspotObj->getUserObjectBindings().setUserAny(Ogre::Any(jointId));
  hotspotVisual->GetSceneNode()->attachObject(hotspotObj);
  hotspotVisual->SetMaterial(this->jointMaterials[_joint->type]);
  hotspotVisual->SetTransparency(0.7);

  // create a handle at the parent end
  Ogre::BillboardSet *handleSet =
      camera->GetScene()->GetManager()->createBillboardSet(1);
  handleSet->setAutoUpdate(true);
  handleSet->setMaterialName("Gazebo/PointHandle");
  Ogre::MaterialPtr mat =
      Ogre::MaterialManager::getSingleton().getByName(
      this->jointMaterials[_joint->type]);
  Ogre::ColourValue color = mat->getTechnique(0)->getPass(0)->getDiffuse();
  color.a = 0.5;
  double dimension = 0.1;
  handleSet->setDefaultDimensions(dimension, dimension);
  Ogre::Billboard *parentHandle = handleSet->createBillboard(0, 0, 0);
  parentHandle->setColour(color);
  Ogre::SceneNode *handleNode =
      hotspotVisual->GetSceneNode()->createChildSceneNode();
  handleNode->attachObject(handleSet);
  handleNode->setInheritScale(false);
  handleNode->setInheritOrientation(false);
  _joint->handles = handleSet;

  hotspotVisual->SetVisibilityFlags(GZ_VISIBILITY_GUI |
      GZ_VISIBILITY_SELECTABLE);
  hotspotVisual->GetSceneNode()->setInheritScale(false);

  this->joints[jointId] = _joint;
  camera->GetScene()->AddVisual(hotspotVisual);

  _joint->hotspot = hotspotVisual;
  gui::model::Events::jointInserted(jointId, _joint->name);
}

/////////////////////////////////////////////////
void JointMaker::Update()
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
  if (this->newJointCreated)
  {
    this->CreateHotSpot(this->mouseJoint);
    this->mouseJoint = NULL;
    this->newJointCreated = false;
  }

  // update joint line and hotspot position.
  for (auto it : this->joints)
  {
    JointData *joint = it.second;
    if (joint->hotspot)
    {
      if (joint->child && joint->parent)
      {
        bool poseUpdate = false;
        if (joint->parentPose != joint->parent->GetWorldPose() ||
            joint->childPose != joint->child->GetWorldPose() ||
            joint->childScale != joint->child->GetScale())
         {
           joint->parentPose = joint->parent->GetWorldPose();
           joint->childPose = joint->child->GetWorldPose();
           joint->childScale = joint->child->GetScale();
           poseUpdate = true;
         }

        if (joint->dirty || poseUpdate)
        {
          // get origin of parent link visuals
          math::Vector3 parentOrigin = joint->parent->GetWorldPose().pos;

          // get origin of child link visuals
          math::Vector3 childOrigin = joint->child->GetWorldPose().pos;

          // set orientation of joint hotspot
          math::Vector3 dPos = (childOrigin - parentOrigin);
          math::Vector3 center = dPos * 0.5;
          joint->hotspot->SetScale(
              math::Vector3(0.008, 0.008, dPos.GetLength()));
          joint->hotspot->SetWorldPosition(parentOrigin + center);
          math::Vector3 u = dPos.Normalize();
          math::Vector3 v = math::Vector3::UnitZ;
          double cosTheta = v.Dot(u);
          double angle = acos(cosTheta);
          math::Vector3 w = (v.Cross(u)).Normalize();
          math::Quaternion q;
          q.SetFromAxis(w, angle);
          joint->hotspot->SetWorldRotation(q);

          // set new material if joint type has changed
          std::string material = this->jointMaterials[joint->type];
          if (joint->hotspot->GetMaterialName() != material)
          {
            // Note: issue setting material when there is a billboard child,
            // seems to hang so detach before setting and re-attach later.
            Ogre::SceneNode *handleNode = joint->handles->getParentSceneNode();
            joint->handles->detachFromParent();
            joint->hotspot->SetMaterial(material);
            joint->hotspot->SetTransparency(0.7);
            handleNode->attachObject(joint->handles);
            Ogre::MaterialPtr mat =
                Ogre::MaterialManager::getSingleton().getByName(material);
            Ogre::ColourValue color =
                mat->getTechnique(0)->getPass(0)->getDiffuse();
            color.a = 0.5;
            joint->handles->getBillboard(0)->setColour(color);
          }

          // set pos of joint handle
          joint->handles->getBillboard(0)->setPosition(
              rendering::Conversions::Convert(parentOrigin -
              joint->hotspot->GetWorldPose().pos));
          joint->handles->_updateBounds();
        }

        // Create / update joint visual
        if (joint->dirty || poseUpdate)
        {
          msgs::JointPtr jointUpdateMsg = joint->jointMsg;
          unsigned int axisCount = JointMaker::GetJointAxisCount(joint->type);
          for (unsigned int i = axisCount; i < 2u; ++i)
          {
            if (i == 0u)
              jointUpdateMsg->clear_axis1();
            else if (i == 1u)
              jointUpdateMsg->clear_axis2();
          }

          if (joint->jointVisual)
          {
            joint->jointVisual->UpdateFromMsg(jointUpdateMsg);
          }
          else
          {
            std::string childName = joint->child->GetName();
            std::string jointVisName = childName;
            size_t idx = childName.find("::");
            if (idx != std::string::npos)
              jointVisName = childName.substr(0, idx+2);
            jointVisName += "_JOINT_VISUAL_";
            gazebo::rendering::JointVisualPtr jointVis(
                new gazebo::rendering::JointVisual(jointVisName, joint->child));

            jointVis->Load(jointUpdateMsg);
            joint->jointVisual = jointVis;
          }

          // Line now connects the child link to the joint frame
          joint->line->SetPoint(0, joint->child->GetWorldPose().pos
              - joint->child->GetParent()->GetWorldPose().pos);
          joint->line->SetPoint(1,
              joint->jointVisual->GetWorldPose().pos
              - joint->child->GetParent()->GetWorldPose().pos);
          joint->line->setMaterial(this->jointMaterials[joint->type]);
          joint->dirty = false;
        }
      }
    }
  }
}

/////////////////////////////////////////////////
void JointMaker::AddScopedLinkName(const std::string &_name)
{
  this->scopedLinkedNames.push_back(_name);
}

/////////////////////////////////////////////////
std::string JointMaker::GetScopedLinkName(const std::string &_name)
{
  for (unsigned int i = 0; i < this->scopedLinkedNames.size(); ++i)
  {
    std::string scopedName = this->scopedLinkedNames[i];
    size_t idx = scopedName.find("::" + _name);
    if (idx != std::string::npos)
      return scopedName;
  }
  return _name;
}

/////////////////////////////////////////////////
void JointMaker::GenerateSDF()
{
  this->modelSDF.reset(new sdf::Element);
  sdf::initFile("model.sdf", this->modelSDF);
  this->modelSDF->ClearElements();

  // loop through all joints
  for (auto jointsIt : this->joints)
  {
    JointData *joint = jointsIt.second;
    sdf::ElementPtr jointElem = this->modelSDF->AddElement("joint");

    msgs::JointPtr jointMsg = joint->jointMsg;
    unsigned int axisCount = GetJointAxisCount(joint->type);
    for (unsigned int i = axisCount; i < 2u; ++i)
    {
      if (i == 0u)
        jointMsg->clear_axis1();
      else if (i == 1u)
        jointMsg->clear_axis2();
    }
    jointElem = msgs::JointToSDF(*jointMsg.get(), jointElem);

    sdf::ElementPtr parentElem = jointElem->GetElement("parent");
    std::string parentName = joint->parent->GetName();
    std::string parentLeafName = parentName;
    size_t pIdx = parentName.find_last_of("::");
    if (pIdx != std::string::npos)
      parentLeafName = parentName.substr(pIdx+1);

    parentLeafName = this->GetScopedLinkName(parentLeafName);
    parentElem->Set(parentLeafName);

    sdf::ElementPtr childElem = jointElem->GetElement("child");
    std::string childName = joint->child->GetName();
    std::string childLeafName = childName;
    size_t cIdx = childName.find_last_of("::");
    if (cIdx != std::string::npos)
      childLeafName = childName.substr(cIdx+1);
    childLeafName = this->GetScopedLinkName(childLeafName);
    childElem->Set(childLeafName);
  }
}

/////////////////////////////////////////////////
sdf::ElementPtr JointMaker::GetSDF() const
{
  return this->modelSDF;
}

/////////////////////////////////////////////////
std::string JointMaker::GetTypeAsString(JointMaker::JointType _type)
{
  std::string jointTypeStr = "";

  auto iter = jointTypes.find(_type);
  if (iter != jointTypes.end())
    jointTypeStr = iter->second;

  return jointTypeStr;
}

/////////////////////////////////////////////////
unsigned int JointMaker::GetJointAxisCount(JointMaker::JointType _type)
{
  if (_type == JOINT_FIXED)
  {
    return 0;
  }
  else if (_type == JOINT_HINGE)
  {
    return 1;
  }
  else if (_type == JOINT_HINGE2)
  {
    return 2;
  }
  else if (_type == JOINT_SLIDER)
  {
    return 1;
  }
  else if (_type == JOINT_SCREW)
  {
    return 1;
  }
  else if (_type == JOINT_UNIVERSAL)
  {
    return 2;
  }
  else if (_type == JOINT_BALL)
  {
    return 0;
  }

  return 0;
}

/////////////////////////////////////////////////
JointMaker::JointType JointMaker::GetState() const
{
  return this->jointType;
}

/////////////////////////////////////////////////
math::Vector3 JointMaker::GetLinkWorldCentroid(
    const rendering::VisualPtr _visual)
{
  math::Vector3 centroid;
  int count = 0;
  for (unsigned int i = 0; i < _visual->GetChildCount(); ++i)
  {
    if (_visual->GetChild(i)->GetName().find("_JOINT_VISUAL_") ==
        std::string::npos)
    {
      centroid += _visual->GetChild(i)->GetWorldPose().pos;
      count++;
    }
  }
  centroid /= count;
  return centroid;
}

/////////////////////////////////////////////////
unsigned int JointMaker::GetJointCount()
{
  return this->joints.size();
}

/////////////////////////////////////////////////
void JointData::OnApply()
{
  this->jointMsg->CopyFrom(*this->inspector->GetData());
  this->type = JointMaker::ConvertJointType(
      msgs::ConvertJointType(this->jointMsg->type()));

  if (this->name != this->jointMsg->name())
    gui::model::Events::jointNameChanged(this->hotspot->GetName(),
        this->jointMsg->name());
  this->name = this->jointMsg->name();

  this->dirty = true;
  gui::model::Events::modelChanged();
}

/////////////////////////////////////////////////
void JointData::OnOpenInspector()
{
  this->OpenInspector();
}

/////////////////////////////////////////////////
void JointData::OpenInspector()
{
  this->inspector->Update(this->jointMsg);
  this->inspector->move(QCursor::pos());
  this->inspector->show();
}

/////////////////////////////////////////////////
void JointMaker::CreateJointFromSDF(sdf::ElementPtr _jointElem,
    const std::string &_modelName)
{
  // Name
  std::string jointName = _jointElem->Get<std::string>("name");

  // Pose
  math::Pose jointPose;
  if (_jointElem->HasElement("pose"))
    jointPose = _jointElem->Get<math::Pose>("pose");
  else
    jointPose.Set(0, 0, 0, 0, 0, 0);

  // Parent
  std::string parentName = _modelName + "::" +
      _jointElem->GetElement("parent")->GetValue()->GetAsString();

  rendering::VisualPtr parentVis =
      gui::get_active_camera()->GetScene()->GetVisual(parentName);

  // Child
  std::string childName = _modelName + "::" +
      _jointElem->GetElement("child")->GetValue()->GetAsString();
  rendering::VisualPtr childVis =
      gui::get_active_camera()->GetScene()->GetVisual(childName);

  if (!parentVis || !childVis)
  {
    gzerr << "Unable to load joint. Joint child / parent not found"
        << std::endl;
    return;
  }

  // Type
  std::string type = _jointElem->Get<std::string>("type");

  JointData *joint = new JointData();
  joint->name = jointName;
  joint->parent = parentVis;
  joint->child = childVis;
  joint->type = this->ConvertJointType(type);
  std::string jointVisName = _modelName + "::" + jointName;

  joint->jointMsg.reset(new msgs::Joint);
  joint->jointMsg->set_name(jointName);

  std::string jointParentName = joint->parent->GetName();
  std::string jointParentLeafName = jointParentName;
  size_t pIdx = jointParentName.find_last_of("::");
  if (pIdx != std::string::npos)
    jointParentLeafName = jointParentName.substr(pIdx+1);
  joint->jointMsg->set_parent(jointParentLeafName);

  std::string jointChildName = joint->child->GetName();
  std::string jointChildLeafName = jointChildName;
  size_t cIdx = jointChildName.find_last_of("::");
  if (cIdx != std::string::npos)
    jointChildLeafName = jointChildName.substr(cIdx+1);
  joint->jointMsg->set_child(jointChildLeafName);

  joint->jointMsg->set_parent_id(joint->parent->GetId());
  joint->jointMsg->set_child_id(joint->child->GetId());

  msgs::Set(joint->jointMsg->mutable_pose(), jointPose);

  joint->jointMsg->set_type(
      msgs::ConvertJointType(this->GetTypeAsString(joint->type)));

  unsigned int axisCount = JointMaker::GetJointAxisCount(joint->type);
  for (unsigned int i = 0; i < axisCount; ++i)
  {
    msgs::Axis *axisMsg;
    sdf::ElementPtr axisElem;
    if (i == 0u)
    {
      axisMsg = joint->jointMsg->mutable_axis1();
      axisElem = _jointElem->GetElement("axis");
    }
    else if (i == 1u)
    {
      axisMsg = joint->jointMsg->mutable_axis2();
      axisElem = _jointElem->GetElement("axis2");
    }
    else
    {
      gzerr << "Invalid axis index["
            << i
            << "]"
            << std::endl;
      continue;
    }
    if (axisElem->HasElement("limit"))
    {
      sdf::ElementPtr limitElem = axisElem->GetElement("limit");
      axisMsg->set_limit_lower(limitElem->Get<double>("lower"));
      axisMsg->set_limit_upper(limitElem->Get<double>("upper"));
      axisMsg->set_limit_effort(limitElem->Get<double>("effort"));
      axisMsg->set_limit_velocity(limitElem->Get<double>("velocity"));
    }
    // Use parent model frame
    if (axisElem->HasElement("use_parent_model_frame"))
    {
      bool useParent = axisElem->Get<bool>("use_parent_model_frame");
      axisMsg->set_use_parent_model_frame(useParent);
    }
    if (axisElem->HasElement("dynamics"))
    {
      sdf::ElementPtr dynamicsElem = axisElem->GetElement("dynamics");
      axisMsg->set_damping(dynamicsElem->Get<double>("damping"));
      axisMsg->set_friction(dynamicsElem->Get<double>("friction"));
    }

    msgs::Set(axisMsg->mutable_xyz(), axisElem->Get<math::Vector3>("xyz"));

    // Add angle field after we've checked that index i is valid
    joint->jointMsg->add_angle(0);
  }

  if (_jointElem->HasElement("physics"))
  {
    sdf::ElementPtr physicsElem = _jointElem->GetElement("physics");
    if (physicsElem->HasElement("ode"))
    {
      sdf::ElementPtr odeElem = physicsElem->GetElement("ode");
      if (odeElem->HasElement("cfm"))
        joint->jointMsg->set_cfm(odeElem->Get<double>("cfm"));
      if (odeElem->HasElement("bounce"))
        joint->jointMsg->set_bounce(odeElem->Get<double>("bounce"));
      if (odeElem->HasElement("velocity"))
        joint->jointMsg->set_velocity(odeElem->Get<double>("velocity"));
      if (odeElem->HasElement("fudge_factor"))
        joint->jointMsg->set_fudge_factor(odeElem->Get<double>("fudge_factor"));

      if (odeElem->HasElement("limit"))
      {
        sdf::ElementPtr odeLimitElem = odeElem->GetElement("limit");
        if (odeLimitElem->HasElement("cfm"))
          joint->jointMsg->set_limit_cfm(odeLimitElem->Get<double>("cfm"));
        if (odeLimitElem->HasElement("erp"))
          joint->jointMsg->set_limit_erp(odeLimitElem->Get<double>("erp"));
      }
      if (odeElem->HasElement("suspension"))
      {
        sdf::ElementPtr odeSuspensionElem = odeElem->GetElement("suspension");
        if (odeSuspensionElem->HasElement("cfm"))
        {
          joint->jointMsg->set_suspension_cfm(
              odeSuspensionElem->Get<double>("cfm"));
        }
        if (odeSuspensionElem->HasElement("erp"))
        {
          joint->jointMsg->set_suspension_erp(
              odeSuspensionElem->Get<double>("erp"));
        }
      }
    }
  }

  // Inspector
  joint->inspector = new JointInspector();
  joint->inspector->Update(joint->jointMsg);
  joint->inspector->setModal(false);
  connect(joint->inspector, SIGNAL(Applied()),
      joint, SLOT(OnApply()));

  // Visuals
  rendering::VisualPtr jointVis(
      new rendering::Visual(jointVisName, parentVis->GetParent()));
  jointVis->Load();
  rendering::DynamicLines *jointLine =
      jointVis->CreateDynamicLine(rendering::RENDERING_LINE_LIST);

  math::Vector3 origin = parentVis->GetWorldPose().pos
      - parentVis->GetParent()->GetWorldPose().pos;
  jointLine->AddPoint(origin);
  jointLine->AddPoint(origin + math::Vector3(0, 0, 0.1));

  jointVis->GetSceneNode()->setInheritScale(false);
  jointVis->GetSceneNode()->setInheritOrientation(false);
  joint->visual = jointVis;
  joint->line = jointLine;
  joint->dirty = true;

  this->CreateHotSpot(joint);
}

/////////////////////////////////////////////////
void JointMaker::ShowJoints(bool _show)
{
  for (auto iter : this->joints)
  {
    rendering::VisualPtr vis = iter.second->hotspot;
    if (vis)
    {
      vis->SetVisible(_show);
      vis->SetHighlighted(false);
    }
    if (iter.second->jointVisual)
      iter.second->jointVisual->SetVisible(_show);
  }
  if (this->selectedJoint)
    this->selectedJoint->SetHighlighted(_show);
}
