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
#include <boost/filesystem.hpp>
#include <sstream>
#include <string>

#include "gazebo/common/Exception.hh"
#include "gazebo/common/SVGLoader.hh"

#include "gazebo/rendering/UserCamera.hh"
#include "gazebo/rendering/Material.hh"
#include "gazebo/rendering/Scene.hh"

#include "gazebo/math/Quaternion.hh"

#include "gazebo/transport/Publisher.hh"
#include "gazebo/transport/Node.hh"
#include "gazebo/transport/TransportIface.hh"

#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/KeyEventHandler.hh"
#include "gazebo/gui/MouseEventHandler.hh"
#include "gazebo/gui/GuiEvents.hh"
#include "gazebo/gui/GuiIface.hh"
#include "gazebo/gui/ModelManipulator.hh"
#include "gazebo/gui/ModelSnap.hh"
#include "gazebo/gui/ModelAlign.hh"
#include "gazebo/gui/SaveDialog.hh"
#include "gazebo/gui/MainWindow.hh"

#include "gazebo/gui/model/ModelData.hh"
#include "gazebo/gui/model/LinkInspector.hh"
#include "gazebo/gui/model/JointMaker.hh"
#include "gazebo/gui/model/ModelEditorEvents.hh"
#include "gazebo/gui/model/ModelCreator.hh"

using namespace gazebo;
using namespace gui;

const std::string ModelCreator::modelDefaultName = "Untitled";
const std::string ModelCreator::previewName = "ModelPreview";

/////////////////////////////////////////////////
ModelCreator::ModelCreator()
{
  this->active = false;

  this->modelTemplateSDF.reset(new sdf::SDF);
  this->modelTemplateSDF->SetFromString(ModelData::GetTemplateSDFString());

  this->updateMutex = new boost::recursive_mutex();

  this->manipMode = "";
  this->linkCounter = 0;
  this->modelCounter = 0;

  this->node = transport::NodePtr(new transport::Node());
  this->node->Init();
  this->makerPub = this->node->Advertise<msgs::Factory>("~/factory");
  this->requestPub = this->node->Advertise<msgs::Request>("~/request");

  this->jointMaker = new JointMaker();

  connect(g_editModelAct, SIGNAL(toggled(bool)), this, SLOT(OnEdit(bool)));
  this->inspectAct = new QAction(tr("Open Link Inspector"), this);
  connect(this->inspectAct, SIGNAL(triggered()), this,
      SLOT(OnOpenInspector()));

  connect(g_deleteAct, SIGNAL(DeleteSignal(const std::string &)), this,
          SLOT(OnDelete(const std::string &)));

  this->connections.push_back(
      gui::Events::ConnectEditModel(
      boost::bind(&ModelCreator::OnEditModel, this, _1)));

  this->connections.push_back(
      gui::model::Events::ConnectSaveModelEditor(
      boost::bind(&ModelCreator::OnSave, this)));

  this->connections.push_back(
      gui::model::Events::ConnectSaveAsModelEditor(
      boost::bind(&ModelCreator::OnSaveAs, this)));

  this->connections.push_back(
      gui::model::Events::ConnectNewModelEditor(
      boost::bind(&ModelCreator::OnNew, this)));

  this->connections.push_back(
      gui::model::Events::ConnectExitModelEditor(
      boost::bind(&ModelCreator::OnExit, this)));

  this->connections.push_back(
    gui::model::Events::ConnectModelNameChanged(
      boost::bind(&ModelCreator::OnNameChanged, this, _1)));

  this->connections.push_back(
      gui::model::Events::ConnectModelChanged(
      boost::bind(&ModelCreator::ModelChanged, this)));

  this->connections.push_back(
      gui::Events::ConnectAlignMode(
        boost::bind(&ModelCreator::OnAlignMode, this, _1, _2, _3, _4)));

  this->connections.push_back(
      gui::Events::ConnectManipMode(
        boost::bind(&ModelCreator::OnManipMode, this, _1)));

  this->connections.push_back(
     event::Events::ConnectSetSelectedEntity(
       boost::bind(&ModelCreator::OnSetSelectedEntity, this, _1, _2)));

  this->connections.push_back(
      gui::Events::ConnectScaleEntity(
      boost::bind(&ModelCreator::OnEntityScaleChanged, this, _1, _2)));

  this->connections.push_back(
      event::Events::ConnectPreRender(
        boost::bind(&ModelCreator::Update, this)));

  g_copyAct->setEnabled(false);
  g_pasteAct->setEnabled(false);

  connect(g_copyAct, SIGNAL(triggered()), this, SLOT(OnCopy()));
  connect(g_pasteAct, SIGNAL(triggered()), this, SLOT(OnPaste()));

  this->saveDialog = new SaveDialog(SaveDialog::MODEL);

  this->Reset();
}

/////////////////////////////////////////////////
ModelCreator::~ModelCreator()
{
  while (!this->allLinks.empty())
    this->RemoveLink(this->allLinks.begin()->first);

  this->allLinks.clear();
  this->node->Fini();
  this->node.reset();
  this->modelTemplateSDF.reset();
  this->requestPub.reset();
  this->makerPub.reset();
  this->connections.clear();

  delete this->saveDialog;
  delete this->updateMutex;

  delete jointMaker;
}

/////////////////////////////////////////////////
void ModelCreator::OnEdit(bool _checked)
{
  if (_checked)
  {
    this->active = true;
    this->modelCounter++;
    KeyEventHandler::Instance()->AddPressFilter("model_creator",
        boost::bind(&ModelCreator::OnKeyPress, this, _1));

    MouseEventHandler::Instance()->AddPressFilter("model_creator",
        boost::bind(&ModelCreator::OnMousePress, this, _1));

    MouseEventHandler::Instance()->AddReleaseFilter("model_creator",
        boost::bind(&ModelCreator::OnMouseRelease, this, _1));

    MouseEventHandler::Instance()->AddMoveFilter("model_creator",
        boost::bind(&ModelCreator::OnMouseMove, this, _1));

    MouseEventHandler::Instance()->AddDoubleClickFilter("model_creator",
        boost::bind(&ModelCreator::OnMouseDoubleClick, this, _1));

    this->jointMaker->EnableEventHandlers();
  }
  else
  {
    this->active = false;
    KeyEventHandler::Instance()->RemovePressFilter("model_creator");
    MouseEventHandler::Instance()->RemovePressFilter("model_creator");
    MouseEventHandler::Instance()->RemoveReleaseFilter("model_creator");
    MouseEventHandler::Instance()->RemoveMoveFilter("model_creator");
    MouseEventHandler::Instance()->RemoveDoubleClickFilter("model_creator");
    this->jointMaker->DisableEventHandlers();
    this->jointMaker->Stop();

    this->DeselectAll();
  }
}

