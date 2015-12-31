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

#include <boost/bind.hpp>
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
#include "gazebo/gui/ModelAlign.hh"

#include "gazebo/gui/model/JointCreationDialog.hh"
#include "gazebo/gui/model/JointInspector.hh"
#include "gazebo/gui/model/ModelEditorEvents.hh"
#include "gazebo/gui/model/JointMaker.hh"

using namespace gazebo;
using namespace gui;

std::map<JointMaker::JointType, std::string> JointMaker::jointTypes;
std::map<JointMaker::JointType, std::string> JointMaker::jointMaterials;
std::vector<ignition::math::Vector3d> JointMaker::unitVectors;

/////////////////////////////////////////////////
JointMaker::JointMaker()
{
  this->unitVectors.push_back(ignition::math::Vector3d::UnitX);
  this->unitVectors.push_back(ignition::math::Vector3d::UnitY);
  this->unitVectors.push_back(ignition::math::Vector3d::UnitZ);

  this->newJoint = NULL;
  this->modelSDF.reset();
  this->jointType = JointMaker::JOINT_NONE;
  this->jointCounter = 0;

  this->jointMaterials[JOINT_FIXED]     = "Gazebo/Red";
  this->jointMaterials[JOINT_HINGE]     = "Gazebo/Orange";
  this->jointMaterials[JOINT_HINGE2]    = "Gazebo/DarkYellow";
  this->jointMaterials[JOINT_SLIDER]    = "Gazebo/Green";
  this->jointMaterials[JOINT_SCREW]     = "Gazebo/DarkGrey";
  this->jointMaterials[JOINT_UNIVERSAL] = "Gazebo/Blue";
  this->jointMaterials[JOINT_BALL]      = "Gazebo/Purple";
  this->jointMaterials[JOINT_GEARBOX]   = "Gazebo/Indigo";

  this->jointTypes[JOINT_FIXED]     = "fixed";
  this->jointTypes[JOINT_HINGE]     = "revolute";
  this->jointTypes[JOINT_HINGE2]    = "revolute2";
  this->jointTypes[JOINT_SLIDER]    = "prismatic";
  this->jointTypes[JOINT_SCREW]     = "screw";
  this->jointTypes[JOINT_UNIVERSAL] = "universal";
  this->jointTypes[JOINT_BALL]      = "ball";
  this->jointTypes[JOINT_GEARBOX]   = "gearbox";
  this->jointTypes[JOINT_NONE]      = "none";

  this->jointCreationDialog = NULL;

  this->connections.push_back(
      event::Events::ConnectPreRender(
      boost::bind(&JointMaker::Update, this)));

  this->connections.push_back(
      gui::model::Events::ConnectOpenJointInspector(
      boost::bind(&JointMaker::OpenInspector, this, _1)));

  this->connections.push_back(
      gui::model::Events::ConnectShowJointContextMenu(
      boost::bind(&JointMaker::ShowContextMenu, this, _1)));

  this->connections.push_back(
      gui::model::Events::ConnectSetSelectedJoint(
      boost::bind(&JointMaker::OnSetSelectedJoint, this, _1, _2)));

  this->connections.push_back(
      event::Events::ConnectSetSelectedEntity(
      boost::bind(&JointMaker::OnSetSelectedEntity, this, _1, _2)));

  this->inspectAct = new QAction(tr("Open Joint Inspector"), this);
  connect(this->inspectAct, SIGNAL(triggered()), this, SLOT(OnOpenInspector()));

  this->updateMutex = new boost::recursive_mutex();

  // Gazebo event connections
  this->connections.push_back(
      gui::model::Events::ConnectLinkInserted(
      boost::bind(&JointMaker::OnLinkInserted, this, _1)));

  this->connections.push_back(
      gui::model::Events::ConnectLinkRemoved(
      boost::bind(&JointMaker::OnLinkRemoved, this, _1)));
}

/////////////////////////////////////////////////
JointMaker::~JointMaker()
{
  if (this->newJoint)
  {
    delete this->newJoint;
    this->newJoint = NULL;
  }

  {
    boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
    while (this->joints.size() > 0)
    {
      std::string jointName = this->joints.begin()->first;
      this->RemoveJoint(jointName);
    }
    this->joints.clear();
  }

  if (this->jointCreationDialog)
    delete this->jointCreationDialog;

  // Delete this last
  delete this->updateMutex;
}

