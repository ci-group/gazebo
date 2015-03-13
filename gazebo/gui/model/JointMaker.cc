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

  this->connections.push_back(
      event::Events::ConnectPreRender(
        boost::bind(&JointMaker::Update, this)));

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

  while (this->joints.size() > 0)
  {
    this->RemoveJoint(this->joints.begin()->first);
  }
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
void JointMaker::RemoveJoint(const std::string &_jointName)
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
  auto jointIt = this->joints.find(_jointName);
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
  }
}

/////////////////////////////////////////////////
void JointMaker::RemoveJointsByLink(const std::string &_linkName)
{
  std::vector<std::string> toDelete;
  boost::unordered_map<std::string, JointData *>::iterator it;
  for (it = this->joints.begin(); it != this->joints.end(); ++it)
  {
    JointData *joint = it->second;

    if (joint->child->GetName() == _linkName ||
        joint->parent->GetName() == _linkName)
    {
      toDelete.push_back(it->first);
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
          this->mouseJoint = this->CreateJoint(this->selectedVis,
              rendering::VisualPtr());
        }
        // Pressed child link
        else if (this->selectedVis != this->hoverVis)
        {
          if (this->hoverVis)
            this->hoverVis->SetEmissive(common::Color(0, 0, 0));
          if (this->selectedVis)
            this->selectedVis->SetEmissive(common::Color(0, 0, 0));
          this->mouseJoint->child = this->hoverVis;

          // reset variables.
          this->selectedVis.reset();
          this->hoverVis.reset();
          this->AddJoint(JointMaker::JOINT_NONE);

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
JointData *JointMaker::CreateJoint(rendering::VisualPtr _parent,
    rendering::VisualPtr _child)
{
  std::stringstream ss;
  ss << _parent->GetName() << "_JOINT_" << this->jointCounter++;
  rendering::VisualPtr jointVis(
      new rendering::Visual(ss.str(), _parent->GetParent()));
  jointVis->Load();
  rendering::DynamicLines *jointLine =
      jointVis->CreateDynamicLine(rendering::RENDERING_LINE_LIST);
  math::Vector3 origin = _parent->GetWorldPose().pos
      - _parent->GetParent()->GetWorldPose().pos;
  jointLine->AddPoint(origin);
  jointLine->AddPoint(origin + math::Vector3(0, 0, 0.1));
  jointVis->GetSceneNode()->setInheritScale(false);
  jointVis->GetSceneNode()->setInheritOrientation(false);

  JointData *jointData = new JointData();
  std::string jointVisName = jointVis->GetName();
  std::string leafName = jointVisName;
  size_t pIdx = jointVisName.find_last_of("::");
  if (pIdx != std::string::npos)
    leafName = jointVisName.substr(pIdx+1);
  jointData->name = leafName;
  jointData->dirty = false;
  jointData->visual = jointVis;
  jointData->parent = _parent;
  jointData->child = _child;
  jointData->line = jointLine;
  jointData->type = this->jointType;
  jointData->inspector = new JointInspector(JointMaker::JOINT_NONE);
  jointData->inspector->setModal(false);
  connect(jointData->inspector, SIGNAL(Applied()),
      jointData, SLOT(OnApply()));

  MainWindow *mainWindow = gui::get_main_window();
  if (mainWindow)
  {
    connect(gui::get_main_window(), SIGNAL(Close()), jointData->inspector,
        SLOT(close()));
  }

  int axisCount = JointMaker::GetJointAxisCount(jointData->type);
  for (int i = 0; i < axisCount; ++i)
  {
    if (i < static_cast<int>(this->UnitVectors.size()))
      jointData->axis[i] = this->UnitVectors[i];
    else
      jointData->axis[i] = this->UnitVectors[0];

    jointData->lowerLimit[i] = -3.14;
    jointData->upperLimit[i] = 3.14;
    jointData->effortLimit[i] = -1;
    jointData->velocityLimit[i] = -1;
    jointData->useParentModelFrame[i] = false;
    jointData->damping[i] = 0;
  }
  jointData->pose = math::Pose::Zero;
  jointData->line->setMaterial(this->jointMaterials[jointData->type]);

  return jointData;
}

/////////////////////////////////////////////////
JointMaker::JointType JointMaker::ConvertJointType(const std::string &_type)
{
  if (_type == "revolute")
  {
    return JointMaker::JOINT_HINGE;
  }
  else if (_type == "revolute2")
  {
    return JointMaker::JOINT_HINGE2;
  }
  else if (_type == "prismatic")
  {
    return JointMaker::JOINT_SLIDER;
  }
  else if (_type == "ball")
  {
    return JointMaker::JOINT_BALL;
  }
  else if (_type == "universal")
  {
    return JointMaker::JOINT_UNIVERSAL;
  }
  else if (_type == "screw")
  {
    return JointMaker::JOINT_SCREW;
  }
  else
  {
    return JointMaker::JOINT_NONE;
  }
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
      delete this->mouseJoint->inspector;
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
        vis->GetName().find("_HOTSPOT_") == std::string::npos)
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
void JointMaker::OpenInspector(const std::string &_name)
{
  JointData *joint = this->joints[_name];
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

  std::string hotSpotName = _joint->visual->GetName() + "_HOTSPOT_";
  rendering::VisualPtr hotspotVisual(
      new rendering::Visual(hotSpotName, _joint->visual, false));

  // create a cylinder to represent the joint
  hotspotVisual->InsertMesh("unit_cylinder");
  Ogre::MovableObject *hotspotObj =
      (Ogre::MovableObject*)(camera->GetScene()->GetManager()->createEntity(
      _joint->visual->GetName(), "unit_cylinder"));
  hotspotObj->getUserObjectBindings().setUserAny(Ogre::Any(hotSpotName));
  hotspotVisual->GetSceneNode()->attachObject(hotspotObj);
  hotspotVisual->SetMaterial(this->jointMaterials[_joint->type]);
  hotspotVisual->SetTransparency(0.5);

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

  this->joints[hotSpotName] = _joint;
  camera->GetScene()->AddVisual(hotspotVisual);

  _joint->hotspot = hotspotVisual;
  gui::model::Events::jointInserted(_joint->name);
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
  boost::unordered_map<std::string, JointData *>::iterator it;
  for (it = this->joints.begin(); it != this->joints.end(); ++it)
  {
    JointData *joint = it->second;
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
          math::Vector3 center = dPos/2.0;
          joint->hotspot->SetScale(math::Vector3(0.02, 0.02, dPos.GetLength()));
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
            joint->hotspot->SetTransparency(0.5);
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
        if (!joint->jointMsg || joint->dirty || poseUpdate)
        {
          joint->jointMsg.reset(new gazebo::msgs::Joint);
          joint->jointMsg->set_parent(joint->parent->GetName());
          joint->jointMsg->set_parent_id(joint->parent->GetId());
          joint->jointMsg->set_child(joint->child->GetName());
          joint->jointMsg->set_child_id(joint->child->GetId());
          joint->jointMsg->set_name(joint->name);

          msgs::Set(joint->jointMsg->mutable_pose(), joint->pose);
          if (joint->type == JointMaker::JOINT_SLIDER)
          {
            joint->jointMsg->set_type(msgs::Joint::PRISMATIC);
          }
          else if (joint->type == JointMaker::JOINT_HINGE)
          {
            joint->jointMsg->set_type(msgs::Joint::REVOLUTE);
          }
          else if (joint->type == JointMaker::JOINT_HINGE2)
          {
            joint->jointMsg->set_type(msgs::Joint::REVOLUTE2);
          }
          else if (joint->type == JointMaker::JOINT_SCREW)
          {
            joint->jointMsg->set_type(msgs::Joint::SCREW);
          }
          else if (joint->type == JointMaker::JOINT_UNIVERSAL)
          {
            joint->jointMsg->set_type(msgs::Joint::UNIVERSAL);
          }
          else if (joint->type == JointMaker::JOINT_BALL)
          {
            joint->jointMsg->set_type(msgs::Joint::BALL);
          }

          int axisCount = JointMaker::GetJointAxisCount(joint->type);
          for (int i = 0; i < axisCount; ++i)
          {
            msgs::Axis *axisMsg;
            if (i == 0)
            {
              axisMsg = joint->jointMsg->mutable_axis1();
            }
            else if (i == 1)
            {
              axisMsg = joint->jointMsg->mutable_axis2();
            }
            else
            {
              gzerr << "Invalid axis index["
                    << i
                    << "]"
                    << std::endl;
              continue;
            }
            msgs::Set(axisMsg->mutable_xyz(), joint->axis[i]);
            axisMsg->set_use_parent_model_frame(joint->useParentModelFrame[i]);

            // Add angle field after we've checked that index i is valid
            joint->jointMsg->add_angle(0);
          }

          if (joint->jointVisual)
          {
            joint->jointVisual->UpdateFromMsg(joint->jointMsg);
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

            jointVis->Load(joint->jointMsg);
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

  boost::unordered_map<std::string, JointData *>::iterator jointsIt;
  // loop through all joints
  for (jointsIt = this->joints.begin(); jointsIt != this->joints.end();
      ++jointsIt)
  {
    JointData *joint = jointsIt->second;
    sdf::ElementPtr jointElem = this->modelSDF->AddElement("joint");

    jointElem->GetAttribute("name")->Set(joint->name);
    jointElem->GetAttribute("type")->Set(GetTypeAsString(joint->type));
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

    sdf::ElementPtr poseElem = jointElem->GetElement("pose");
    poseElem->Set(joint->pose);
    int axisCount = GetJointAxisCount(joint->type);
    for (int i = 0; i < axisCount; ++i)
    {
      std::stringstream ss;
      ss << "axis";
      if (i > 0)
        ss << (i+1);
      sdf::ElementPtr axisElem = jointElem->GetElement(ss.str());
      axisElem->GetElement("xyz")->Set(joint->axis[i]);

      sdf::ElementPtr limitElem = axisElem->GetElement("limit");
      limitElem->GetElement("lower")->Set(joint->lowerLimit[i]);
      limitElem->GetElement("upper")->Set(joint->upperLimit[i]);
      limitElem->GetElement("effort")->Set(joint->effortLimit[i]);
      limitElem->GetElement("velocity")->Set(joint->velocityLimit[i]);
      axisElem->GetElement("use_parent_model_frame")->Set(
          joint->useParentModelFrame[i]);
      axisElem->GetElement("dynamics")->GetElement("damping")->Set(
          joint->damping[i]);
    }
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

  if (_type == JointMaker::JOINT_FIXED)
  {
    jointTypeStr = "fixed";
  }
  else if (_type == JointMaker::JOINT_SLIDER)
  {
    jointTypeStr = "prismatic";
  }
  else if (_type == JointMaker::JOINT_HINGE)
  {
    jointTypeStr = "revolute";
  }
  else if (_type == JointMaker::JOINT_HINGE2)
  {
    jointTypeStr = "revolute2";
  }
  else if (_type == JointMaker::JOINT_SCREW)
  {
    jointTypeStr = "screw";
  }
  else if (_type == JointMaker::JOINT_UNIVERSAL)
  {
    jointTypeStr = "universal";
  }
  else if (_type == JointMaker::JOINT_BALL)
  {
    jointTypeStr = "ball";
  }
  else if (_type == JointMaker::JOINT_NONE)
  {
    jointTypeStr = "none";
  }

  return jointTypeStr;
}

/////////////////////////////////////////////////
int JointMaker::GetJointAxisCount(JointMaker::JointType _type)
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
  this->pose = this->inspector->GetPose();
  this->type = this->inspector->GetType();
  this->name = this->inspector->GetName();

  int axisCount = JointMaker::GetJointAxisCount(this->type);
  for (int i = 0; i < axisCount; ++i)
  {
    this->axis[i] = this->inspector->GetAxis(i);
    this->lowerLimit[i] = this->inspector->GetLowerLimit(i);
    this->upperLimit[i] = this->inspector->GetUpperLimit(i);
    this->useParentModelFrame[i] = this->inspector->GetUseParentModelFrame(i);
  }
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
  this->inspector->SetType(this->type);
  this->inspector->SetName(this->name);

  std::string parentName = this->parent->GetName();
  std::string parentLeafName = parentName;
  size_t pIdx = parentName.find_last_of("::");
  if (pIdx != std::string::npos)
    parentLeafName = parentName.substr(pIdx+1);
  this->inspector->SetParent(parentLeafName);

  std::string childName = this->child->GetName();
  std::string childLeafName = childName;
  size_t cIdx = childName.find_last_of("::");
  if (cIdx != std::string::npos)
    childLeafName = childName.substr(cIdx+1);
  this->inspector->SetChild(childLeafName);

  this->inspector->SetPose(this->pose);
  int axisCount = JointMaker::GetJointAxisCount(this->type);
  for (int i = 0; i < axisCount; ++i)
  {
    this->inspector->SetAxis(i, this->axis[i]);
    this->inspector->SetAxis(i, this->axis[i]);
    this->inspector->SetLowerLimit(i, this->lowerLimit[i]);
    this->inspector->SetUpperLimit(i, this->upperLimit[i]);
    this->inspector->SetUseParentModelFrame(i, this->useParentModelFrame[i]);
  }
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
  joint->pose = jointPose;
  joint->parent = parentVis;
  joint->child = childVis;
  joint->type = this->ConvertJointType(type);
  std::string jointVisName = _modelName + "::" + joint->name;

  // Axes
  int axisCount = JointMaker::GetJointAxisCount(joint->type);
  for (int i = 0; i < axisCount; ++i)
  {
    sdf::ElementPtr axisElem;
    if (i == 0)
      axisElem = _jointElem->GetElement("axis");
    else
      axisElem = _jointElem->GetElement("axis2");

    joint->axis[i] = axisElem->Get<math::Vector3>("xyz");

    if (axisElem->HasElement("limit"))
    {
      sdf::ElementPtr limitElem = axisElem->GetElement("limit");
      joint->lowerLimit[i] = limitElem->Get<double>("lower");
      joint->upperLimit[i] = limitElem->Get<double>("upper");
      joint->effortLimit[i] = limitElem->Get<double>("effort");
      joint->velocityLimit[i] = limitElem->Get<double>("velocity");
    }

    // Use parent model frame
    if (axisElem->HasElement("use_parent_model_frame"))
    {
      bool useParent = axisElem->Get<bool>("use_parent_model_frame");
      joint->useParentModelFrame[i] = useParent;
    }

    if (axisElem->HasElement("dynamics"))
    {
      sdf::ElementPtr dynamicsElem = axisElem->GetElement("dynamics");
      joint->damping[i] = dynamicsElem->Get<double>("damping");
    }
  }

  // Inspector
  joint->inspector = new JointInspector(joint->type);
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