/////////////////////////////////////////////////
void ModelCreator::OnEditModel(const std::string &_modelName)
{
  if (!gui::get_active_camera() ||
      !gui::get_active_camera()->GetScene())
  {
    gzerr << "Unable to edit model. GUI camera or scene is NULL"
        << std::endl;
    return;
  }

  if (!this->active)
  {
    gzwarn << "Model Editor must be active before loading a model. " <<
              "Not loading model " << _modelName << std::endl;
    return;
  }

  // Get SDF model element from model name
  // TODO replace with entity_info and parse gazebo.msgs.Model msgs
  // or handle model_sdf requests in world.
  boost::shared_ptr<msgs::Response> response =
    transport::request(gui::get_world(), "world_sdf");

  msgs::GzString msg;
  // Make sure the response is correct
  if (response->type() == msg.GetTypeName())
  {
    // Parse the response message
    msg.ParseFromString(response->serialized_data());

    // Parse the string into sdf
    sdf::SDF sdfParsed;
    sdfParsed.SetFromString(msg.data());

    // Check that sdf contains world
    if (sdfParsed.root->HasElement("world") &&
        sdfParsed.root->GetElement("world")->HasElement("model"))
    {
      sdf::ElementPtr world = sdfParsed.root->GetElement("world");
      sdf::ElementPtr model = world->GetElement("model");
      while (model)
      {
        if (model->GetAttribute("name")->GetAsString() == _modelName)
        {
          this->LoadSDF(model);

          // Hide the model from the scene to substitute with the preview visual
          this->SetModelVisible(_modelName, false);

          rendering::ScenePtr scene = gui::get_active_camera()->GetScene();
          rendering::VisualPtr visual = scene->GetVisual(_modelName);

          math::Pose pose;
          if (visual)
          {
            pose = visual->GetWorldPose();
            this->previewVisual->SetWorldPose(pose);
          }

          this->serverModelName = _modelName;
          this->serverModelSDF = model;
          this->modelPose = pose;

          return;
        }
        model = model->GetNextElement("model");
      }
      gzwarn << "Couldn't find SDF for " << _modelName << ". Not loading it."
          << std::endl;
    }
  }
  else
  {
    GZ_ASSERT(response->type() == msg.GetTypeName(),
        "Received incorrect response from 'world_sdf' request.");
  }
}

/////////////////////////////////////////////////
void ModelCreator::LoadSDF(sdf::ElementPtr _modelElem)
{
  // Reset preview visual in case there was something already loaded
  this->Reset();

  // Model general info
  // Keep previewModel with previewName to avoid conflicts
  if (_modelElem->HasElement("pose"))
    this->modelPose = _modelElem->Get<math::Pose>("pose");
  else
    this->modelPose = math::Pose::Zero;
  this->previewVisual->SetPose(this->modelPose);

  if (_modelElem->HasElement("static"))
    this->isStatic = _modelElem->Get<bool>("static");
  if (_modelElem->HasElement("allow_auto_disable"))
    this->autoDisable = _modelElem->Get<bool>("allow_auto_disable");
  gui::model::Events::modelPropertiesChanged(this->isStatic, this->autoDisable,
      this->modelPose);

  // Links
  if (!_modelElem->HasElement("link"))
  {
    gzerr << "Can't load a model without links." << std::endl;
    return;
  }
  sdf::ElementPtr linkElem = _modelElem->GetElement("link");
  while (linkElem)
  {
    this->CreateLinkFromSDF(linkElem);
    linkElem = linkElem->GetNextElement("link");
  }

  // Joints
  std::stringstream preivewModelName;
  preivewModelName << this->previewName << "_" << this->modelCounter;
  sdf::ElementPtr jointElem;
  if (_modelElem->HasElement("joint"))
     jointElem = _modelElem->GetElement("joint");

  while (jointElem)
  {
    this->jointMaker->CreateJointFromSDF(jointElem, preivewModelName.str());
    jointElem = jointElem->GetNextElement("joint");
  }
}

/////////////////////////////////////////////////
void ModelCreator::OnNew()
{
  this->Stop();

  if (this->allLinks.empty())
  {
    this->Reset();
    gui::model::Events::newModel();
    return;
  }
  QString msg;
  QMessageBox msgBox(QMessageBox::Warning, QString("New"), msg);
  QPushButton *cancelButton = msgBox.addButton("Cancel",
      QMessageBox::RejectRole);
  msgBox.setEscapeButton(cancelButton);
  QPushButton *saveButton = new QPushButton("Save");

  switch (this->currentSaveState)
  {
    case ALL_SAVED:
    {
      msg.append("Are you sure you want to close this model and open a new "
                 "canvas?\n\n");
      QPushButton *newButton =
          msgBox.addButton("New Canvas", QMessageBox::AcceptRole);
      msgBox.setDefaultButton(newButton);
      break;
    }
    case UNSAVED_CHANGES:
    case NEVER_SAVED:
    {
      msg.append("You have unsaved changes. Do you want to save this model "
                 "and open a new canvas?\n\n");
      msgBox.addButton("Don't Save", QMessageBox::DestructiveRole);
      msgBox.addButton(saveButton, QMessageBox::AcceptRole);
      msgBox.setDefaultButton(saveButton);
      break;
    }
    default:
      return;
  }

  msgBox.setText(msg);

  msgBox.exec();

  if (msgBox.clickedButton() != cancelButton)
  {
    if (msgBox.clickedButton() == saveButton)
    {
      if (!this->OnSave())
      {
        return;
      }
    }

    this->Reset();
    gui::model::Events::newModel();
  }
}

/////////////////////////////////////////////////
bool ModelCreator::OnSave()
{
  this->Stop();

  switch (this->currentSaveState)
  {
    case UNSAVED_CHANGES:
    {
      this->SaveModelFiles();
      gui::model::Events::saveModel(this->modelName);
      return true;
    }
    case NEVER_SAVED:
    {
      return this->OnSaveAs();
    }
    default:
      return false;
  }
}