/////////////////////////////////////////////////
void JointMaker::Reset()
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
  if (this->newJoint)
  {
    delete this->newJoint;
    this->newJoint = NULL;
  }

  this->jointType = JointMaker::JOINT_NONE;
  this->hoverVis.reset();
  this->inspectName = "";
  this->selectedJoints.clear();

  this->scopedLinkedNames.clear();

  while (!this->joints.empty())
  {
    std::string jointId = this->joints.begin()->first;
    this->RemoveJoint(jointId);
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

  MouseEventHandler::Instance()->AddMoveFilter("model_joint",
      boost::bind(&JointMaker::OnMouseMove, this, _1));

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

  std::string jointId = _jointId;
  JointData *joint = NULL;

  auto jointIt = this->joints.find(_jointId);

  // Existing joint
  if (jointIt != this->joints.end())
  {
    joint = jointIt->second;
  }
  // Joint being created
  else if (this->newJoint)
  {
    joint = this->newJoint;
    // Already has hotspot
    if (joint->hotspot)
      jointId = joint->hotspot->GetName();
    // Still only line
    else
      jointId = "";
  }

  if (!joint)
    return;

  if (jointId != "")
  {
    this->joints.erase(jointId);
    gui::model::Events::jointRemoved(jointId);
  }

  auto scene = rendering::get_scene();
  if (!scene)
    return;

  if (joint->handles)
  {
    scene->GetManager()->destroyBillboardSet(joint->handles);
    joint->handles = NULL;
  }

  if (joint->hotspot)
  {
    scene->RemoveVisual(joint->hotspot);
    joint->hotspot->Fini();
  }

  if (joint->visual)
  {
    scene->RemoveVisual(joint->visual);
    joint->visual->Fini();
  }

  if (joint->jointVisual)
  {
    auto parentAxisVis = joint->jointVisual->GetParentAxisVisual();
    if (parentAxisVis)
    {
      parentAxisVis->GetParent()->DetachVisual(parentAxisVis->GetName());
      scene->RemoveVisual(parentAxisVis);
    }
    joint->jointVisual->GetParent()->DetachVisual(
        joint->jointVisual->GetName());
    scene->RemoveVisual(joint->jointVisual);
  }

  if (joint->inspector)
  {
    joint->inspector->hide();
    delete joint->inspector;
    joint->inspector = NULL;
  }

  this->newJoint = NULL;
  joint->hotspot.reset();
  joint->visual.reset();
  joint->jointVisual.reset();
  joint->parent.reset();
  joint->child.reset();
  delete joint->inspector;
  delete joint;
  gui::model::Events::modelChanged();
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
  if (_event.Button() == common::MouseEvent::MIDDLE)
  {
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    camera->HandleMouseEvent(_event);
    return true;
  }
  else if (_event.Button() != common::MouseEvent::LEFT)
    return false;

  if (this->jointType != JointMaker::JOINT_NONE)
    return false;

  // intercept mouse press events when user clicks on the joint hotspot visual
  rendering::VisualPtr vis = camera->GetVisual(_event.Pos());
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

  // Not in the process of selecting joint links with mouse
  // Handle joint selection
  if (this->jointType == JointMaker::JOINT_NONE ||
      (this->newJoint && this->newJoint->parent && this->newJoint->child))
  {
    rendering::VisualPtr vis = camera->GetVisual(_event.Pos());
    if (vis)
    {
      if (this->joints.find(vis->GetName()) != this->joints.end())
      {
        // trigger joint inspector on right click
        if (_event.Button() == common::MouseEvent::RIGHT)
        {
          this->inspectName = vis->GetName();
          this->ShowContextMenu(this->inspectName);
          return true;
        }
        else if (_event.Button() == common::MouseEvent::LEFT)
        {
          // Not in multi-selection mode.
          if (!(QApplication::keyboardModifiers() & Qt::ControlModifier))
          {
            this->DeselectAll();
            this->SetSelected(vis, true);
          }
          // Multi-selection mode
          else
          {
            auto it = std::find(this->selectedJoints.begin(),
                this->selectedJoints.end(), vis);
            // Highlight and select clicked joint if not already selected
            // Otherwise deselect if already selected
            this->SetSelected(vis, it == this->selectedJoints.end());
          }
        }
      }
      else
        this->DeselectAll();
      return false;
    }
  }
  // Still selecting parent/child during new joint creation
  else
  {
    if (_event.Button() == common::MouseEvent::LEFT)
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
        if (!this->newJoint)
        {
          if (!this->SetParentLink(this->hoverVis))
            return false;

          if (this->jointCreationDialog)
          {
            this->jointCreationDialog->SetParent(
                this->newJoint->parent->GetName());
          }
        }
        // Pressed child link
        else if (this->newJoint && this->newJoint->parent != this->hoverVis)
        {
          if (!this->SetChildLink(this->hoverVis))
            return false;

          if (this->jointCreationDialog)
          {
            this->jointCreationDialog->SetChild(
                this->newJoint->child->GetName());
          }
        }

        if (this->hoverVis)
        {
          this->hoverVis->SetEmissive(common::Color(0, 0, 0));
          this->hoverVis.reset();
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
  jointLine->AddPoint(origin.Ign());
  jointLine->AddPoint(origin.Ign() + ignition::math::Vector3d(0, 0, 0.1));
  jointVis->GetSceneNode()->setInheritScale(false);
  jointVis->GetSceneNode()->setInheritOrientation(false);

  std::string jointVisName = jointVis->GetName();
  std::string leafName = jointVisName;
  size_t pIdx = jointVisName.rfind("::");
  if (pIdx != std::string::npos)
    leafName = jointVisName.substr(pIdx+2);

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

  // Inspector
  jointData->inspector = new JointInspector(this);
  jointData->inspector->setModal(false);
  connect(jointData->inspector, SIGNAL(Applied()), jointData, SLOT(OnApply()));

  MainWindow *mainWindow = gui::get_main_window();
  if (mainWindow)
  {
    connect(gui::get_main_window(), SIGNAL(Close()), jointData->inspector,
        SLOT(close()));
  }

  // setup the joint msg
  jointData->UpdateMsg();

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
std::string JointMaker::GetJointMaterial(const std::string &_type)
{
  auto it = jointMaterials.find(ConvertJointType(_type));
  if (it != jointMaterials.end())
    return it->second;
  else
    return "";
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
  // Start joint creation
  if (_type != JointMaker::JOINT_NONE)
  {
    if (!this->jointCreationDialog)
    {
      auto mainWindow = gui::get_main_window();
      this->jointCreationDialog = new JointCreationDialog(this, mainWindow);
    }
    this->jointCreationDialog->Open(_type);
  }
  // End joint creation
  else
  {
    // signal the end of a joint action.
    emit JointAdded();
  }
}

/////////////////////////////////////////////////
void JointMaker::Stop()
{
  // Reset links
  if (this->newJoint)
  {
    if (this->newJoint->parent)
    {
      this->newJoint->parent->SetWorldPose(this->parentLinkOriginalPose);
      this->SetVisualMoved(this->newJoint->parent, false);
    }
    if (this->newJoint->child)
    {
      this->newJoint->child->SetWorldPose(this->childLinkOriginalPose);
      this->SetVisualMoved(this->newJoint->child, false);
    }
  }

  this->RemoveJoint("");

  this->AddJoint(JointMaker::JOINT_NONE);
  if (this->hoverVis)
    this->hoverVis->SetEmissive(common::Color(0, 0, 0));
  this->hoverVis.reset();

  if (this->jointCreationDialog && this->jointCreationDialog->isVisible())
    this->jointCreationDialog->reject();
}

/////////////////////////////////////////////////
bool JointMaker::OnMouseMove(const common::MouseEvent &_event)
{
  // Only handle mouse move events during joint creation
  if (this->jointType == JointMaker::JOINT_NONE ||
      (this->newJoint && this->newJoint->child))
  {
    return false;
  }
  QApplication::setOverrideCursor(Qt::CrossCursor);

  // Get the active camera and scene.
  rendering::UserCameraPtr camera = gui::get_active_camera();

  if (_event.Dragging())
  {
    // this enables the joint maker to pan while connecting joints
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    camera->HandleMouseEvent(_event);
    return true;
  }

  rendering::VisualPtr vis = camera->GetVisual(_event.Pos());

  // Highlight visual on hover
  if (vis)
  {
    if (this->hoverVis)
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
      if (!this->newJoint || (this->newJoint->parent &&
           this->hoverVis != this->newJoint->parent))
      {
        this->hoverVis->SetEmissive(common::Color(0.5, 0.5, 0.5));
      }
    }
  }

  // Case when a parent link is already selected and currently
  // extending the joint line to a child link
  if (this->newJoint && this->newJoint->parent && this->hoverVis &&
      this->newJoint->line)
  {
    math::Vector3 parentPos;

    // Set end point to center of child link
    if (!this->hoverVis->IsPlane())
    {
      parentPos =  this->GetLinkWorldCentroid(this->newJoint->parent).Ign()
          - this->newJoint->line->Point(0);
    }
    // Set end point to mouse plane intersection
    else
    {
      ignition::math::Vector3d pt;
      camera->WorldPointOnPlane(_event.Pos().X(), _event.Pos().Y(),
          ignition::math::Planed(ignition::math::Vector3d(0, 0, 1)), pt);

      parentPos = this->GetLinkWorldCentroid(this->newJoint->parent).Ign()
          - this->newJoint->line->Point(0) - pt;
    }
    this->newJoint->line->SetPoint(1,
        (this->GetLinkWorldCentroid(this->hoverVis) - parentPos).Ign());
  }
  return true;
}

/////////////////////////////////////////////////
void JointMaker::OnOpenInspector()
{
  if (this->inspectName.empty())
    return;

  this->OpenInspector(this->inspectName);
  this->inspectName = "";
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
  rendering::VisualPtr vis = camera->GetVisual(_event.Pos());

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
    if (!this->selectedJoints.empty())
    {
      for (auto jointVis : this->selectedJoints)
      {
        this->RemoveJoint(jointVis->GetName());
      }
      this->DeselectAll();
      return true;
    }
  }

  return false;
}

/////////////////////////////////////////////////
void JointMaker::OnDelete()
{
  if (this->inspectName.empty())
    return;

  this->RemoveJoint(this->inspectName);
  this->inspectName = "";
}

/////////////////////////////////////////////////
std::string JointMaker::CreateHotSpot(JointData *_joint)
{
  if (!_joint)
    return "";

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

  double linkSize = std::min(0.1,
      _joint->parent->GetBoundingBox().GetSize().GetLength()*0.05);
  linkSize = std::max(linkSize, 0.01);

  double dimension = linkSize;
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
  _joint->inspector->SetJointId(_joint->hotspot->GetName());

  _joint->dirty = true;

  return jointId;
}

/////////////////////////////////////////////////
void JointMaker::Update()
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
  // Update each joint
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

           // Highlight links connected to joint being created if they have
           // been moved to another position
           if (joint == this->newJoint)
           {
             // Parent
             this->SetVisualMoved(joint->parent,
                 joint->parent->GetWorldPose().Ign() !=
                 this->parentLinkOriginalPose);

             // Child
             this->SetVisualMoved(joint->child,
                 joint->child->GetWorldPose().Ign() !=
                 this->childLinkOriginalPose);
           }
         }

        // Create / update joint visual
        if (joint->dirty || poseUpdate)
        {
          joint->Update();

          if (joint == this->newJoint && this->newJoint->parent &&
              this->newJoint->child && this->jointCreationDialog &&
              this->jointCreationDialog->isVisible())
          {
            // Get poses as homogeneous transforms
            ignition::math::Matrix4d parentWorld(
                this->newJoint->parent->GetWorldPose().Ign());
            ignition::math::Matrix4d childWorld(
                this->newJoint->child->GetWorldPose().Ign());

            // w_T_c = w_T_p * p_T_c
            // w_T_p^-1 * w_T_c = p_T_c
            ignition::math::Matrix4d childParent = parentWorld.Inverse() *
                childWorld;

            this->jointCreationDialog->UpdateRelativePose(childParent.Pose());
          }
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
    size_t pIdx = parentName.find("::");
    if (pIdx != std::string::npos)
      parentName = parentName.substr(pIdx+2);
    parentElem->Set(parentName);

    sdf::ElementPtr childElem = jointElem->GetElement("child");
    std::string childName = joint->child->GetName();
    size_t cIdx = childName.find("::");
    if (cIdx != std::string::npos)
      childName = childName.substr(cIdx+2);
    childElem->Set(childName);
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
  else if (_type == JOINT_GEARBOX)
  {
    return 2;
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
    const rendering::VisualPtr &_visual)
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
  // Get data from inspector
  msgs::Joint *inspectorMsg = this->inspector->Data();
  if (!inspectorMsg)
    return;

  this->jointMsg->CopyFrom(*inspectorMsg);

  // Name
  if (this->name != this->jointMsg->name())
    gui::model::Events::jointNameChanged(this->hotspot->GetName(),
        this->jointMsg->name());
  this->name = this->jointMsg->name();

  // Type
  this->type = JointMaker::ConvertJointType(
      msgs::ConvertJointType(this->jointMsg->type()));

  // Parent
  if (this->parent->GetName().find(this->jointMsg->parent()) ==
      std::string::npos)
  {
    // Get scoped name
    std::string oldName = this->parent->GetName();
    std::string scope = oldName;
    size_t idx = oldName.rfind("::");
    if (idx != std::string::npos)
      scope = oldName.substr(0, idx+2);

    rendering::VisualPtr parentVis = gui::get_active_camera()->GetScene()
        ->GetVisual(scope + this->jointMsg->parent());
    if (parentVis)
      this->parent = parentVis;
    else
      gzwarn << "Invalid parent, keeping old parent" << std::endl;
  }

  // Child
  if (this->child->GetName().find(this->jointMsg->child()) ==
      std::string::npos)
  {
    // Get scoped name
    std::string oldName = this->child->GetName();
    std::string scope = oldName;
    size_t idx = oldName.rfind("::");
    if (idx != std::string::npos)
      scope = oldName.substr(0, idx+2);

    rendering::VisualPtr childVis = gui::get_active_camera()->GetScene()
        ->GetVisual(scope + this->jointMsg->child());
    if (childVis)
    {
      this->child = childVis;
      if (this->jointVisual)
        childVis->AttachVisual(this->jointVisual);
    }
    else
      gzwarn << "Invalid child, keeping old child" << std::endl;
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
  this->inspector->Update(this->jointMsg);
  this->inspector->Open();
}

/////////////////////////////////////////////////
void JointData::Update()
{
  // Material
  std::string material = JointMaker::jointMaterials[this->type];

  // Hotspot and parent handle
  if (this->parent && this->child && this->hotspot && this->handles)
  {
    math::Vector3 parentOrigin = this->parent->GetWorldPose().pos;
    math::Vector3 childOrigin = this->child->GetWorldPose().pos;

    // Hotspot position
    math::Vector3 dPos = (childOrigin - parentOrigin);
    math::Vector3 center = dPos * 0.5;
    double length = std::max(dPos.GetLength(), 0.001);
    this->hotspot->SetScale(math::Vector3(0.008, 0.008, length));
    this->hotspot->SetWorldPosition(parentOrigin + center);

    // Hotspot orientation
    math::Vector3 u = dPos.Normalize();
    math::Vector3 v = math::Vector3::UnitZ;
    double cosTheta = v.Dot(u);
    double angle = acos(cosTheta);
    math::Vector3 w = (v.Cross(u)).Normalize();
    math::Quaternion q;
    q.SetFromAxis(w, angle);
    this->hotspot->SetWorldRotation(q);

    // Parent handle position
    this->handles->getBillboard(0)->setPosition(
        rendering::Conversions::Convert(parentOrigin -
        this->hotspot->GetWorldPose().pos));
    this->handles->_updateBounds();

    // set new material if joint type has changed
    if (this->hotspot->GetMaterialName() != material)
    {
      // Note: issue setting material when there is a billboard child,
      // seems to hang so detach before setting and re-attach later.
      Ogre::SceneNode *handleNode = this->handles->getParentSceneNode();
      this->handles->detachFromParent();
      this->hotspot->SetMaterial(material);
      this->hotspot->SetTransparency(0.7);
      handleNode->attachObject(this->handles);
      Ogre::MaterialPtr mat =
          Ogre::MaterialManager::getSingleton().getByName(material);
      Ogre::ColourValue color = mat->getTechnique(0)->getPass(0)->getDiffuse();
      color.a = 0.5;
      this->handles->getBillboard(0)->setColour(color);
    }
  }

  // Joint message and joint visual
  if (this->jointMsg)
  {
    this->UpdateMsg();

    // Update existing visual
    if (this->jointVisual)
    {
      this->jointVisual->UpdateFromMsg(this->jointMsg);
    }
    // Create joint visual
    else if (this->child)
    {
      std::string childName = this->child->GetName();
      std::string jointVisName = childName;
      size_t idx = childName.find("::");
      if (idx != std::string::npos)
        jointVisName = childName.substr(0, idx+2);
      jointVisName += "_JOINT_VISUAL_";

      gazebo::rendering::JointVisualPtr jointVis(
          new gazebo::rendering::JointVisual(jointVisName, this->child));
      jointVis->Load(this->jointMsg);
      this->jointVisual = jointVis;
    }
  }

  // Line
  if (this->line)
  {
    this->line->setMaterial(material);

    // Parent - child
    if (this->child && this->jointVisual)
    {
      this->line->SetPoint(0, (this->child->GetWorldPose().pos
          - this->child->GetParent()->GetWorldPose().pos).Ign());
      this->line->SetPoint(1,
          (this->jointVisual->GetWorldPose().pos
          - this->child->GetParent()->GetWorldPose().pos).Ign());
    }
    // Parent - mouse
    else if (this->parent && this->parent->GetParent())
    {
      ignition::math::Vector3d origin = (this->parent->GetWorldPose().pos
          - this->parent->GetParent()->GetWorldPose().pos).Ign();
      this->line->SetPoint(0, origin);
    }
  }

  // Notify joint changes
  if (this->parent && this->child && this->hotspot)
  {
    std::string parentName = this->parent->GetName();
    std::string childName = this->child->GetName();
    gui::model::Events::jointChanged(this->hotspot->GetName(), this->name,
        JointMaker::jointTypes[this->type], parentName, childName);
  }

  this->dirty = false;
}

/////////////////////////////////////////////////
void JointData::UpdateMsg()
{
  // Some values are only stored in the msg, so we keep those
  msgs::JointPtr oldMsg(new msgs::Joint);
  if (this->jointMsg)
  {
    oldMsg->CopyFrom(*this->jointMsg);
  }

  // Reset
  this->jointMsg.reset(new msgs::Joint);

  // Name
  this->jointMsg->set_name(this->name);

  // Parent
  if (this->parent)
  {
    std::string jointParentName = this->parent->GetName();
    std::string leafName = jointParentName;
    size_t pIdx = jointParentName.rfind("::");
    if (pIdx != std::string::npos)
      leafName = jointParentName.substr(pIdx+2);

    this->jointMsg->set_parent(leafName);
    this->jointMsg->set_parent_id(this->parent->GetId());
  }

  // Child
  if (this->child)
  {
    std::string jointChildName = this->child->GetName();
    std::string leafName = jointChildName;
    size_t pIdx = jointChildName.rfind("::");
    if (pIdx != std::string::npos)
      leafName = jointChildName.substr(pIdx+2);

    this->jointMsg->set_child(leafName);
    this->jointMsg->set_child_id(this->child->GetId());
  }

  // Pose
  if (oldMsg && oldMsg->has_pose())
  {
    this->jointMsg->mutable_pose()->CopyFrom(*(oldMsg->mutable_pose()));
  }
  else
  {
    msgs::Set(this->jointMsg->mutable_pose(), ignition::math::Pose3d::Zero);
  }

  // Type
  this->jointMsg->set_type(
      msgs::ConvertJointType(JointMaker::GetTypeAsString(this->type)));

  // Axes
  unsigned int axisCount = JointMaker::GetJointAxisCount(this->type);
  for (unsigned int i = 0; i < axisCount; ++i)
  {
    msgs::Axis *axisMsg;
    msgs::Axis *oldAxisMsg = NULL;
    if (i == 0u)
    {
      axisMsg = this->jointMsg->mutable_axis1();
      if (oldMsg && oldMsg->has_axis1())
        oldAxisMsg = oldMsg->mutable_axis1();
    }
    else if (i == 1u)
    {
      axisMsg = this->jointMsg->mutable_axis2();
      if (oldMsg && oldMsg->has_axis2())
        oldAxisMsg = oldMsg->mutable_axis2();
    }
    else
    {
      gzerr << "Invalid axis index["
            << i
            << "]"
            << std::endl;
      continue;
    }
    // Keep axis from previous msg if possible
    if (oldAxisMsg)
    {
      axisMsg->CopyFrom(*oldAxisMsg);
    }
    else
    {
      if (this->type == JointMaker::JOINT_GEARBOX)
      {
        msgs::Set(axisMsg->mutable_xyz(), ignition::math::Vector3d::UnitZ);
      }
      else
      {
        if (this->axes.size() < i+1)
        {
          this->axes.push_back(
              JointMaker::unitVectors[i%JointMaker::unitVectors.size()]);
        }
        msgs::Set(axisMsg->mutable_xyz(), this->axes[i]);
      }
      axisMsg->set_use_parent_model_frame(false);
      axisMsg->set_limit_lower(-GZ_DBL_MAX);
      axisMsg->set_limit_upper(GZ_DBL_MAX);
      axisMsg->set_limit_effort(-1);
      axisMsg->set_limit_velocity(-1);
      axisMsg->set_damping(0);
    }

    // Add angle field after we've checked that index i is valid
    this->jointMsg->add_angle(0);
  }

  // Others
  if (oldMsg && oldMsg->has_limit_erp())
  {
    this->jointMsg->set_limit_erp(oldMsg->limit_erp());
  }
  else
    this->jointMsg->set_limit_erp(0.2);

  if (oldMsg && oldMsg->has_suspension_erp())
  {
    this->jointMsg->set_suspension_erp(oldMsg->suspension_erp());
  }
  else
    this->jointMsg->set_suspension_erp(0.2);
}

/////////////////////////////////////////////////
void JointMaker::ShowContextMenu(const std::string &_name)
{
  auto it = this->joints.find(_name);
  if (it == this->joints.end())
    return;

  QMenu menu;
  if (this->inspectAct)
    menu.addAction(this->inspectAct);

  this->inspectName = _name;
  QAction *deleteAct = new QAction(tr("Delete"), this);
  connect(deleteAct, SIGNAL(triggered()), this, SLOT(OnDelete()));
  menu.addAction(deleteAct);

  menu.exec(QCursor::pos());
}

/////////////////////////////////////////////////
void JointMaker::OnSetSelectedEntity(const std::string &/*_name*/,
    const std::string &/*_mode*/)
{
  this->DeselectAll();
}

/////////////////////////////////////////////////
void JointMaker::OnSetSelectedJoint(const std::string &_name,
    const bool _selected)
{
  this->SetSelected(_name, _selected);
}

/////////////////////////////////////////////////
void JointMaker::SetSelected(const std::string &_name,
    const bool _selected)
{
  auto it = this->joints.find(_name);
  if (it == this->joints.end())
    return;

  this->SetSelected((*it).second->hotspot, _selected);
}

/////////////////////////////////////////////////
void JointMaker::SetSelected(rendering::VisualPtr _jointVis,
    const bool _selected)
{
  if (!_jointVis)
    return;

  _jointVis->SetHighlighted(_selected);
  auto it = std::find(this->selectedJoints.begin(),
      this->selectedJoints.end(), _jointVis);
  if (_selected)
  {
    if (it == this->selectedJoints.end())
    {
      this->selectedJoints.push_back(_jointVis);
      model::Events::setSelectedJoint(_jointVis->GetName(), _selected);
    }
  }
  else
  {
    if (it != this->selectedJoints.end())
    {
      this->selectedJoints.erase(it);
      model::Events::setSelectedJoint(_jointVis->GetName(), _selected);
    }
  }
}

/////////////////////////////////////////////////
void JointMaker::DeselectAll()
{
  while (!this->selectedJoints.empty())
  {
    rendering::VisualPtr vis = this->selectedJoints[0];
    vis->SetHighlighted(false);
    this->selectedJoints.erase(this->selectedJoints.begin());
    model::Events::setSelectedJoint(vis->GetName(), false);
  }
}

/////////////////////////////////////////////////
void JointMaker::CreateJointFromSDF(sdf::ElementPtr _jointElem,
    const std::string &_modelName)
{
  msgs::Joint jointMsg = msgs::JointFromSDF(_jointElem);

  // Parent
  std::string parentName = _modelName + "::" + jointMsg.parent();
  rendering::VisualPtr parentVis =
      gui::get_active_camera()->GetScene()->GetVisual(parentName);

  // Child
  std::string childName = _modelName + "::" + jointMsg.child();
  rendering::VisualPtr childVis =
      gui::get_active_camera()->GetScene()->GetVisual(childName);

  if (!parentVis || !childVis)
  {
    gzerr << "Unable to load joint. Joint child / parent not found"
        << std::endl;
    return;
  }

  JointData *joint = new JointData();
  joint->name = jointMsg.name();
  joint->parent = parentVis;
  joint->child = childVis;
  joint->type = this->ConvertJointType(msgs::ConvertJointType(jointMsg.type()));
  std::string jointVisName = _modelName + "::" + joint->name;

  joint->jointMsg.reset(new msgs::Joint);
  joint->jointMsg->CopyFrom(jointMsg);
  joint->jointMsg->set_parent_id(joint->parent->GetId());
  joint->jointMsg->set_child_id(joint->child->GetId());

  // Inspector
  joint->inspector = new JointInspector(this);
  joint->inspector->Update(joint->jointMsg);
  joint->inspector->setModal(false);
  connect(joint->inspector, SIGNAL(Applied()), joint, SLOT(OnApply()));

  // Visuals
  rendering::VisualPtr jointVis(
      new rendering::Visual(jointVisName, parentVis->GetParent()));
  jointVis->Load();
  rendering::DynamicLines *jointLine =
      jointVis->CreateDynamicLine(rendering::RENDERING_LINE_LIST);

  math::Vector3 origin = parentVis->GetWorldPose().pos
      - parentVis->GetParent()->GetWorldPose().pos;
  jointLine->AddPoint(origin.Ign());
  jointLine->AddPoint(origin.Ign() + ignition::math::Vector3d(0, 0, 0.1));

  jointVis->GetSceneNode()->setInheritScale(false);
  jointVis->GetSceneNode()->setInheritOrientation(false);
  joint->visual = jointVis;
  joint->line = jointLine;
  joint->dirty = true;

  auto jointId = this->CreateHotSpot(joint);

  // Notify other widgets
  if (!jointId.empty())
  {
    gui::model::Events::jointInserted(jointId, joint->name,
        jointTypes[joint->type], joint->parent->GetName(),
        joint->child->GetName());
  }
}

/////////////////////////////////////////////////
void JointMaker::OnLinkInserted(const std::string &_linkName)
{
  std::string leafName = _linkName;
  size_t idx = _linkName.rfind("::");
  if (idx != std::string::npos)
    leafName = _linkName.substr(idx+2);

  this->linkList[_linkName] = leafName;

  this->EmitLinkInserted(_linkName);
}

/////////////////////////////////////////////////
void JointMaker::OnLinkRemoved(const std::string &_linkName)
{
  if (this->linkList.erase(_linkName))
    this->EmitLinkRemoved(_linkName);
}

/////////////////////////////////////////////////
std::map<std::string, std::string> JointMaker::LinkList() const
{
  return this->linkList;
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
  this->DeselectAll();
}

/////////////////////////////////////////////////
bool JointMaker::SetParentLink(rendering::VisualPtr _parentLink)
{
  if (!_parentLink)
  {
    gzerr << "Parent link is null" << std::endl;
    return false;
  }

  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);

  // Choosing parent for the first time
  if (!this->newJoint)
  {
    // Create new line connecting parent to mouse
    this->newJoint = this->CreateJointLine("JOINT_LINE", _parentLink);
  }
  // Update parent of joint being created
  else if (this->newJoint->parent)
  {
    // Reset previous parent
    this->newJoint->parent->SetWorldPose(this->parentLinkOriginalPose);
    this->SetVisualMoved(this->newJoint->parent, false);

    this->newJoint->parent = _parentLink;
    this->newJoint->dirty = true;
  }
  else
  {
    gzerr << "There's a joint being created but the parent visual hasn't been "
        << "defined. This should never happen." << std::endl;
    return false;
  }

  this->parentLinkOriginalPose = _parentLink->GetWorldPose().Ign();
  return true;
}

