/*
 * Copyright 2013 Open Source Robotics Foundation
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
#include <boost/filesystem.hpp>

#include "gazebo/common/KeyEvent.hh"
#include "gazebo/common/Exception.hh"

#include "gazebo/rendering/UserCamera.hh"
#include "gazebo/rendering/Visual.hh"
#include "gazebo/rendering/Scene.hh"

#include "gazebo/math/Quaternion.hh"

#include "gazebo/transport/Publisher.hh"
#include "gazebo/transport/Node.hh"

#include "gazebo/physics/Inertial.hh"

#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/KeyEventHandler.hh"
#include "gazebo/gui/MouseEventHandler.hh"
#include "gazebo/gui/GuiEvents.hh"
#include "gazebo/gui/GuiIface.hh"
#include "gazebo/gui/ModelManipulator.hh"

#include "gazebo/gui/model/ModelData.hh"
#include "gazebo/gui/model/PartGeneralTab.hh"
#include "gazebo/gui/model/PartVisualTab.hh"
#include "gazebo/gui/model/PartInspector.hh"
#include "gazebo/gui/model/JointMaker.hh"
#include "gazebo/gui/model/ModelCreator.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
ModelCreator::ModelCreator()
{
  this->modelName = "";

  this->modelTemplateSDF.reset(new sdf::SDF);
  this->modelTemplateSDF->SetFromString(this->GetTemplateSDFString());

  this->boxCounter = 0;
  this->cylinderCounter = 0;
  this->sphereCounter = 0;
  this->modelCounter = 0;

  this->node = transport::NodePtr(new transport::Node());
  this->node->Init();
  this->makerPub = this->node->Advertise<msgs::Factory>("~/factory");
  this->requestPub = this->node->Advertise<msgs::Request>("~/request");

  this->jointMaker = new JointMaker();

  this->inspectAct = new QAction(tr("Open Part Inspector"), this);
  connect(this->inspectAct, SIGNAL(triggered()), this,
      SLOT(OnOpenInspector()));

  connect(g_deleteAct, SIGNAL(DeleteSignal(const std::string &)), this,
          SLOT(OnDelete(const std::string &)));

  this->Reset();
}

/////////////////////////////////////////////////
ModelCreator::~ModelCreator()
{
  this->Reset();
  this->node->Fini();
  this->node.reset();
  this->modelTemplateSDF.reset();
  this->requestPub.reset();
  this->makerPub.reset();

  delete jointMaker;
}

/////////////////////////////////////////////////
std::string ModelCreator::CreateModel()
{
  this->Reset();
  return this->modelName;
}

/////////////////////////////////////////////////
void ModelCreator::AddJoint(const std::string &_type)
{
  this->Stop();
  if (this->jointMaker)
    this->jointMaker->AddJoint(_type);
}

/////////////////////////////////////////////////
std::string ModelCreator::AddBox(const math::Vector3 &_size,
    const math::Pose &_pose)
{
  if (!this->modelVisual)
  {
    this->Reset();
  }

  std::ostringstream linkNameStream;
  linkNameStream << "unit_box_" << this->boxCounter++;
  std::string linkName = linkNameStream.str();

  rendering::VisualPtr linkVisual(new rendering::Visual(linkName,
      this->modelVisual));
  linkVisual->Load();

  std::ostringstream visualName;
  visualName << linkName << "_visual";
  rendering::VisualPtr visVisual(new rendering::Visual(visualName.str(),
      linkVisual));
  sdf::ElementPtr visualElem =  this->modelTemplateSDF->root
      ->GetElement("model")->GetElement("link")->GetElement("visual");
  visualElem->GetElement("material")->GetElement("script")
      ->GetElement("name")->Set("Gazebo/GreyTransparent");

  sdf::ElementPtr geomElem =  visualElem->GetElement("geometry");
  geomElem->ClearElements();
  ((geomElem->AddElement("box"))->AddElement("size"))->Set(_size);

  visVisual->Load(visualElem);

  linkVisual->SetPose(_pose);
  if (_pose == math::Pose::Zero)
  {
    linkVisual->SetPosition(math::Vector3(_pose.pos.x, _pose.pos.y,
    _pose.pos.z + _size.z/2));
  }

  this->CreatePart(visVisual);

  this->mouseVisual = linkVisual;

  return linkName;
}

/////////////////////////////////////////////////
std::string ModelCreator::AddSphere(double _radius,
    const math::Pose &_pose)
{
  if (!this->modelVisual)
    this->Reset();

  std::ostringstream linkNameStream;
  linkNameStream << "unit_sphere_" << this->sphereCounter++;
  std::string linkName = linkNameStream.str();

  rendering::VisualPtr linkVisual(new rendering::Visual(
      linkName, this->modelVisual));
  linkVisual->Load();

  std::ostringstream visualName;
  visualName << linkName << "_visual";
  rendering::VisualPtr visVisual(new rendering::Visual(visualName.str(),
        linkVisual));
  sdf::ElementPtr visualElem =  this->modelTemplateSDF->root
      ->GetElement("model")->GetElement("link")->GetElement("visual");
  visualElem->GetElement("material")->GetElement("script")
      ->GetElement("name")->Set("Gazebo/GreyTransparent");

  sdf::ElementPtr geomElem =  visualElem->GetElement("geometry");
  geomElem->ClearElements();
  ((geomElem->AddElement("sphere"))->GetElement("radius"))->Set(_radius);

  visVisual->Load(visualElem);

  linkVisual->SetPose(_pose);
  if (_pose == math::Pose::Zero)
  {
    linkVisual->SetPosition(math::Vector3(_pose.pos.x, _pose.pos.y,
    _pose.pos.z + _radius));
  }

  this->CreatePart(visVisual);
  this->mouseVisual = linkVisual;

  return linkName;
}

/////////////////////////////////////////////////
std::string ModelCreator::AddCylinder(double _radius, double _length,
    const math::Pose &_pose)
{
  if (!this->modelVisual)
    this->Reset();

  std::ostringstream linkNameStream;
  linkNameStream << "unit_cylinder_" << this->cylinderCounter++;
  std::string linkName = linkNameStream.str();

  rendering::VisualPtr linkVisual(new rendering::Visual(
      linkName, this->modelVisual));
  linkVisual->Load();

  std::ostringstream visualName;
  visualName << linkName << "_visual";
  rendering::VisualPtr visVisual(new rendering::Visual(visualName.str(),
        linkVisual));
  sdf::ElementPtr visualElem =  this->modelTemplateSDF->root
      ->GetElement("model")->GetElement("link")->GetElement("visual");
  visualElem->GetElement("material")->GetElement("script")
      ->GetElement("name")->Set("Gazebo/GreyTransparent");

  sdf::ElementPtr geomElem =  visualElem->GetElement("geometry");
  geomElem->ClearElements();
  sdf::ElementPtr cylinderElem = geomElem->AddElement("cylinder");
  (cylinderElem->GetElement("radius"))->Set(_radius);
  (cylinderElem->GetElement("length"))->Set(_length);

  visVisual->Load(visualElem);

  linkVisual->SetPose(_pose);
  if (_pose == math::Pose::Zero)
  {
    linkVisual->SetPosition(math::Vector3(_pose.pos.x, _pose.pos.y,
    _pose.pos.z + _length/2));
  }

  this->CreatePart(visVisual);
  this->mouseVisual = linkVisual;

  return linkName;
}

/////////////////////////////////////////////////
std::string ModelCreator::AddCustom(const std::string &_path,
    const math::Vector3 &_scale, const math::Pose &_pose)
{
  if (!this->modelVisual)
    this->Reset();

  std::string path = _path;

  std::ostringstream linkNameStream;
  linkNameStream << "custom_" << this->customCounter++;
  std::string linkName = linkNameStream.str();

  rendering::VisualPtr linkVisual(new rendering::Visual(this->modelName + "::" +
        linkName, this->modelVisual));
  linkVisual->Load();

  std::ostringstream visualName;
  visualName << linkName << "_visual";
  rendering::VisualPtr visVisual(new rendering::Visual(visualName.str(),
        linkVisual));
  sdf::ElementPtr visualElem =  this->modelTemplateSDF->root
      ->GetElement("model")->GetElement("link")->GetElement("visual");
  visualElem->GetElement("material")->GetElement("script")
      ->GetElement("name")->Set("Gazebo/GreyTransparent");

  sdf::ElementPtr geomElem =  visualElem->GetElement("geometry");
  geomElem->ClearElements();
  sdf::ElementPtr meshElem = geomElem->AddElement("mesh");
  meshElem->GetElement("scale")->Set(_scale);
  meshElem->GetElement("uri")->Set(path);
  visVisual->Load(visualElem);

  linkVisual->SetPose(_pose);
  if (_pose == math::Pose::Zero)
  {
    linkVisual->SetPosition(math::Vector3(_pose.pos.x, _pose.pos.y,
    _pose.pos.z + _scale.z/2));
  }

  this->CreatePart(visVisual);
  this->mouseVisual = linkVisual;

  return linkName;
}

/////////////////////////////////////////////////
void ModelCreator::CreatePart(const rendering::VisualPtr &_visual)
{
  PartData *part = new PartData();

  part->partVisual = _visual->GetParent();
  part->visuals.push_back(_visual);

  part->SetName(part->partVisual->GetName());
  part->SetPose(part->partVisual->GetWorldPose());
  part->SetGravity(true);
  part->SetSelfCollide(false);
  part->SetKinematic(false);

  part->inspector = new PartInspector;
  part->inspector->SetName(part->GetName());
  part->inspector->setModal(false);
  connect(part->inspector, SIGNAL(Applied()),
      part, SLOT(OnApply()));

  connect(part->inspector->GetVisual(), SIGNAL(VisualAdded()), part,
      SLOT(OnAddVisual()));
  connect(part->inspector->GetVisual(),
      SIGNAL(VisualRemoved(const std::string &)), part,
      SLOT(OnRemoveVisual(const std::string &)));

  //part->inertial.reset(new physics::Inertial);
  //CollisionData *collisionData = new CollisionData;
  //part->collisions.push_back(collisionData);
  //part->sensorData = new SensorData;

  this->allParts[part->GetName()] = part;
}

/////////////////////////////////////////////////
void ModelCreator::RemovePart(const std::string &_partName)
{
  if (!this->modelVisual)
  {
    this->Reset();
    return;
  }

  if (this->allParts.find(_partName) == this->allParts.end())
    return;

  PartData *part = this->allParts[_partName];
  if (!part)
    return;

  if (part->partVisual == this->selectedVis)
  {
    this->selectedVis->SetHighlighted(false);
    this->selectedVis.reset();
  }

  rendering::ScenePtr scene = part->partVisual->GetScene();
  for (unsigned int i = 0; i < part->visuals.size(); ++i)
  {
    rendering::VisualPtr vis = part->visuals[i];
    scene->RemoveVisual(vis);
  }
  scene->RemoveVisual(part->partVisual);
  part->visuals.clear();
  part->partVisual.reset();

  for (unsigned int i = 0; i < part->collisions.size(); ++i)
  {
    delete part->collisions[i];
  }
  part->collisions.clear();

  delete part->inspector;
  //part->inertial.reset();
  //delete part->sensorData;

  this->allParts.erase(_partName);
}

/////////////////////////////////////////////////
void ModelCreator::Reset()
{
  if (!gui::get_active_camera() ||
      !gui::get_active_camera()->GetScene())
    return;

  KeyEventHandler::Instance()->AddPressFilter("model_part",
      boost::bind(&ModelCreator::OnKeyPressPart, this, _1));

  MouseEventHandler::Instance()->AddReleaseFilter("model_part",
      boost::bind(&ModelCreator::OnMouseReleasePart, this, _1));

  MouseEventHandler::Instance()->AddMoveFilter("model_part",
      boost::bind(&ModelCreator::OnMouseMovePart, this, _1));

  MouseEventHandler::Instance()->AddDoubleClickFilter("model_part",
      boost::bind(&ModelCreator::OnMouseDoubleClickPart, this, _1));

  this->jointMaker->Reset();
  this->selectedVis.reset();

  std::stringstream ss;
  ss << "defaultModel_" << this->modelCounter++;
  this->modelName = ss.str();

  rendering::ScenePtr scene = gui::get_active_camera()->GetScene();

  this->isStatic = false;
  this->autoDisable = true;

  while (this->allParts.size() > 0)
    this->RemovePart(this->allParts.begin()->first);
  this->allParts.clear();

  if (this->modelVisual)
    scene->RemoveVisual(this->modelVisual);

  this->modelVisual.reset(new rendering::Visual(this->modelName,
      scene->GetWorldVisual()));

  this->modelVisual->Load();
  this->modelPose = math::Pose::Zero;
  this->modelVisual->SetPose(this->modelPose);
  scene->AddVisual(this->modelVisual);
}

/////////////////////////////////////////////////
void ModelCreator::SetModelName(const std::string &_modelName)
{
  this->modelName = _modelName;
}

/////////////////////////////////////////////////
std::string ModelCreator::GetModelName() const
{
  return this->modelName;
}

/////////////////////////////////////////////////
void ModelCreator::SetStatic(bool _static)
{
  this->isStatic = _static;
}

/////////////////////////////////////////////////
void ModelCreator::SetAutoDisable(bool _auto)
{
  this->autoDisable = _auto;
}

/////////////////////////////////////////////////
void ModelCreator::SaveToSDF(const std::string &_savePath)
{
  std::ofstream savefile;
  boost::filesystem::path path;
  path = boost::filesystem::operator/(_savePath, this->modelName + ".sdf");
  savefile.open(path.string().c_str());
  if (savefile.is_open())
  {
    savefile << this->modelSDF->ToString();
    savefile.close();
  }
  else
  {
    gzerr << "Unable to open file for writing: '" << path.string().c_str()
        << "'. Possibly a permission issue." << std::endl;
  }
}

/////////////////////////////////////////////////
void ModelCreator::FinishModel()
{
  event::Events::setSelectedEntity("", "normal");
  this->Reset();
  this->CreateTheEntity();
}

/////////////////////////////////////////////////
void ModelCreator::CreateTheEntity()
{
  msgs::Factory msg;
  msg.set_sdf(this->modelSDF->ToString());
  this->makerPub->Publish(msg);
}

/////////////////////////////////////////////////
std::string ModelCreator::GetTemplateSDFString()
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
void ModelCreator::AddPart(PartType _type)
{
  this->Stop();

  this->addPartType = _type;
  if (_type != PART_NONE)
  {
    switch (_type)
    {
      case PART_BOX:
      {
        this->AddBox();
        break;
      }
      case PART_SPHERE:
      {
        this->AddSphere();
        break;
      }
      case PART_CYLINDER:
      {
        this->AddCylinder();
        break;
      }
      default:
      {
        gzwarn << "Unknown part type '" << _type << "'. " <<
            "Part not added" << std::endl;
        break;
      }
    }
  }
}

/////////////////////////////////////////////////
void ModelCreator::Stop()
{
  if (this->addPartType != PART_NONE && this->mouseVisual)
  {
    for (unsigned int i = 0; i < this->mouseVisual->GetChildCount(); ++i)
        this->RemovePart(this->mouseVisual->GetName());
    this->mouseVisual.reset();
  }
  if (this->jointMaker)
    this->jointMaker->Stop();
}

/////////////////////////////////////////////////
void ModelCreator::OnDelete(const std::string &_entity)
{
  // check if it's our model
  if (_entity == this->modelName)
  {
    this->Reset();
    return;
  }

  // if it's a link
  if (this->allParts.find(_entity) != this->allParts.end())
  {
    if (this->jointMaker)
      this->jointMaker->RemoveJointsByPart(_entity);
    this->RemovePart(_entity);
    return;
  }

  // if it's a visual
  rendering::VisualPtr vis =
      gui::get_active_camera()->GetScene()->GetVisual(_entity);
  if (vis)
  {
    rendering::VisualPtr parentLink = vis->GetParent();
    if (this->allParts.find(parentLink->GetName()) != this->allParts.end())
    {
      // remove the parent link if it's the only child
      if (parentLink->GetChildCount() == 1)
      {
        if (this->jointMaker)
          this->jointMaker->RemoveJointsByPart(parentLink->GetName());
        this->RemovePart(parentLink->GetName());
        return;
      }
    }
  }
}

/////////////////////////////////////////////////
bool ModelCreator::OnKeyPressPart(const common::KeyEvent &_event)
{
  if (_event.key == Qt::Key_Escape)
  {
    this->Stop();
  }
  else if (_event.key == Qt::Key_Delete)
  {
    if (this->selectedVis)
    {
      this->OnDelete(this->selectedVis->GetName());
      this->selectedVis.reset();
    }
  }
  return false;
}

/////////////////////////////////////////////////
bool ModelCreator::OnMouseReleasePart(const common::MouseEvent &_event)
{
  if (this->jointMaker->GetState() != JointMaker::JOINT_NONE)
    return false;

  if (this->mouseVisual)
  {
    // set the part data pose
    if (this->allParts.find(this->mouseVisual->GetName()) !=
        this->allParts.end())
    {
      PartData *part = this->allParts[this->mouseVisual->GetName()];
      part->SetPose(this->mouseVisual->GetWorldPose());
    }

    // reset and return
    emit PartAdded();
    this->mouseVisual.reset();
    this->AddPart(PART_NONE);
    return true;
  }

  // In mouse normal mode, let users select a part if the parent model
  // is currently selected.
  rendering::VisualPtr vis = gui::get_active_camera()->GetVisual(_event.pos);
  if (vis)
  {
    if (this->allParts.find(vis->GetParent()->GetName()) !=
        this->allParts.end())
    {
      // trigger part inspector on right click
      if (_event.button == common::MouseEvent::RIGHT)
      {
        this->inspectVis = vis->GetParent();
        QMenu menu;
        menu.addAction(this->inspectAct);
        menu.exec(QCursor::pos());
        return true;
      }

      // if the model is selected
      if (gui::get_active_camera()->GetScene()->GetSelectedVisual()
          == this->modelVisual || this->selectedVis)
      {
        if (this->selectedVis)
        {
          this->selectedVis->SetHighlighted(false);
        }
        else
        {
          // turn off model selection so we don't end up with
          // both part and model selected at the same time
          event::Events::setSelectedEntity("", "normal");
        }

        this->selectedVis = vis->GetParent();
        this->selectedVis->SetHighlighted(true);
        return true;
      }
    }
    else if (this->selectedVis)
    {
      this->selectedVis->SetHighlighted(false);
      this->selectedVis.reset();
    }
  }
  return false;
}

/////////////////////////////////////////////////
bool ModelCreator::OnMouseMovePart(const common::MouseEvent &_event)
{
  if (!this->mouseVisual)
    return false;

  if (!gui::get_active_camera())
    return false;

  math::Pose pose = this->mouseVisual->GetWorldPose();
  pose.pos = ModelManipulator::GetMousePositionOnPlane(
      gui::get_active_camera(), _event);

  if (!_event.shift)
  {
    pose.pos = ModelManipulator::SnapPoint(pose.pos);
  }
  pose.pos.z = this->mouseVisual->GetWorldPose().pos.z;

  this->mouseVisual->SetWorldPose(pose);

  return true;
}

/////////////////////////////////////////////////
bool ModelCreator::OnMouseDoubleClickPart(const common::MouseEvent &_event)
{
  // open the part inspector on double click
 rendering::VisualPtr vis = gui::get_active_camera()->GetVisual(_event.pos);
  if (!vis)
    return false;

  if (this->allParts.find(vis->GetParent()->GetName()) !=
      this->allParts.end())
  {
    if (this->selectedVis)
      this->selectedVis->SetHighlighted(false);

    this->OpenInspector(vis->GetParent()->GetName());
    return true;
  }

  return false;
}

/////////////////////////////////////////////////
void ModelCreator::OnOpenInspector()
{
  this->OpenInspector(this->inspectVis->GetName());
  this->inspectVis.reset();
}

/////////////////////////////////////////////////
void ModelCreator::OpenInspector(const std::string &_name)
{
  PartData *part = this->allParts[_name];
  PartGeneralTab *generalTab = part->inspector->GetGeneral();
  generalTab->SetGravity(part->GetGravity());
  generalTab->SetSelfCollide(part->GetSelfCollide());
  generalTab->SetKinematic(part->GetKinematic());
  generalTab->SetPose(part->GetPose());
  generalTab->SetMass(part->GetMass());
  generalTab->SetInertialPose(part->GetInertialPose());
  generalTab->SetInertia(part->GetInertiaIXX(), part->GetInertiaIYY(),
      part->GetInertiaIZZ(), part->GetInertiaIXY(),
      part->GetInertiaIXZ(), part->GetInertiaIYZ());

  PartVisualTab *visualTab = part->inspector->GetVisual();

  for (unsigned int i = 0; i < part->visuals.size(); ++i)
  {
    if (i >= visualTab->GetVisualCount())
      visualTab->AddVisual();
    visualTab->SetName(i, part->visuals[i]->GetName());
    visualTab->SetPose(i, part->visuals[i]->GetPose());
    visualTab->SetTransparency(i, part->visuals[i]->GetTransparency());
    visualTab->SetMaterial(i, part->visuals[i]->GetMaterialName());
    visualTab->SetGeometry(i, part->visuals[i]->GetMeshName());
  }
  part->inspector->show();
}


/////////////////////////////////////////////////
JointMaker *ModelCreator::GetJointMaker() const
{
  return this->jointMaker;
}

/////////////////////////////////////////////////
void ModelCreator::GenerateSDF()
{
  sdf::ElementPtr modelElem;
  sdf::ElementPtr linkElem;

  this->modelSDF.reset(new sdf::SDF);
  this->modelSDF->SetFromString(this->GetTemplateSDFString());

  modelElem = this->modelSDF->root->GetElement("model");

  linkElem = modelElem->GetElement("link");
  sdf::ElementPtr templateLinkElem = linkElem->Clone();
  sdf::ElementPtr templateVisualElem = templateLinkElem->GetElement(
      "visual")->Clone();
  sdf::ElementPtr templateCollisionElem = templateLinkElem->GetElement(
      "collision")->Clone();
  modelElem->ClearElements();
  std::stringstream visualNameStream;
  std::stringstream collisionNameStream;

  modelElem->GetAttribute("name")->Set(this->modelName);

  boost::unordered_map<std::string, PartData *>::iterator partsIt;

  // set center of all parts to be origin
  math::Vector3 mid;
  for (partsIt = this->allParts.begin(); partsIt != this->allParts.end();
      ++partsIt)
  {
    PartData *part = partsIt->second;
    mid += part->GetPose().pos;
  }
  mid /= this->allParts.size();
  this->origin.pos = mid;
  modelElem->GetElement("pose")->Set(this->origin);

  // loop through all parts and generate sdf
  for (partsIt = this->allParts.begin(); partsIt != this->allParts.end();
      ++partsIt)
  {
    visualNameStream.str("");
    collisionNameStream.str("");

    PartData *part = partsIt->second;
    sdf::ElementPtr newLinkElem = templateLinkElem->Clone();
    newLinkElem->ClearElements();
    newLinkElem->GetAttribute("name")->Set(part->GetName());
    newLinkElem->GetElement("pose")->Set(part->GetPose() - this->origin);
    newLinkElem->GetElement("gravity")->Set(part->GetGravity());
    newLinkElem->GetElement("self_collide")->Set(part->GetSelfCollide());
    newLinkElem->GetElement("kinematic")->Set(part->GetKinematic());
    sdf::ElementPtr inertialElem = newLinkElem->GetElement("inertial");
    inertialElem->GetElement("mass")->Set(part->GetMass());
    inertialElem->GetElement("pose")->Set(part->GetInertialPose());
    sdf::ElementPtr inertiaElem = inertialElem->GetElement("inertia");
    inertiaElem->GetElement("ixx")->Set(part->GetInertiaIXX());
    inertiaElem->GetElement("iyy")->Set(part->GetInertiaIYY());
    inertiaElem->GetElement("izz")->Set(part->GetInertiaIZZ());
    inertiaElem->GetElement("ixy")->Set(part->GetInertiaIXY());
    inertiaElem->GetElement("ixz")->Set(part->GetInertiaIXZ());
    inertiaElem->GetElement("iyz")->Set(part->GetInertiaIYZ());

    modelElem->InsertElement(newLinkElem);

    for (unsigned int i = 0; i < part->visuals.size(); ++i)
    {
      sdf::ElementPtr visualElem = templateVisualElem->Clone();
      sdf::ElementPtr collisionElem = templateCollisionElem->Clone();

      rendering::VisualPtr visual = part->visuals[i];

      visualElem->GetAttribute("name")->Set(visual->GetName());
      collisionElem->GetAttribute("name")->Set(
          visual->GetParent()->GetName() + "_collision");
      visualElem->GetElement("pose")->Set(visual->GetPose());
      collisionElem->GetElement("pose")->Set(visual->GetPose());

      sdf::ElementPtr geomElem =  visualElem->GetElement("geometry");
      geomElem->ClearElements();

      math::Vector3 scale = visual->GetScale();
      if (visual->GetParent()->GetName().find("unit_box") != std::string::npos)
      {
        sdf::ElementPtr boxElem = geomElem->AddElement("box");
        (boxElem->GetElement("size"))->Set(scale);
      }
      else if (visual->GetParent()->GetName().find("unit_cylinder")
         != std::string::npos)
      {
        sdf::ElementPtr cylinderElem = geomElem->AddElement("cylinder");
        (cylinderElem->GetElement("radius"))->Set(scale.x/2.0);
        (cylinderElem->GetElement("length"))->Set(scale.z);
      }
      else if (visual->GetParent()->GetName().find("unit_sphere")
          != std::string::npos)
      {
        sdf::ElementPtr sphereElem = geomElem->AddElement("sphere");
        (sphereElem->GetElement("radius"))->Set(scale.x/2.0);
      }
      else if (visual->GetParent()->GetName().find("custom")
          != std::string::npos)
      {
        sdf::ElementPtr customElem = geomElem->AddElement("mesh");
        (customElem->GetElement("scale"))->Set(scale);
        (customElem->GetElement("uri"))->Set(visual->GetMeshName());
      }
      sdf::ElementPtr geomElemClone = geomElem->Clone();
      geomElem =  collisionElem->GetElement("geometry");
      geomElem->ClearElements();
      geomElem->InsertElement(geomElemClone->GetFirstElement());

      newLinkElem->InsertElement(visualElem);
      newLinkElem->InsertElement(collisionElem);
    }
  }

  // Add joint sdf elements
  this->jointMaker->GenerateSDF();
  sdf::ElementPtr jointsElem = this->jointMaker->GetSDF();

  sdf::ElementPtr jointElem;
  if (jointsElem->HasElement("joint"))
    jointElem = jointsElem->GetElement("joint");
  while (jointElem)
  {
    modelElem->InsertElement(jointElem->Clone());
    jointElem = jointElem->GetNextElement("joint");
  }

  // Model settings
  modelElem->GetElement("static")->Set(this->isStatic);
  modelElem->GetElement("allow_auto_disable")->Set(this->autoDisable);
}