/////////////////////////////////////////////////
bool ModelCreator::OnSaveAs()
{
  this->Stop();

  if (this->saveDialog->OnSaveAs())
  {
    // Prevent changing save location
    this->currentSaveState = ALL_SAVED;
    // Get name set by user
    this->SetModelName(this->saveDialog->GetModelName());
    // Update name on palette
    gui::model::Events::saveModel(this->modelName);
    // Generate and save files
    this->SaveModelFiles();
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
void ModelCreator::OnNameChanged(const std::string &_name)
{
  if (_name.compare(this->modelName) == 0)
    return;

  this->SetModelName(_name);
  this->ModelChanged();
}

/////////////////////////////////////////////////
void ModelCreator::OnExit()
{
  this->Stop();

  if (this->allLinks.empty())
  {
    if (!this->serverModelName.empty())
      this->SetModelVisible(this->serverModelName, true);
    this->Reset();
    gui::model::Events::newModel();
    gui::model::Events::finishModel();
    return;
  }

  switch (this->currentSaveState)
  {
    case ALL_SAVED:
    {
      QString msg("Are you ready to exit?\n\n");
      QMessageBox msgBox(QMessageBox::NoIcon, QString("Exit"), msg);

      QPushButton *cancelButton = msgBox.addButton("Cancel",
          QMessageBox::RejectRole);
      QPushButton *exitButton =
          msgBox.addButton("Exit", QMessageBox::AcceptRole);
      msgBox.setDefaultButton(exitButton);
      msgBox.setEscapeButton(cancelButton);

      msgBox.exec();
      if (msgBox.clickedButton() == cancelButton)
      {
        return;
      }
      this->FinishModel();
      break;
    }
    case UNSAVED_CHANGES:
    case NEVER_SAVED:
    {
      QString msg("Save Changes before exiting?\n\n");

      QMessageBox msgBox(QMessageBox::NoIcon, QString("Exit"), msg);
      QPushButton *cancelButton = msgBox.addButton("Cancel",
          QMessageBox::RejectRole);
      msgBox.addButton("Don't Save, Exit", QMessageBox::DestructiveRole);
      QPushButton *saveButton = msgBox.addButton("Save and Exit",
          QMessageBox::AcceptRole);
      msgBox.setDefaultButton(cancelButton);
      msgBox.setDefaultButton(saveButton);

      msgBox.exec();
      if (msgBox.clickedButton() == cancelButton)
        return;

      if (msgBox.clickedButton() == saveButton)
      {
        if (!this->OnSave())
        {
          return;
        }
      }
      break;
    }
    default:
      return;
  }

  // Create entity on main window up to the saved point
  if (this->currentSaveState != NEVER_SAVED)
    this->FinishModel();
  else
    this->SetModelVisible(this->serverModelName, true);

  this->Reset();

  gui::model::Events::newModel();
  gui::model::Events::finishModel();
}

/////////////////////////////////////////////////
void ModelCreator::SaveModelFiles()
{
  this->saveDialog->GenerateConfig();
  this->saveDialog->SaveToConfig();
  this->GenerateSDF();
  this->saveDialog->SaveToSDF(this->modelSDF);
  this->currentSaveState = ALL_SAVED;
}

/////////////////////////////////////////////////
std::string ModelCreator::CreateModel()
{
  this->Reset();
  return this->folderName;
}

/////////////////////////////////////////////////
void ModelCreator::AddJoint(const std::string &_type)
{
  this->Stop();
  if (this->jointMaker)
    this->jointMaker->AddJoint(_type);
}

/////////////////////////////////////////////////
std::string ModelCreator::AddShape(LinkType _type,
    const math::Vector3 &_size, const math::Pose &_pose,
    const std::string &_uri, unsigned int _samples)
{
  if (!this->previewVisual)
  {
    this->Reset();
  }

  std::stringstream linkNameStream;
  linkNameStream << this->previewName << "_" << this->modelCounter
      << "::link_" << this->linkCounter++;
  std::string linkName = linkNameStream.str();

  rendering::VisualPtr linkVisual(new rendering::Visual(linkName,
      this->previewVisual));
  linkVisual->Load();
  linkVisual->SetTransparency(ModelData::GetEditTransparency());

  std::ostringstream visualName;
  visualName << linkName << "::visual";
  rendering::VisualPtr visVisual(new rendering::Visual(visualName.str(),
      linkVisual));
  sdf::ElementPtr visualElem =  this->modelTemplateSDF->root
      ->GetElement("model")->GetElement("link")->GetElement("visual");

  sdf::ElementPtr geomElem =  visualElem->GetElement("geometry");
  geomElem->ClearElements();

  if (_type == LINK_CYLINDER)
  {
    sdf::ElementPtr cylinderElem = geomElem->AddElement("cylinder");
    (cylinderElem->GetElement("radius"))->Set(_size.x*0.5);
    (cylinderElem->GetElement("length"))->Set(_size.z);
  }
  else if (_type == LINK_SPHERE)
  {
    ((geomElem->AddElement("sphere"))->GetElement("radius"))->Set(_size.x*0.5);
  }
  else if (_type == LINK_MESH)
  {
    sdf::ElementPtr meshElem = geomElem->AddElement("mesh");
    meshElem->GetElement("scale")->Set(_size);
    meshElem->GetElement("uri")->Set(_uri);
  }
  else if (_type == LINK_POLYLINE)
  {
    QFileInfo info(QString::fromStdString(_uri));
    if (!info.isFile() || info.completeSuffix().toLower() != "svg")
    {
      gzerr << "File [" << _uri << "] not found or invalid!" << std::endl;
      return std::string();
    }

    common::SVGLoader svgLoader(_samples);
    std::vector<common::SVGPath> paths;
    svgLoader.Parse(_uri, paths);

    if (paths.empty())
    {
      gzerr << "No paths found on file [" << _uri << "]" << std::endl;
      return std::string();
    }

    // Find extreme values to center the polylines
    math::Vector2d min(paths[0].polylines[0][0]);
    math::Vector2d max(min);
    for (common::SVGPath p : paths)
    {
      for (std::vector<math::Vector2d> poly : p.polylines)
      {
        for (math::Vector2d pt : poly)
        {
          if (pt.x < min.x)
            min.x = pt.x;
          if (pt.y < min.y)
            min.y = pt.y;
          if (pt.x > max.x)
            max.x = pt.x;
          if (pt.y > max.y)
            max.y = pt.y;
        }
      }
    }

    for (common::SVGPath p : paths)
    {
      for (std::vector<math::Vector2d> poly : p.polylines)
      {
        sdf::ElementPtr polylineElem = geomElem->AddElement("polyline");
        polylineElem->GetElement("height")->Set(_size.z);

        for (math::Vector2d pt : poly)
        {
          // Translate to center
          pt = pt - min - (max-min)*0.5;
          // Swap X and Y so Z will point up
          // (in 2D it points into the screen)
          sdf::ElementPtr pointElem = polylineElem->AddElement("point");
          pointElem->Set(math::Vector2d(pt.y*_size.y, pt.x*_size.x));
        }
      }
    }
  }
  else
  {
    if (_type != LINK_BOX)
    {
      gzwarn << "Unknown link type '" << _type << "'. " <<
          "Adding a box" << std::endl;
    }
    ((geomElem->AddElement("box"))->GetElement("size"))->Set(_size);
  }

  visVisual->Load(visualElem);
  this->CreateLink(visVisual);

  linkVisual->SetPose(_pose);

  // insert over ground plane for now
  math::Vector3 linkPos = linkVisual->GetWorldPose().pos;
  if (_type == LINK_BOX || _type == LINK_CYLINDER || _type == LINK_SPHERE)
  {
    linkPos.z = _size.z * 0.5;
  }
  // override orientation as it's more natural to insert objects upright rather
  // than inserting it in the model frame.
  linkVisual->SetWorldPose(math::Pose(linkPos, math::Quaternion()));

  this->mouseVisual = linkVisual;

  return linkName;
}

/////////////////////////////////////////////////
void ModelCreator::CreateLink(const rendering::VisualPtr &_visual)
{
  LinkData *link = new LinkData();
  MainWindow *mainWindow = gui::get_main_window();
  if (mainWindow)
  {
    connect(gui::get_main_window(), SIGNAL(Close()), link->inspector,
        SLOT(close()));
  }

  link->linkVisual = _visual->GetParent();
  link->AddVisual(_visual);

  // override transparency
  _visual->SetTransparency(_visual->GetTransparency() *
      (1-ModelData::GetEditTransparency()-0.1)
      + ModelData::GetEditTransparency());

  // create collision with identical geometry
  rendering::VisualPtr collisionVis =
      _visual->Clone(link->linkVisual->GetName() + "::collision",
      link->linkVisual);

  // orange
  collisionVis->SetMaterial("Gazebo/Orange");
  collisionVis->SetTransparency(
      math::clamp(ModelData::GetEditTransparency() * 2.0, 0.0, 0.8));
  // fix for transparency alpha compositing
  Ogre::MovableObject *colObj = collisionVis->GetSceneNode()->
      getAttachedObject(0);
  colObj->setRenderQueueGroup(colObj->getRenderQueueGroup()+1);
  link->AddCollision(collisionVis);

  std::string linkName = link->linkVisual->GetName();

  std::string leafName = linkName;
  size_t idx = linkName.find_last_of("::");
  if (idx != std::string::npos)
    leafName = linkName.substr(idx+1);

  link->SetName(leafName);

  {
    boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
    this->allLinks[linkName] = link;
    if (this->canonicalLink.empty())
      this->canonicalLink = linkName;
  }

  rendering::ScenePtr scene = link->linkVisual->GetScene();
  scene->AddVisual(link->linkVisual);

  this->ModelChanged();
}

/////////////////////////////////////////////////
LinkData *ModelCreator::CloneLink(const std::string &_linkName)
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);

  auto it = this->allLinks.find(_linkName);
  if (it == allLinks.end())
  {
    gzerr << "No link with name: " << _linkName << " found."  << std::endl;
    return NULL;
  }

  // generate unique name.
  std::string newName = _linkName + "_clone";
  auto itName = this->allLinks.find(newName);
  int nameCounter = 0;
  while (itName != this->allLinks.end())
  {
    std::stringstream newLinkName;
    newLinkName << _linkName << "_clone_" << nameCounter++;
    newName = newLinkName.str();
    itName = this->allLinks.find(newName);
  }

  std::string leafName = newName;
  size_t idx = newName.find_last_of("::");
  if (idx != std::string::npos)
    leafName = newName.substr(idx+1);
  LinkData *link = it->second->Clone(leafName);

  this->allLinks[newName] = link;

  this->ModelChanged();

  return link;
}