/////////////////////////////////////////////////
bool JointMaker::SetChildLink(rendering::VisualPtr _childLink)
{
  if (!_childLink)
  {
    gzerr << "Child link can't be null" << std::endl;
    return false;
  }

  if (!this->newJoint || !this->newJoint->parent)
  {
    gzerr << "New joint must have a parent before a child" << std::endl;
    return false;
  }

  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);

  // Choosing child for the first time
  if (!this->newJoint->child)
  {
    rendering::VisualPtr parentVis = this->newJoint->parent;

    // Clear joint line connected to parent
    this->RemoveJoint("");

    // Create new joint with parent and child
    auto joint = this->CreateJoint(parentVis, _childLink);
    this->newJoint = joint;

    // Create hotspot visual
    this->CreateHotSpot(this->newJoint);
  }
  // Update child
  else
  {
    // Reset previous child
    this->newJoint->child->SetWorldPose(this->childLinkOriginalPose);
    this->SetVisualMoved(this->newJoint->child, false);

    this->newJoint->child = _childLink;
    this->newJoint->dirty = true;
    _childLink->AttachVisual(this->newJoint->jointVisual);
  }

  this->childLinkOriginalPose = _childLink->GetWorldPose().Ign();

  // Change state to not creating joint
  gui::Events::manipMode("select");
  this->jointType = JointMaker::JOINT_NONE;
  return true;
}