/////////////////////////////////////////////////
void ModelCreator::CreateLinkFromSDF(sdf::ElementPtr _linkElem)
{
  LinkData *link = new LinkData();
  MainWindow *mainWindow = gui::get_main_window();
  if (mainWindow)
  {
    connect(gui::get_main_window(), SIGNAL(Close()), link->inspector,
        SLOT(close()));
  }

  link->Load(_linkElem);

  // Link
  std::stringstream linkNameStream;
  std::string leafName = link->GetName();

  linkNameStream << this->previewName << "_" << this->modelCounter << "::";
  linkNameStream << leafName;
  std::string linkName = linkNameStream.str();

  if (this->canonicalLink.empty())
    this->canonicalLink = linkName;

  link->SetName(leafName);

  // if link name is scoped, it could mean that it's from an included model.
  // The joint maker needs to know about this in order to specify the correct
  // parent and child links in sdf generation step.
  if (leafName.find("::") != std::string::npos)
    this->jointMaker->AddScopedLinkName(leafName);

  rendering::VisualPtr linkVisual(new rendering::Visual(linkName,
      this->previewVisual));
  linkVisual->Load();
  linkVisual->SetPose(link->GetPose());
  link->linkVisual = linkVisual;

  // Visuals
  int visualIndex = 0;
  sdf::ElementPtr visualElem;

  if (_linkElem->HasElement("visual"))
    visualElem = _linkElem->GetElement("visual");

  linkVisual->SetTransparency(ModelData::GetEditTransparency());

  while (visualElem)
  {
    // Visual name
    std::string visualName;
    if (visualElem->HasAttribute("name"))
    {
      visualName = linkName + "::" + visualElem->Get<std::string>("name");
      visualIndex++;
    }
    else
    {
      std::stringstream visualNameStream;
      visualNameStream << linkName << "::visual_" << visualIndex++;
      visualName = visualNameStream.str();
      gzwarn << "SDF missing visual name attribute. Created name " << visualName
          << std::endl;
    }
    rendering::VisualPtr visVisual(new rendering::Visual(visualName,
        linkVisual));
    visVisual->Load(visualElem);

    // Visual pose
    math::Pose visualPose;
    if (visualElem->HasElement("pose"))
      visualPose = visualElem->Get<math::Pose>("pose");
    else
      visualPose.Set(0, 0, 0, 0, 0, 0);
    visVisual->SetPose(visualPose);

    // Add to link
    link->AddVisual(visVisual);

    // override transparency
    visVisual->SetTransparency(visVisual->GetTransparency() *
        (1-ModelData::GetEditTransparency()-0.1)
        + ModelData::GetEditTransparency());

    visualElem = visualElem->GetNextElement("visual");
  }

  // Collisions
  int collisionIndex = 0;
  sdf::ElementPtr collisionElem;

  if (_linkElem->HasElement("collision"))
    collisionElem = _linkElem->GetElement("collision");

  while (collisionElem)
  {
    // Collision name
    std::string collisionName;
    if (collisionElem->HasAttribute("name"))
    {
      collisionName = linkName + "::" + collisionElem->Get<std::string>("name");
      collisionIndex++;
    }
    else
    {
      std::ostringstream collisionNameStream;
      collisionNameStream << linkName << "::collision_" << collisionIndex++;
      collisionName = collisionNameStream.str();
      gzwarn << "SDF missing collision name attribute. Created name " <<
          collisionName << std::endl;
    }
    rendering::VisualPtr colVisual(new rendering::Visual(collisionName,
        linkVisual));

    // Collision pose
    math::Pose collisionPose;
    if (collisionElem->HasElement("pose"))
      collisionPose = collisionElem->Get<math::Pose>("pose");
    else
      collisionPose.Set(0, 0, 0, 0, 0, 0);

    // Make a visual element from the collision element
    sdf::ElementPtr colVisualElem =  this->modelTemplateSDF->root
        ->GetElement("model")->GetElement("link")->GetElement("visual");

    sdf::ElementPtr geomElem = colVisualElem->GetElement("geometry");
    geomElem->ClearElements();
    geomElem->Copy(collisionElem->GetElement("geometry"));

    colVisual->Load(colVisualElem);
    colVisual->SetPose(collisionPose);
    colVisual->SetMaterial("Gazebo/Orange");
    colVisual->SetTransparency(
        math::clamp(ModelData::GetEditTransparency() * 2.0, 0.0, 0.8));
    // fix for transparency alpha compositing
    Ogre::MovableObject *colObj = colVisual->GetSceneNode()->
        getAttachedObject(0);
    colObj->setRenderQueueGroup(colObj->getRenderQueueGroup()+1);

    // Add to link
    link->AddCollision(colVisual);

    collisionElem = collisionElem->GetNextElement("collision");
  }

  {
    boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
    this->allLinks[linkName] = link;
  }

  rendering::ScenePtr scene = link->linkVisual->GetScene();
  scene->AddVisual(link->linkVisual);

  this->ModelChanged();
}

/////////////////////////////////////////////////
void ModelCreator::RemoveLink(const std::string &_linkName)
{
  if (!this->previewVisual)
  {
    this->Reset();
    return;
  }

  LinkData *link = NULL;
  {
    boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
    if (this->allLinks.find(_linkName) == this->allLinks.end())
      return;
    link = this->allLinks[_linkName];
  }

  if (!link)
    return;

  rendering::ScenePtr scene = link->linkVisual->GetScene();
  for (auto &it : link->visuals)
  {
    rendering::VisualPtr vis = it.first;
    scene->RemoveVisual(vis);
  }
  scene->RemoveVisual(link->linkVisual);
  for (auto &colIt : link->collisions)
  {
    rendering::VisualPtr vis = colIt.first;
    scene->RemoveVisual(vis);
  }

  scene->RemoveVisual(link->linkVisual);

  link->linkVisual.reset();
  {
    boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
    this->allLinks.erase(_linkName);
    delete link;
  }

  this->ModelChanged();
}

/////////////////////////////////////////////////
void ModelCreator::Reset()
{
  delete this->saveDialog;
  this->saveDialog = new SaveDialog(SaveDialog::MODEL);

  this->jointMaker->Reset();
  this->selectedVisuals.clear();
  g_copyAct->setEnabled(false);
  g_pasteAct->setEnabled(false);

  this->currentSaveState = NEVER_SAVED;
  this->SetModelName(this->modelDefaultName);
  this->serverModelName = "";
  this->serverModelSDF.reset();
  this->serverModelVisible.clear();
  this->canonicalLink = "";

  this->modelTemplateSDF.reset(new sdf::SDF);
  this->modelTemplateSDF->SetFromString(ModelData::GetTemplateSDFString());

  this->modelSDF.reset(new sdf::SDF);

  this->isStatic = false;
  this->autoDisable = true;
  gui::model::Events::modelPropertiesChanged(this->isStatic, this->autoDisable,
      this->modelPose);

  while (!this->allLinks.empty())
    this->RemoveLink(this->allLinks.begin()->first);
  this->allLinks.clear();

  if (!gui::get_active_camera() ||
    !gui::get_active_camera()->GetScene())
  return;

  rendering::ScenePtr scene = gui::get_active_camera()->GetScene();
  if (this->previewVisual)
    scene->RemoveVisual(this->previewVisual);

  this->previewVisual.reset(new rendering::Visual(this->previewName,
      scene->GetWorldVisual()));

  this->previewVisual->Load();
  this->modelPose = math::Pose::Zero;
  this->previewVisual->SetPose(this->modelPose);
  scene->AddVisual(this->previewVisual);
}

/////////////////////////////////////////////////
void ModelCreator::SetModelName(const std::string &_modelName)
{
  this->modelName = _modelName;
  this->saveDialog->SetModelName(_modelName);

  this->folderName = this->saveDialog->
      GetFolderNameFromModelName(this->modelName);

  if (this->currentSaveState == NEVER_SAVED)
  {
    // Set new saveLocation
    boost::filesystem::path oldPath(this->saveDialog->GetSaveLocation());

    boost::filesystem::path newPath = oldPath.parent_path() / this->folderName;
    this->saveDialog->SetSaveLocation(newPath.string());
  }
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
  this->ModelChanged();
}

/////////////////////////////////////////////////
void ModelCreator::SetAutoDisable(bool _auto)
{
  this->autoDisable = _auto;
  this->ModelChanged();
}

/////////////////////////////////////////////////
void ModelCreator::FinishModel()
{
  if (!this->serverModelName.empty())
  {
    // delete model on server first before spawning the updated one.
    transport::request(gui::get_world(), "entity_delete",
        this->serverModelName);
    int timeoutCounter = 0;
    int timeout = 100;
    while (timeoutCounter < timeout)
    {
      boost::shared_ptr<msgs::Response> response =
          transport::request(gui::get_world(), "entity_info",
          this->serverModelName);
      // Make sure the response is correct
      if (response->response() == "nonexistent")
        break;

      common::Time::MSleep(100);
      timeoutCounter++;
    }
  }
  event::Events::setSelectedEntity("", "normal");
  this->CreateTheEntity();
  this->Reset();
}