/////////////////////////////////////////////////
void JointMaker::OnType(const int _typeInt)
{
  this->jointType = static_cast<JointMaker::JointType>(_typeInt);

  if (this->newJoint && this->jointType != JOINT_NONE)
  {
    this->newJoint->type = this->jointType;
    this->newJoint->dirty = true;
  }
}

/////////////////////////////////////////////////
void JointMaker::SetAxis(const QString &_axis,
      const ignition::math::Vector3d &_value)
{
  if (this->newJoint && this->newJoint->jointMsg)
  {
    if (_axis == "axis1" && this->newJoint->jointMsg->has_axis1())
    {
      msgs::Set(this->newJoint->jointMsg->mutable_axis1()->mutable_xyz(),
        _value);
      this->newJoint->axes[0] = _value;
    }
    else if (_axis == "axis2" && this->newJoint->jointMsg->has_axis2())
    {
      msgs::Set(this->newJoint->jointMsg->mutable_axis2()->mutable_xyz(),
        _value);
      this->newJoint->axes[1] = _value;
    }
    this->newJoint->dirty = true;
  }
}

/////////////////////////////////////////////////
void JointMaker::SetJointPose(const ignition::math::Pose3d &_pose)
{
  if (this->newJoint && this->newJoint->jointMsg)
  {
    msgs::Set(this->newJoint->jointMsg->mutable_pose(), _pose);
    this->newJoint->dirty = true;
  }
}