/////////////////////////////////////////////////
void ModelCreator::CreateTheEntity()
{
  if (!this->modelSDF->root->HasElement("model"))
  {
    gzerr << "Generated invalid SDF! Cannot create entity." << std::endl;
    return;
  }

  msgs::Factory msg;
  // Create a new name if the model exists
  sdf::ElementPtr modelElem = this->modelSDF->root->GetElement("model");
  std::string modelElemName = modelElem->Get<std::string>("name");
  if (has_entity_name(modelElemName))
  {
    int i = 0;
    while (has_entity_name(modelElemName))
    {
      modelElemName = modelElem->Get<std::string>("name") + "_" +
        boost::lexical_cast<std::string>(i++);
    }
    modelElem->GetAttribute("name")->Set(modelElemName);
  }

  msg.set_sdf(this->modelSDF->ToString());
  msgs::Set(msg.mutable_pose(), this->modelPose);
  this->makerPub->Publish(msg);
}

/////////////////////////////////////////////////
void ModelCreator::AddLink(LinkType _type)
{
  if (!this->previewVisual)
  {
    this->Reset();
  }

  this->Stop();

  this->addLinkType = _type;
  if (_type != LINK_NONE)
    this->AddShape(_type);
}

/////////////////////////////////////////////////
void ModelCreator::Stop()
{
  if (this->addLinkType != LINK_NONE && this->mouseVisual)
  {
    for (unsigned int i = 0; i < this->mouseVisual->GetChildCount(); ++i)
        this->RemoveLink(this->mouseVisual->GetName());
    this->mouseVisual.reset();
    emit LinkAdded();
  }
  if (this->jointMaker)
    this->jointMaker->Stop();
}

/////////////////////////////////////////////////
void ModelCreator::OnDelete(const std::string &_entity)
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);

  // if it's a link
  if (this->allLinks.find(_entity) != this->allLinks.end())
  {
    if (this->jointMaker)
      this->jointMaker->RemoveJointsByLink(_entity);
    this->RemoveLink(_entity);
    return;
  }

  // if it's a visual
  rendering::VisualPtr vis =
      gui::get_active_camera()->GetScene()->GetVisual(_entity);
  if (vis)
  {
    rendering::VisualPtr parentLink = vis->GetParent();
    std::string parentLinkName = parentLink->GetName();

    if (this->allLinks.find(parentLinkName) != this->allLinks.end())
    {
      // remove the parent link if it's the only child
      if (parentLink->GetChildCount() == 1)
      {
        if (this->jointMaker)
          this->jointMaker->RemoveJointsByLink(parentLink->GetName());
        this->RemoveLink(parentLink->GetName());
        return;
      }
    }
  }
}

/////////////////////////////////////////////////
bool ModelCreator::OnKeyPress(const common::KeyEvent &_event)
{
  if (_event.key == Qt::Key_Escape)
  {
    this->Stop();
  }
  else if (_event.key == Qt::Key_Delete)
  {
    if (!this->selectedVisuals.empty())
    {
      for (auto it = this->selectedVisuals.begin();
          it != this->selectedVisuals.end();)
      {
        (*it)->SetHighlighted(false);
        this->OnDelete((*it)->GetName());
        it = this->selectedVisuals.erase(it);
      }
    }
  }
  else if (_event.control)
  {
    if (_event.key == Qt::Key_C && _event.control)
    {
      g_copyAct->trigger();
      return true;
    }
    if (_event.key == Qt::Key_V && _event.control)
    {
      g_pasteAct->trigger();
      return true;
    }
  }
  return false;
}

/////////////////////////////////////////////////
bool ModelCreator::OnMousePress(const common::MouseEvent &_event)
{
  rendering::UserCameraPtr userCamera = gui::get_active_camera();
  if (!userCamera)
    return false;

  if (this->jointMaker->GetState() != JointMaker::JOINT_NONE)
  {
    userCamera->HandleMouseEvent(_event);
    return true;
  }

  rendering::VisualPtr vis = userCamera->GetVisual(_event.pos);
  if (vis)
  {
    if (!vis->IsPlane() && gui::get_entity_id(vis->GetRootVisual()->GetName()))
    {
      // Handle snap from GLWidget
      if (g_snapAct->isChecked())
        return false;

      // Prevent interaction with other models, send event only to
      // user camera
      userCamera->HandleMouseEvent(_event);
      return true;
    }
  }
  return false;
}

/////////////////////////////////////////////////
bool ModelCreator::OnMouseRelease(const common::MouseEvent &_event)
{
  rendering::UserCameraPtr userCamera = gui::get_active_camera();
  if (!userCamera)
    return false;

  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);

  if (this->mouseVisual)
  {
    if (_event.button == common::MouseEvent::RIGHT)
      return true;

    // set the link data pose
    if (this->allLinks.find(this->mouseVisual->GetName()) !=
        this->allLinks.end())
    {
      LinkData *link = this->allLinks[this->mouseVisual->GetName()];
      link->SetPose(this->mouseVisual->GetWorldPose()-this->modelPose);
    }

    // reset and return
    emit LinkAdded();
    this->mouseVisual.reset();
    this->AddLink(LINK_NONE);
    return true;
  }

  rendering::VisualPtr vis = userCamera->GetVisual(_event.pos);
  if (vis)
  {
    rendering::VisualPtr linkVis = vis->GetParent();
    // Is link
    if (this->allLinks.find(linkVis->GetName()) !=
        this->allLinks.end())
    {
      // Handle snap from GLWidget
      if (g_snapAct->isChecked())
        return false;

      // trigger link inspector on right click
      if (_event.button == common::MouseEvent::RIGHT)
      {
        this->inspectVis = vis->GetParent();

        QMenu menu;
        menu.addAction(this->inspectAct);

        std::vector<JointData *> joints = this->jointMaker->GetJointDataByLink(
            this->inspectVis->GetName());

        if (!joints.empty())
        {
          QMenu *jointsMenu = menu.addMenu(tr("Open Joint Inspector"));

          for (auto joint : joints)
          {
            QAction *jointAct = new QAction(tr(joint->name.c_str()), this);
            connect(jointAct, SIGNAL(triggered()), joint,
                SLOT(OnOpenInspector()));
            jointsMenu->addAction(jointAct);
          }
        }

        menu.exec(QCursor::pos());
        return true;
      }

      // Not in multi-selection mode.
      if (!(QApplication::keyboardModifiers() & Qt::ControlModifier))
      {
        this->DeselectAll();

        // Highlight and selected clicked link
        linkVis->SetHighlighted(true);
        this->selectedVisuals.push_back(linkVis);
      }
      // Multi-selection mode
      else
      {
        auto it = std::find(this->selectedVisuals.begin(),
            this->selectedVisuals.end(), linkVis);
        // Highlight and select clicked link if not already selected
        if (it == this->selectedVisuals.end())
        {
          linkVis->SetHighlighted(true);
          this->selectedVisuals.push_back(linkVis);
        }
        // Deselect if already selected
        else
        {
          linkVis->SetHighlighted(false);
          this->selectedVisuals.erase(it);
        }
      }
      g_copyAct->setEnabled(!this->selectedVisuals.empty());
      g_alignAct->setEnabled(this->selectedVisuals.size() > 1);

      if (this->manipMode == "translate" || this->manipMode == "rotate" ||
          this->manipMode == "scale")
      {
        this->OnManipMode(this->manipMode);
      }

      return true;
    }
    // Not link
    else
    {
      this->DeselectAll();

      g_alignAct->setEnabled(false);
      g_copyAct->setEnabled(!this->selectedVisuals.empty());

      if (!vis->IsPlane())
        return true;
    }
  }
  return false;
}

/////////////////////////////////////////////////
bool ModelCreator::OnMouseMove(const common::MouseEvent &_event)
{
  this->lastMouseEvent = _event;
  rendering::UserCameraPtr userCamera = gui::get_active_camera();
  if (!userCamera)
    return false;

  if (!this->mouseVisual)
  {
    rendering::VisualPtr vis = userCamera->GetVisual(_event.pos);
    if (vis && !vis->IsPlane())
    {
      // Main window models always handled here
      if (this->allLinks.find(vis->GetParent()->GetName()) ==
          this->allLinks.end())
      {
        // Prevent highlighting for snapping
        if (this->manipMode == "snap" || this->manipMode == "select" ||
            this->manipMode == "")
        {
          // Don't change cursor on hover
          QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
          userCamera->HandleMouseEvent(_event);
        }
        // Allow ModelManipulator to work while dragging handle over this
        else if (_event.dragging)
        {
          ModelManipulator::Instance()->OnMouseMoveEvent(_event);
        }
        return true;
      }
    }
    return false;
  }

  math::Pose pose = this->mouseVisual->GetWorldPose();
  pose.pos = ModelManipulator::GetMousePositionOnPlane(
      userCamera, _event);

  if (!_event.shift)
  {
    pose.pos = ModelManipulator::SnapPoint(pose.pos);
  }
  pose.pos.z = this->mouseVisual->GetWorldPose().pos.z;

  this->mouseVisual->SetWorldPose(pose);

  return true;
}

/////////////////////////////////////////////////
bool ModelCreator::OnMouseDoubleClick(const common::MouseEvent &_event)
{
  // open the link inspector on double click
  rendering::VisualPtr vis = gui::get_active_camera()->GetVisual(_event.pos);
  if (!vis)
    return false;

  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);

  if (this->allLinks.find(vis->GetParent()->GetName()) !=
      this->allLinks.end())
  {
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
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
  LinkData *link = this->allLinks[_name];
  link->SetPose(link->linkVisual->GetWorldPose()-this->modelPose);
  link->UpdateConfig();
  link->inspector->move(QCursor::pos());
  link->inspector->show();
}

/////////////////////////////////////////////////
void ModelCreator::OnCopy()
{
  if (!g_editModelAct->isChecked())
    return;

  if (!this->selectedVisuals.empty())
  {
    this->copiedLinkNames.clear();
    for (auto vis : this->selectedVisuals)
    {
      this->copiedLinkNames.push_back(vis->GetName());
    }
    g_pasteAct->setEnabled(true);
  }
}

/////////////////////////////////////////////////
void ModelCreator::OnPaste()
{
  if (this->copiedLinkNames.empty() || !g_editModelAct->isChecked())
  {
    return;
  }

  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);

  // For now, only copy the last selected model
  auto it = this->allLinks.find(this->copiedLinkNames.back());
  if (it != this->allLinks.end())
  {
    LinkData *copiedLink = it->second;
    if (!copiedLink)
      return;

    this->Stop();
    this->DeselectAll();

    if (!this->previewVisual)
    {
      this->Reset();
    }

    LinkData* clonedLink = this->CloneLink(it->first);

    math::Pose clonePose = copiedLink->linkVisual->GetWorldPose();
    rendering::UserCameraPtr userCamera = gui::get_active_camera();
    if (userCamera)
    {
      math::Vector3 mousePosition =
        ModelManipulator::GetMousePositionOnPlane(userCamera,
                                                  this->lastMouseEvent);
      clonePose.pos.x = mousePosition.x;
      clonePose.pos.y = mousePosition.y;
    }

    clonedLink->linkVisual->SetWorldPose(clonePose);
    this->addLinkType = LINK_MESH;
    this->mouseVisual = clonedLink->linkVisual;
  }
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

  this->modelSDF.reset(new sdf::SDF);
  this->modelSDF->SetFromString(ModelData::GetTemplateSDFString());

  modelElem = this->modelSDF->root->GetElement("model");

  modelElem->ClearElements();
  modelElem->GetAttribute("name")->Set(this->folderName);

  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);

  if (this->serverModelName.empty())
  {
    // set center of all links to be origin
    // TODO set a better origin other than the centroid
    math::Vector3 mid;
    for (auto &linksIt : this->allLinks)
    {
      LinkData *link = linksIt.second;
      mid += link->GetPose().pos;
    }
    if (!this->allLinks.empty())
      mid /= this->allLinks.size();
    this->modelPose.pos = mid;
  }

  // Update preview model and link poses in case they changed
  for (auto &linksIt : this->allLinks)
  {
    this->previewVisual->SetWorldPose(this->modelPose);
    LinkData *link = linksIt.second;
    link->SetPose(link->linkVisual->GetWorldPose() - this->modelPose);
    link->linkVisual->SetPose(link->GetPose());
  }

  // generate canonical link sdf first.
  if (!this->canonicalLink.empty())
  {
    auto canonical = this->allLinks.find(this->canonicalLink);
    if (canonical != this->allLinks.end())
    {
      LinkData *link = canonical->second;
      link->UpdateConfig();

      sdf::ElementPtr newLinkElem = this->GenerateLinkSDF(link);
      modelElem->InsertElement(newLinkElem);
    }
  }

  // loop through rest of all links and generate sdf
  for (auto &linksIt : this->allLinks)
  {
    if (linksIt.first == this->canonicalLink)
      continue;

    LinkData *link = linksIt.second;
    link->UpdateConfig();

    sdf::ElementPtr newLinkElem = this->GenerateLinkSDF(link);
    modelElem->InsertElement(newLinkElem);
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

  // If we're editing an existing model, copy the original plugin sdf elements
  // since we are not generating them.
  if (this->serverModelSDF)
  {
    if (this->serverModelSDF->HasElement("plugin"))
    {
      sdf::ElementPtr pluginElem = this->serverModelSDF->GetElement("plugin");
      while (pluginElem)
      {
        modelElem->InsertElement(pluginElem->Clone());
        pluginElem = pluginElem->GetNextElement("plugin");
      }
    }
  }
}