/////////////////////////////////////////////////
void JointMaker::SetParentLink(const std::string &_name)
{
  auto vis = this->LinkVisualFromName(_name);

  if (vis)
    this->SetParentLink(vis);
}

/////////////////////////////////////////////////
void JointMaker::SetChildLink(const std::string &_name)
{
  auto vis = this->LinkVisualFromName(_name);

  if (vis)
    this->SetChildLink(vis);
}

/////////////////////////////////////////////////
rendering::VisualPtr JointMaker::LinkVisualFromName(const std::string &_name)
{
  // Get scoped name
  std::string scopedName;
  for (auto link : this->linkList)
  {
    if (link.second == _name || link.first == _name)
    {
      scopedName = link.first;
      break;
    }
  }

  if (scopedName.empty())
    return NULL;

  // Get visual
  rendering::ScenePtr scene = rendering::get_scene();

  if (!scene)
    return NULL;

  return scene->GetVisual(scopedName);
}

/////////////////////////////////////////////////
void JointMaker::SetLinksRelativePose(const ignition::math::Pose3d &_pose,
    const bool _reset)
{
  if (!this->newJoint || !this->newJoint->parent || !this->newJoint->child)
  {
    gzerr << "Can't set relative pose without new joint's parent and child "
        << "links." << std::endl;
    return;
  }

  ignition::math::Pose3d newChildPose;

  if (_reset)
  {
    newChildPose = this->childLinkOriginalPose;
    this->newJoint->parent->SetWorldPose(this->parentLinkOriginalPose);
  }
  else
  {
    // Get poses as homogeneous transforms
    ignition::math::Matrix4d parent_world(
        this->newJoint->parent->GetWorldPose().Ign());
    ignition::math::Matrix4d child_parent(_pose);

    // w_T_c = w_T_p * p_T_c
    ignition::math::Matrix4d child_world =
        parent_world * child_parent;

    newChildPose = child_world.Pose();
  }

  this->newJoint->child->SetWorldPose(newChildPose);
}