/////////////////////////////////////////////////
sdf::ElementPtr ModelCreator::GenerateLinkSDF(LinkData *_link)
{
  std::stringstream visualNameStream;
  std::stringstream collisionNameStream;
  visualNameStream.str("");
  collisionNameStream.str("");

  sdf::ElementPtr newLinkElem = _link->linkSDF->Clone();
  newLinkElem->GetElement("pose")->Set(_link->linkVisual->GetWorldPose()
      - this->modelPose);

  // visuals
  for (auto const &it : _link->visuals)
  {
    rendering::VisualPtr visual = it.first;
    msgs::Visual visualMsg = it.second;
    sdf::ElementPtr visualElem = visual->GetSDF()->Clone();

    visualElem->GetElement("transparency")->Set<double>(
        visualMsg.transparency());
    newLinkElem->InsertElement(visualElem);
  }

  // collisions
  for (auto const &colIt : _link->collisions)
  {
    sdf::ElementPtr collisionElem = msgs::CollisionToSDF(colIt.second);
    newLinkElem->InsertElement(collisionElem);
  }
  return newLinkElem;
}

/////////////////////////////////////////////////
void ModelCreator::OnAlignMode(const std::string &_axis,
    const std::string &_config, const std::string &_target, bool _preview)
{
  ModelAlign::Instance()->AlignVisuals(this->selectedVisuals, _axis, _config,
      _target, !_preview);
}

/////////////////////////////////////////////////
void ModelCreator::DeselectAll()
{
  if (!this->selectedVisuals.empty())
  {
    for (auto &vis : this->selectedVisuals)
    {
      vis->SetHighlighted(false);
    }
    this->selectedVisuals.clear();
  }
}

/////////////////////////////////////////////////
void ModelCreator::OnManipMode(const std::string &_mode)
{
  if (!this->active)
    return;

  this->manipMode = _mode;

  if (!this->selectedVisuals.empty())
  {
    ModelManipulator::Instance()->SetAttachedVisual(
        this->selectedVisuals.back());
  }

  ModelManipulator::Instance()->SetManipulationMode(_mode);
  ModelSnap::Instance()->Reset();

  // deselect 0 to n-1 models.
  if (this->selectedVisuals.size() > 1)
  {
    for (auto it = this->selectedVisuals.begin();
        it != --this->selectedVisuals.end();)
    {
       (*it)->SetHighlighted(false);
       it = this->selectedVisuals.erase(it);
    }
  }
}

/////////////////////////////////////////////////
void ModelCreator::OnSetSelectedEntity(const std::string &/*_name*/,
    const std::string &/*_mode*/)
{
  this->DeselectAll();
}

/////////////////////////////////////////////////
void ModelCreator::ModelChanged()
{
  if (this->currentSaveState != NEVER_SAVED)
    this->currentSaveState = UNSAVED_CHANGES;
}

/////////////////////////////////////////////////
void ModelCreator::Update()
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);

  // Check if any links have been moved or resized and trigger ModelChanged
  for (auto &linksIt : this->allLinks)
  {
    LinkData *link = linksIt.second;
    if (link->GetPose() != link->linkVisual->GetPose())
    {
      link->SetPose(link->linkVisual->GetWorldPose() - this->modelPose);
      this->ModelChanged();
    }
    for (auto &scaleIt : this->linkScaleUpdate)
    {
      if (link->linkVisual->GetName() == scaleIt.first)
        link->SetScale(scaleIt.second);
    }
    if (!this->linkScaleUpdate.empty())
      this->ModelChanged();
    this->linkScaleUpdate.clear();
  }
}

/////////////////////////////////////////////////
void ModelCreator::OnEntityScaleChanged(const std::string &_name,
  const math::Vector3 &_scale)
{
  boost::recursive_mutex::scoped_lock lock(*this->updateMutex);
  for (auto linksIt : this->allLinks)
  {
    if (_name == linksIt.first ||
        _name.find(linksIt.first) != std::string::npos)
    {
      this->linkScaleUpdate[linksIt.first] = _scale;
      break;
    }
  }
}

/////////////////////////////////////////////////
void ModelCreator::SetModelVisible(const std::string &_name, bool _visible)
{
  rendering::ScenePtr scene = gui::get_active_camera()->GetScene();
  rendering::VisualPtr visual = scene->GetVisual(_name);
  if (!visual)
    return;

  this->SetModelVisible(visual, _visible);

  if (_visible)
    visual->SetHighlighted(false);
}

/////////////////////////////////////////////////
void ModelCreator::SetModelVisible(rendering::VisualPtr _visual, bool _visible)
{
  if (!_visual)
    return;

  for (unsigned int i = 0; i < _visual->GetChildCount(); ++i)
    this->SetModelVisible(_visual->GetChild(i), _visible);

  if (!_visible)
  {
    // store original visibility
    this->serverModelVisible[_visual->GetId()] = _visual->GetVisible();
    _visual->SetVisible(_visible);
  }
  else
  {
    // restore original visibility
    auto it = this->serverModelVisible.find(_visual->GetId());
    if (it != this->serverModelVisible.end())
    {
      _visual->SetVisible(it->second, false);
    }
  }
}
/////////////////////////////////////////////////
ModelCreator::SaveState ModelCreator::GetCurrentSaveState() const
{
  return this->currentSaveState;
}