/////////////////////////////////////////////////
void JointMaker::AlignLinks(const bool _childToParent,
    const std::string &_axis, const std::string &_config, const bool _reverse)
{
  if (!this->newJoint || !this->newJoint->parent || !this->newJoint->child)
  {
    gzerr << "Couldn't find new joint's parent and child links to be aligned."
        << std::endl;
    return;
  }

  std::vector<rendering::VisualPtr> links;
  links.push_back(this->newJoint->parent);
  links.push_back(this->newJoint->child);

  std::string target = _childToParent ? "first" : "last";

  ModelAlign::Instance()->AlignVisuals(links, _axis, _config,
      target, true, _reverse);
}

/////////////////////////////////////////////////
void JointMaker::SetVisualMoved(rendering::VisualPtr _vis, const bool _moved)
{
  if (_vis->GetChildCount() != 0)
  {
    for (unsigned int j = 0; j < _vis->GetChildCount(); ++j)
    {
      this->SetVisualMoved(_vis->GetChild(j), _moved);
    }
  }
  else
  {
    if (_moved)
    {
      _vis->SetEmissive(common::Color(0, 0, 1, 1));
    }
    else
    {
      _vis->SetEmissive(common::Color(0, 0, 0, 1));
    }
  }
}

/////////////////////////////////////////////////
void JointMaker::FinalizeCreation()
{
  gui::model::Events::modelChanged();
  this->jointType = JointMaker::JOINT_NONE;

  // Notify schematic view and palette list
  if (this->newJoint && this->newJoint->hotspot &&
      this->newJoint->child && this->newJoint->parent)
  {
    gui::model::Events::jointInserted(
        this->newJoint->hotspot->GetName(),
        this->newJoint->name,
        this->jointTypes[this->newJoint->type],
        this->newJoint->parent->GetName(),
        this->newJoint->child->GetName());


    // Reset visuals
    this->SetVisualMoved(this->newJoint->parent, false);
    this->SetVisualMoved(this->newJoint->child, false);
  }
  this->newJoint = NULL;

  // Notify ModelEditor to uncheck tool button
  this->JointAdded();

  this->Stop();
}
