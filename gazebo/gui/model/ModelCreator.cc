/*
 * Copyright (C) 2014-2016 Open Source Robotics Foundation
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
#include <boost/filesystem.hpp>
#include <sstream>
#include <functional>
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
#include "gazebo/gui/model/ModelPluginInspector.hh"
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

  if (g_deleteAct)
  {
    connect(g_deleteAct, SIGNAL(DeleteSignal(const std::string &)), this,
        SLOT(OnDelete(const std::string &)));
  }

  this->connections.push_back(
      gui::Events::ConnectEditModel(
      std::bind(&ModelCreator::OnEditModel, this, std::placeholders::_1)));

  this->connections.push_back(
      gui::model::Events::ConnectSaveModelEditor(
      std::bind(&ModelCreator::OnSave, this)));

  this->connections.push_back(
      gui::model::Events::ConnectSaveAsModelEditor(
      std::bind(&ModelCreator::OnSaveAs, this)));

  this->connections.push_back(
      gui::model::Events::ConnectNewModelEditor(
      std::bind(&ModelCreator::OnNew, this)));

  this->connections.push_back(
      gui::model::Events::ConnectExitModelEditor(
      std::bind(&ModelCreator::OnExit, this)));

  this->connections.push_back(
    gui::model::Events::ConnectModelNameChanged(
      std::bind(&ModelCreator::OnNameChanged, this, std::placeholders::_1)));

  this->connections.push_back(
      gui::model::Events::ConnectModelChanged(
      std::bind(&ModelCreator::ModelChanged, this)));

  this->connections.push_back(
      gui::model::Events::ConnectOpenLinkInspector(
      std::bind(&ModelCreator::OpenInspector, this, std::placeholders::_1)));

  this->connections.push_back(
      gui::model::Events::ConnectOpenModelPluginInspector(
      std::bind(&ModelCreator::OpenModelPluginInspector, this,
        std::placeholders::_1)));

  this->connections.push_back(
      gui::Events::ConnectAlignMode(
        std::bind(&ModelCreator::OnAlignMode, this, std::placeholders::_1,
          std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
          std::placeholders::_5)));

  this->connections.push_back(
      gui::Events::ConnectManipMode(
        std::bind(&ModelCreator::OnManipMode, this, std::placeholders::_1)));

  this->connections.push_back(
     event::Events::ConnectSetSelectedEntity(
       std::bind(&ModelCreator::OnSetSelectedEntity, this,
         std::placeholders::_1, std::placeholders::_2)));

  this->connections.push_back(
     gui::model::Events::ConnectSetSelectedLink(
       std::bind(&ModelCreator::OnSetSelectedLink, this,
         std::placeholders::_1, std::placeholders::_2)));

  this->connections.push_back(
     gui::model::Events::ConnectSetSelectedModelPlugin(
       std::bind(&ModelCreator::OnSetSelectedModelPlugin, this,
         std::placeholders::_1, std::placeholders::_2)));

  this->connections.push_back(
      gui::Events::ConnectScaleEntity(
      std::bind(&ModelCreator::OnEntityScaleChanged, this,
        std::placeholders::_1, std::placeholders::_2)));

  this->connections.push_back(
      gui::model::Events::ConnectShowLinkContextMenu(
      std::bind(&ModelCreator::ShowContextMenu, this, std::placeholders::_1)));

  this->connections.push_back(
      gui::model::Events::ConnectShowModelPluginContextMenu(
      std::bind(&ModelCreator::ShowModelPluginContextMenu, this,
        std::placeholders::_1)));

  this->connections.push_back(
      gui::model::Events::ConnectRequestLinkRemoval(
        std::bind(&ModelCreator::RemoveEntity, this, std::placeholders::_1)));

  this->connections.push_back(
      gui::model::Events::ConnectRequestModelPluginRemoval(
        std::bind(&ModelCreator::RemoveModelPlugin, this,
          std::placeholders::_1)));

  this->connections.push_back(
      event::Events::ConnectPreRender(
        std::bind(&ModelCreator::Update, this)));

  this->connections.push_back(
      gui::model::Events::ConnectModelPropertiesChanged(
      std::bind(&ModelCreator::OnPropertiesChanged, this,
        std::placeholders::_1, std::placeholders::_2)));

  this->connections.push_back(
      gui::model::Events::ConnectRequestModelPluginInsertion(
      std::bind(&ModelCreator::OnAddModelPlugin, this,
      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)));

  if (g_copyAct)
  {
    g_copyAct->setEnabled(false);
    connect(g_copyAct, SIGNAL(triggered()), this, SLOT(OnCopy()));
  }
  if (g_pasteAct)
  {
    g_pasteAct->setEnabled(false);
    connect(g_pasteAct, SIGNAL(triggered()), this, SLOT(OnPaste()));
  }

  this->saveDialog = new SaveDialog(SaveDialog::MODEL);

  this->Reset();
}

/////////////////////////////////////////////////
ModelCreator::~ModelCreator()
{
  while (!this->allLinks.empty())
    this->RemoveLinkImpl(this->allLinks.begin()->first);

  while (!this->allNestedModels.empty())
    this->RemoveNestedModelImpl(this->allNestedModels.begin()->first);

  this->allNestedModels.clear();
  this->allLinks.clear();
  this->allModelPlugins.clear();
  this->node->Fini();
  this->node.reset();
  this->modelTemplateSDF.reset();
  this->requestPub.reset();
  this->makerPub.reset();
  this->connections.clear();

  delete this->saveDialog;
  delete this->jointMaker;
}

/////////////////////////////////////////////////
void ModelCreator::OnEdit(bool _checked)
{
  if (_checked)
  {
    this->active = true;
    KeyEventHandler::Instance()->AddPressFilter("model_creator",
        std::bind(&ModelCreator::OnKeyPress, this, std::placeholders::_1));

    MouseEventHandler::Instance()->AddPressFilter("model_creator",
        std::bind(&ModelCreator::OnMousePress, this, std::placeholders::_1));

    MouseEventHandler::Instance()->AddReleaseFilter("model_creator",
        std::bind(&ModelCreator::OnMouseRelease, this, std::placeholders::_1));

    MouseEventHandler::Instance()->AddMoveFilter("model_creator",
        std::bind(&ModelCreator::OnMouseMove, this, std::placeholders::_1));

    MouseEventHandler::Instance()->AddDoubleClickFilter("model_creator",
        std::bind(&ModelCreator::OnMouseDoubleClick, this,
          std::placeholders::_1));

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
    if (sdfParsed.Root()->HasElement("world") &&
        sdfParsed.Root()->GetElement("world")->HasElement("model"))
    {
      sdf::ElementPtr world = sdfParsed.Root()->GetElement("world");
      sdf::ElementPtr model = world->GetElement("model");
      while (model)
      {
        if (model->GetAttribute("name")->GetAsString() == _modelName)
        {
          // Create the root model
          this->CreateModelFromSDF(model);

          // Hide the model from the scene to substitute with the preview visual
          this->SetModelVisible(_modelName, false);

          rendering::ScenePtr scene = gui::get_active_camera()->GetScene();
          rendering::VisualPtr visual = scene->GetVisual(_modelName);

          ignition::math::Pose3d pose;
          if (visual)
          {
            pose = visual->GetWorldPose().Ign();
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
NestedModelData *ModelCreator::CreateModelFromSDF(
    const sdf::ElementPtr &_modelElem, const rendering::VisualPtr &_parentVis,
    const bool _emit)
{
  rendering::VisualPtr modelVisual;
  std::stringstream modelNameStream;
  std::string nestedModelName;
  NestedModelData *modelData = new NestedModelData();

  // If no parent vis, this is the root model
  if (!_parentVis)
  {
    // Reset preview visual in case there was something already loaded
    this->Reset();

    // Keep previewModel with previewName to avoid conflicts
    modelVisual = this->previewVisual;
    modelNameStream << modelVisual->GetName();

    // Model general info
    if (_modelElem->HasAttribute("name"))
      this->SetModelName(_modelElem->Get<std::string>("name"));

    if (_modelElem->HasElement("pose"))
      this->modelPose = _modelElem->Get<ignition::math::Pose3d>("pose");
    else
      this->modelPose = ignition::math::Pose3d::Zero;
    this->previewVisual->SetPose(this->modelPose);

    if (_modelElem->HasElement("static"))
      this->isStatic = _modelElem->Get<bool>("static");
    if (_modelElem->HasElement("allow_auto_disable"))
      this->autoDisable = _modelElem->Get<bool>("allow_auto_disable");
    gui::model::Events::modelPropertiesChanged(this->isStatic,
        this->autoDisable);
    gui::model::Events::modelNameChanged(this->GetModelName());

    modelData->modelVisual = modelVisual;
  }
  // Nested models are attached to a parent visual
  else
  {
    // Internal name
    std::stringstream parentNameStream;
    parentNameStream << _parentVis->GetName();

    modelNameStream << parentNameStream.str() << "::" <<
        _modelElem->Get<std::string>("name");
    nestedModelName = modelNameStream.str();

    // Generate unique name
    auto itName = this->allNestedModels.find(nestedModelName);
    int nameCounter = 0;
    std::string uniqueName;
    while (itName != this->allNestedModels.end())
    {
      std::stringstream uniqueNameStr;
      uniqueNameStr << nestedModelName << "_" << nameCounter++;
      uniqueName = uniqueNameStr.str();
      itName = this->allNestedModels.find(uniqueName);
    }
    if (!uniqueName.empty())
      nestedModelName = uniqueName;

    // Model Visual
    modelVisual.reset(
        new rendering::Visual(nestedModelName, _parentVis, false));
    modelVisual->Load();
    modelVisual->SetTransparency(ModelData::GetEditTransparency());

    if (_modelElem->HasElement("pose"))
      modelVisual->SetPose(_modelElem->Get<ignition::math::Pose3d>("pose"));

    // Only keep SDF and preview visual
    std::string leafName = nestedModelName;
    leafName = leafName.substr(leafName.rfind("::")+2);

    modelData->modelSDF = _modelElem;
    modelData->modelVisual = modelVisual;
    modelData->SetName(leafName);
    modelData->SetPose(_modelElem->Get<ignition::math::Pose3d>("pose"));
  }

  // Notify nested model insertion
  if (_parentVis)
  {
    std::lock_guard<std::recursive_mutex> lock(this->updateMutex);
    this->allNestedModels[nestedModelName] = modelData;

    // fire nested inserted events only when the nested model is
    //  not attached to the mouse
    if (_emit)
      gui::model::Events::nestedModelInserted(nestedModelName);
  }

  // Recursively load models nested in this model
  // This must be done after other widgets were notified about the current
  // model but before making joints
  sdf::ElementPtr nestedModelElem;
  if (_modelElem->HasElement("model"))
     nestedModelElem = _modelElem->GetElement("model");
  while (nestedModelElem)
  {
    if (this->canonicalModel.empty())
      this->canonicalModel = nestedModelName;

    NestedModelData *nestedModelData =
        this->CreateModelFromSDF(nestedModelElem, modelVisual, _emit);
    rendering::VisualPtr nestedModelVis = nestedModelData->modelVisual;
    modelData->models[nestedModelVis->GetName()] = nestedModelVis;
    nestedModelElem = nestedModelElem->GetNextElement("model");
  }

  // Links
  sdf::ElementPtr linkElem;
  if (_modelElem->HasElement("link"))
    linkElem = _modelElem->GetElement("link");
  while (linkElem)
  {
    LinkData *linkData = this->CreateLinkFromSDF(linkElem, modelVisual);

    // if its parent is not the preview visual then the link has to be nested
    if (modelVisual != this->previewVisual)
      linkData->nested = true;
    rendering::VisualPtr linkVis = linkData->linkVisual;

    modelData->links[linkVis->GetName()] = linkVis;
    linkElem = linkElem->GetNextElement("link");
  }

  // Don't load joints or plugins for nested models
  if (!_parentVis)
  {
    // Joints
    sdf::ElementPtr jointElem;
    if (_modelElem->HasElement("joint"))
       jointElem = _modelElem->GetElement("joint");

    while (jointElem)
    {
      this->jointMaker->CreateJointFromSDF(jointElem, modelNameStream.str());
      jointElem = jointElem->GetNextElement("joint");
    }

    // Plugins
    sdf::ElementPtr pluginElem;
    if (_modelElem->HasElement("plugin"))
      pluginElem = _modelElem->GetElement("plugin");
    while (pluginElem)
    {
      this->AddModelPlugin(pluginElem);
      pluginElem = pluginElem->GetNextElement("plugin");
    }
  }

  return modelData;
}

/////////////////////////////////////////////////
void ModelCreator::OnNew()
{
  this->Stop();

  if (this->allLinks.empty() && this->allNestedModels.empty() &&
      this->allModelPlugins.empty())
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

  if (this->allLinks.empty() && this->allNestedModels.empty() &&
      this->allModelPlugins.empty())
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
void ModelCreator::OnPropertiesChanged(const bool _static,
    const bool _autoDisable)
{
  this->autoDisable = _autoDisable;
  this->isStatic = _static;
  this->ModelChanged();
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
LinkData *ModelCreator::AddShape(EntityType _type,
    const math::Vector3 &_size, const math::Pose &_pose,
    const std::string &_uri, unsigned int _samples)
{
  if (!this->previewVisual)
  {
    this->Reset();
  }

  std::stringstream linkNameStream;
  linkNameStream << this->previewVisual->GetName() << "::link_" <<
      this->linkCounter++;
  std::string linkName = linkNameStream.str();

  rendering::VisualPtr linkVisual(new rendering::Visual(linkName,
      this->previewVisual, false));
  linkVisual->Load();
  linkVisual->SetTransparency(ModelData::GetEditTransparency());

  std::ostringstream visualName;
  visualName << linkName << "::visual";
  rendering::VisualPtr visVisual(new rendering::Visual(visualName.str(),
      linkVisual, false));
  sdf::ElementPtr visualElem =  this->modelTemplateSDF->Root()
      ->GetElement("model")->GetElement("link")->GetElement("visual");

  sdf::ElementPtr geomElem =  visualElem->GetElement("geometry");
  geomElem->ClearElements();

  if (_type == ENTITY_CYLINDER)
  {
    sdf::ElementPtr cylinderElem = geomElem->AddElement("cylinder");
    (cylinderElem->GetElement("radius"))->Set(_size.x*0.5);
    (cylinderElem->GetElement("length"))->Set(_size.z);
  }
  else if (_type == ENTITY_SPHERE)
  {
    ((geomElem->AddElement("sphere"))->GetElement("radius"))->Set(_size.x*0.5);
  }
  else if (_type == ENTITY_MESH)
  {
    sdf::ElementPtr meshElem = geomElem->AddElement("mesh");
    meshElem->GetElement("scale")->Set(_size);
    meshElem->GetElement("uri")->Set(_uri);
  }
  else if (_type == ENTITY_POLYLINE)
  {
    QFileInfo info(QString::fromStdString(_uri));
    if (!info.isFile() || info.completeSuffix().toLower() != "svg")
    {
      gzerr << "File [" << _uri << "] not found or invalid!" << std::endl;
      return NULL;
    }

    common::SVGLoader svgLoader(_samples);
    std::vector<common::SVGPath> paths;
    svgLoader.Parse(_uri, paths);

    if (paths.empty())
    {
      gzerr << "No paths found on file [" << _uri << "]" << std::endl;
      return NULL;
    }

    // SVG paths do not map to sdf polylines, because we now allow a contour
    // to be made of multiple svg disjoint paths.
    // For this reason, we compute the closed polylines that can be extruded
    // in this step
    std::vector< std::vector<ignition::math::Vector2d> > closedPolys;
    std::vector< std::vector<ignition::math::Vector2d> > openPolys;
    svgLoader.PathsToClosedPolylines(paths, 0.05, closedPolys, openPolys);
    if (closedPolys.empty())
    {
      gzerr << "No closed polylines found on file [" << _uri << "]"
        << std::endl;
      return NULL;
    }
    if (!openPolys.empty())
    {
      gzmsg << "There are " << openPolys.size() << "open polylines. "
        << "They will be ignored." << std::endl;
    }
    // Find extreme values to center the polylines
    ignition::math::Vector2d min(paths[0].polylines[0][0]);
    ignition::math::Vector2d max(min);

    for (auto const &poly : closedPolys)
    {
      for (auto const &pt : poly)
      {
        if (pt.X() < min.X())
          min.X() = pt.X();
        if (pt.Y() < min.Y())
          min.Y() = pt.Y();
        if (pt.X() > max.X())
          max.X() = pt.X();
        if (pt.Y() > max.Y())
          max.Y() = pt.Y();
      }
    }
    for (auto const &poly : closedPolys)
    {
      sdf::ElementPtr polylineElem = geomElem->AddElement("polyline");
      polylineElem->GetElement("height")->Set(_size.z);

      for (auto const &p : poly)
      {
        // Translate to center
        ignition::math::Vector2d pt = p - min - (max-min)*0.5;
        // Swap X and Y so Z will point up
        // (in 2D it points into the screen)
        sdf::ElementPtr pointElem = polylineElem->AddElement("point");
        pointElem->Set(
            ignition::math::Vector2d(pt.Y()*_size.y, pt.X()*_size.x));
      }
    }
  }
  else
  {
    if (_type != ENTITY_BOX)
    {
      gzwarn << "Unknown link type '" << _type << "'. " <<
          "Adding a box" << std::endl;
    }

    ((geomElem->AddElement("box"))->GetElement("size"))->Set(_size);
  }

  visVisual->Load(visualElem);
  LinkData *linkData = this->CreateLink(visVisual);
  linkVisual->SetVisibilityFlags(GZ_VISIBILITY_GUI | GZ_VISIBILITY_SELECTABLE);

  linkVisual->SetPose(_pose);

  // insert over ground plane for now
  math::Vector3 linkPos = linkVisual->GetWorldPose().pos;
  if (_type == ENTITY_BOX || _type == ENTITY_CYLINDER || _type == ENTITY_SPHERE)
  {
    linkPos.z = _size.z * 0.5;
  }
  // override orientation as it's more natural to insert objects upright rather
  // than inserting it in the model frame.
  linkVisual->SetWorldPose(math::Pose(linkPos, math::Quaternion()));

  return linkData;
}

/////////////////////////////////////////////////
NestedModelData *ModelCreator::AddModel(const sdf::ElementPtr &_sdf)
{
  // Create a top-level nested model
  return this->CreateModelFromSDF(_sdf, this->previewVisual, false);
}

/////////////////////////////////////////////////
LinkData *ModelCreator::CreateLink(const rendering::VisualPtr &_visual)
{
  LinkData *link = new LinkData();

  msgs::Model model;
  double mass = 1.0;

  // set reasonable inertial values based on geometry
  std::string geomType = _visual->GetGeometryType();
  if (geomType == "cylinder")
    msgs::AddCylinderLink(model, mass, 0.5, 1.0);
  else if (geomType == "sphere")
    msgs::AddSphereLink(model, mass, 0.5);
  else
    msgs::AddBoxLink(model, mass, ignition::math::Vector3d::One);
  link->Load(msgs::LinkToSDF(model.link(0)));

  MainWindow *mainWindow = gui::get_main_window();
  if (mainWindow)
  {
    connect(gui::get_main_window(), SIGNAL(Close()), link->inspector,
        SLOT(close()));
  }

  link->linkVisual = _visual->GetParent();
  link->AddVisual(_visual);

  link->inspector->SetLinkId(link->linkVisual->GetName());

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
  ModelData::UpdateRenderGroup(collisionVis);
  link->AddCollision(collisionVis);

  std::string linkName = link->linkVisual->GetName();

  std::string leafName = linkName;
  size_t idx = linkName.rfind("::");
  if (idx != std::string::npos)
    leafName = linkName.substr(idx+2);

  link->SetName(leafName);

  {
    std::lock_guard<std::recursive_mutex> lock(this->updateMutex);
    this->allLinks[linkName] = link;
    if (this->canonicalLink.empty())
      this->canonicalLink = linkName;
  }

  this->ModelChanged();

  return link;
}

/////////////////////////////////////////////////
LinkData *ModelCreator::CloneLink(const std::string &_linkName)
{
  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);

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
  size_t idx = newName.rfind("::");
  if (idx != std::string::npos)
    leafName = newName.substr(idx+2);
  LinkData *link = it->second->Clone(leafName);

  this->allLinks[newName] = link;

  this->ModelChanged();

  return link;
}

/////////////////////////////////////////////////
NestedModelData *ModelCreator::CloneNestedModel(
    const std::string &_nestedModelName)
{
  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);

  auto it = this->allNestedModels.find(_nestedModelName);
  if (it == allNestedModels.end())
  {
    gzerr << "No nested model with name: " << _nestedModelName <<
        " found."  << std::endl;
    return NULL;
  }

  std::string newName = _nestedModelName + "_clone";
  std::string leafName = newName;
  size_t idx = newName.rfind("::");
  if (idx != std::string::npos)
    leafName = newName.substr(idx+2);
  sdf::ElementPtr cloneSDF = it->second->modelSDF->Clone();
  cloneSDF->GetAttribute("name")->Set(leafName);

  NestedModelData *modelData = this->CreateModelFromSDF(cloneSDF,
    it->second->modelVisual->GetParent(), false);

  this->ModelChanged();

  return modelData;
}

/////////////////////////////////////////////////
LinkData *ModelCreator::CreateLinkFromSDF(const sdf::ElementPtr &_linkElem,
    const rendering::VisualPtr &_parentVis)
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
  linkNameStream << _parentVis->GetName() << "::";
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

  rendering::VisualPtr linkVisual(
      new rendering::Visual(linkName, _parentVis, false));
  linkVisual->Load();
  linkVisual->SetPose(link->Pose());
  link->linkVisual = linkVisual;
  link->inspector->SetLinkId(link->linkVisual->GetName());

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
        linkVisual, false));
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
        linkVisual, false));

    // Collision pose
    math::Pose collisionPose;
    if (collisionElem->HasElement("pose"))
      collisionPose = collisionElem->Get<math::Pose>("pose");
    else
      collisionPose.Set(0, 0, 0, 0, 0, 0);

    // Make a visual element from the collision element
    sdf::ElementPtr colVisualElem =  this->modelTemplateSDF->Root()
        ->GetElement("model")->GetElement("link")->GetElement("visual");

    sdf::ElementPtr geomElem = colVisualElem->GetElement("geometry");
    geomElem->ClearElements();
    geomElem->Copy(collisionElem->GetElement("geometry"));

    colVisual->Load(colVisualElem);
    colVisual->SetPose(collisionPose);
    colVisual->SetMaterial("Gazebo/Orange");
    colVisual->SetTransparency(
        math::clamp(ModelData::GetEditTransparency() * 2.0, 0.0, 0.8));
    ModelData::UpdateRenderGroup(colVisual);

    // Add to link
    msgs::Collision colMsg = msgs::CollisionFromSDF(collisionElem);
    link->AddCollision(colVisual, &colMsg);

    collisionElem = collisionElem->GetNextElement("collision");
  }

  linkVisual->SetVisibilityFlags(GZ_VISIBILITY_GUI | GZ_VISIBILITY_SELECTABLE);

  // emit linkInserted events for all links, including links in nested models
  gui::model::Events::linkInserted(linkName);

  {
    std::lock_guard<std::recursive_mutex> lock(this->updateMutex);
    this->allLinks[linkName] = link;
  }

  this->ModelChanged();

  return link;
}

/////////////////////////////////////////////////
void ModelCreator::RemoveNestedModelImpl(const std::string &_nestedModelName)
{
  if (!this->previewVisual)
  {
    this->Reset();
    return;
  }

  NestedModelData *modelData = NULL;
  {
    std::lock_guard<std::recursive_mutex> lock(this->updateMutex);
    if (this->allNestedModels.find(_nestedModelName) ==
        this->allNestedModels.end())
    {
      return;
    }
    modelData = this->allNestedModels[_nestedModelName];
  }

  if (!modelData)
    return;

  // Copy before reference is deleted.
  std::string nestedModelName(_nestedModelName);

  // remove all its models
  for (auto &modelIt : modelData->models)
    this->RemoveNestedModelImpl(modelIt.first);

  // remove all its links and joints
  for (auto &linkIt : modelData->links)
  {
    // if it's a link
    if (this->allLinks.find(linkIt.first) != this->allLinks.end())
    {
      if (this->jointMaker)
      {
        this->jointMaker->RemoveJointsByLink(linkIt.first);
      }
      this->RemoveLinkImpl(linkIt.first);
    }
  }

  rendering::ScenePtr scene = modelData->modelVisual->GetScene();
  if (scene)
  {
    scene->RemoveVisual(modelData->modelVisual);
  }

  modelData->modelVisual.reset();
  {
    std::lock_guard<std::recursive_mutex> lock(this->updateMutex);
    this->allNestedModels.erase(_nestedModelName);
    delete modelData;
  }
  gui::model::Events::nestedModelRemoved(nestedModelName);

  this->ModelChanged();
}

/////////////////////////////////////////////////
void ModelCreator::RemoveLinkImpl(const std::string &_linkName)
{
  if (!this->previewVisual)
  {
    this->Reset();
    return;
  }

  LinkData *link = NULL;
  {
    std::lock_guard<std::recursive_mutex> lock(this->updateMutex);
    auto linkIt = this->allLinks.find(_linkName);
    if (linkIt == this->allLinks.end())
      return;
    link = linkIt->second;
  }

  if (!link)
    return;

  // Copy before reference is deleted.
  std::string linkName(_linkName);

  rendering::ScenePtr scene = link->linkVisual->GetScene();
  if (scene)
  {
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
  }

  link->linkVisual.reset();
  {
    std::lock_guard<std::recursive_mutex> lock(this->updateMutex);
    this->allLinks.erase(linkName);
    delete link;
  }
  gui::model::Events::linkRemoved(linkName);

  this->ModelChanged();
}

/////////////////////////////////////////////////
void ModelCreator::Reset()
{
  delete this->saveDialog;
  this->saveDialog = new SaveDialog(SaveDialog::MODEL);

  this->jointMaker->Reset();
  this->selectedLinks.clear();

  if (g_copyAct)
    g_copyAct->setEnabled(false);

  if (g_pasteAct)
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
  gui::model::Events::modelPropertiesChanged(this->isStatic, this->autoDisable);
  gui::model::Events::modelNameChanged(this->GetModelName());

  while (!this->allLinks.empty())
    this->RemoveLinkImpl(this->allLinks.begin()->first);
  this->allLinks.clear();

  while (!this->allNestedModels.empty())
    this->RemoveNestedModelImpl(this->allNestedModels.begin()->first);
  this->allNestedModels.clear();

  this->allModelPlugins.clear();

  if (!gui::get_active_camera() ||
    !gui::get_active_camera()->GetScene())
  return;

  rendering::ScenePtr scene = gui::get_active_camera()->GetScene();
  if (this->previewVisual)
    scene->RemoveVisual(this->previewVisual);

  std::stringstream previewModelName;
  previewModelName << this->previewName << "_" << this->modelCounter++;
  this->previewVisual.reset(new rendering::Visual(previewModelName.str(),
      scene->WorldVisual(), false));

  this->previewVisual->Load();
  this->modelPose = ignition::math::Pose3d::Zero;
  this->previewVisual->SetPose(this->modelPose);
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
  if (!this->modelSDF->Root()->HasElement("model"))
  {
    gzerr << "Generated invalid SDF! Cannot create entity." << std::endl;
    return;
  }

  msgs::Factory msg;
  // Create a new name if the model exists
  sdf::ElementPtr modelElem = this->modelSDF->Root()->GetElement("model");
  std::string modelElemName = modelElem->Get<std::string>("name");
  if (has_entity_name(modelElemName))
  {
    int i = 0;
    while (has_entity_name(modelElemName))
    {
      modelElemName = modelElem->Get<std::string>("name") + "_" +
        std::to_string(i++);
    }
    modelElem->GetAttribute("name")->Set(modelElemName);
  }

  msg.set_sdf(this->modelSDF->ToString());
  msgs::Set(msg.mutable_pose(), this->modelPose);
  this->makerPub->Publish(msg);
}

/////////////////////////////////////////////////
void ModelCreator::AddEntity(const sdf::ElementPtr &_sdf)
{
  if (!this->previewVisual)
  {
    this->Reset();
  }

  this->Stop();

  if (_sdf->GetName() == "model")
  {
    this->addEntityType = ENTITY_MODEL;
    NestedModelData *modelData = this->AddModel(_sdf);
    if (modelData)
      this->mouseVisual = modelData->modelVisual;
  }
}

/////////////////////////////////////////////////
void ModelCreator::AddLink(EntityType _type)
{
  if (!this->previewVisual)
  {
    this->Reset();
  }

  this->Stop();

  this->addEntityType = _type;
  if (_type != ENTITY_NONE)
  {
    LinkData *linkData = this->AddShape(_type);
    if (linkData)
      this->mouseVisual = linkData->linkVisual;
  }
}

/////////////////////////////////////////////////
void ModelCreator::Stop()
{
  if (this->addEntityType != ENTITY_NONE && this->mouseVisual)
  {
    this->RemoveEntity(this->mouseVisual->GetName());
    this->mouseVisual.reset();
    emit LinkAdded();
  }
  if (this->jointMaker)
    this->jointMaker->Stop();
}

/////////////////////////////////////////////////
void ModelCreator::OnDelete()
{
  if (this->inspectName.empty())
    return;

  this->OnDelete(this->inspectName);
  this->inspectName = "";
}

/////////////////////////////////////////////////
void ModelCreator::OnDelete(const std::string &_entity)
{
  this->RemoveEntity(_entity);
}

/////////////////////////////////////////////////
void ModelCreator::RemoveEntity(const std::string &_entity)
{
  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);

  // if it's a nestedModel
  if (this->allNestedModels.find(_entity) != this->allNestedModels.end())
  {
    this->RemoveNestedModelImpl(_entity);
    return;
  }

  // if it's a link
  if (this->allLinks.find(_entity) != this->allLinks.end())
  {
    if (this->jointMaker)
      this->jointMaker->RemoveJointsByLink(_entity);
    this->RemoveLinkImpl(_entity);
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
        this->RemoveLinkImpl(parentLink->GetName());
        return;
      }
    }
  }
}

/////////////////////////////////////////////////
void ModelCreator::OnRemoveModelPlugin(const QString &_name)
{
  this->RemoveModelPlugin(_name.toStdString());
}

/////////////////////////////////////////////////
void ModelCreator::RemoveModelPlugin(const std::string &_name)
{
  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);

  auto it = this->allModelPlugins.find(_name);
  if (it == this->allModelPlugins.end())
  {
    return;
  }

  ModelPluginData *data = it->second;

  // Remove from map
  this->allModelPlugins.erase(_name);
  delete data;

  // Notify removal
  gui::model::Events::modelPluginRemoved(_name);
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
    for (const auto &nestedModelVis : this->selectedNestedModels)
    {
      this->OnDelete(nestedModelVis->GetName());
    }

    for (const auto &linkVis : this->selectedLinks)
    {
      this->OnDelete(linkVis->GetName());
    }

    for (const auto &plugin : this->selectedModelPlugins)
    {
      this->RemoveModelPlugin(plugin);
    }
    this->DeselectAll();
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

  if (this->jointMaker->State() != JointMaker::JOINT_NONE)
  {
    userCamera->HandleMouseEvent(_event);
    return true;
  }

  rendering::VisualPtr vis = userCamera->GetVisual(_event.Pos());
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

  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);

  // case when inserting an entity
  if (this->mouseVisual)
  {
    if (_event.Button() == common::MouseEvent::RIGHT)
      return true;

    // set the link data pose
    auto linkIt = this->allLinks.find(this->mouseVisual->GetName());
    if (linkIt != this->allLinks.end())
    {
      LinkData *link = linkIt->second;
      link->SetPose((this->mouseVisual->GetWorldPose()-this->modelPose).Ign());
      gui::model::Events::linkInserted(this->mouseVisual->GetName());
    }
    else
    {
      auto modelIt = this->allNestedModels.find(this->mouseVisual->GetName());
      if (modelIt != this->allNestedModels.end())
      {
        NestedModelData *modelData = modelIt->second;
        modelData->SetPose((
            this->mouseVisual->GetWorldPose()-this->modelPose).Ign());

        this->EmitNestedModelInsertedEvent(this->mouseVisual);
      }
    }

    // reset and return
    emit LinkAdded();
    this->mouseVisual.reset();
    this->AddLink(ENTITY_NONE);
    return true;
  }

  // mouse selection and context menu events
  rendering::VisualPtr vis = userCamera->GetVisual(_event.Pos());
  if (vis)
  {
    rendering::VisualPtr topLevelVis = vis->GetNthAncestor(2);
    if (!topLevelVis)
      return false;

    bool isLink = this->allLinks.find(topLevelVis->GetName()) !=
        this->allLinks.end();
    bool isNestedModel = this->allNestedModels.find(topLevelVis->GetName()) !=
        this->allNestedModels.end();

    bool isSelectedLink = false;
    bool isSelectedNestedModel = false;
    if (isLink)
    {
      isSelectedLink = std::find(this->selectedLinks.begin(),
          this->selectedLinks.end(), topLevelVis) !=
          this->selectedLinks.end();
    }
    else if (isNestedModel)
    {
      isSelectedNestedModel = std::find(this->selectedNestedModels.begin(),
          this->selectedNestedModels.end(), topLevelVis) !=
          this->selectedNestedModels.end();
    }

    // trigger context menu on right click
    if (_event.Button() == common::MouseEvent::RIGHT)
    {
      if (!isLink && !isNestedModel)
      {
        // user clicked on background model
        this->DeselectAll();
        QMenu menu;
        menu.addAction(g_copyAct);
        menu.addAction(g_pasteAct);
        menu.exec(QCursor::pos());
        return true;
      }

      // if right clicked on entity that's not previously selected then
      // select it
      if (!isSelectedLink && !isSelectedNestedModel)
      {
        this->DeselectAll();
        this->SetSelected(topLevelVis, true);
      }

      this->inspectName = topLevelVis->GetName();

      this->ShowContextMenu(this->inspectName);
      return true;
    }

    // Handle snap from GLWidget
    if (g_snapAct->isChecked())
      return false;

    // Is link / nested model
    if (isLink || isNestedModel)
    {
      // Not in multi-selection mode.
      if (!(QApplication::keyboardModifiers() & Qt::ControlModifier))
      {
        this->DeselectAll();
        this->SetSelected(topLevelVis, true);
      }
      // Multi-selection mode
      else
      {
        this->DeselectAllModelPlugins();

        // Highlight and select clicked entity if not already selected
        if (!isSelectedLink && !isSelectedNestedModel)
        {
          this->SetSelected(topLevelVis, true);
        }
        // Deselect if already selected
        else
        {
          this->SetSelected(topLevelVis, false);
        }
      }

      if (this->manipMode == "translate" || this->manipMode == "rotate" ||
          this->manipMode == "scale")
      {
        this->OnManipMode(this->manipMode);
      }

      return true;
    }
    // Not link or nested model
    else
    {
      this->DeselectAll();

      g_alignAct->setEnabled(false);
      g_copyAct->setEnabled(!this->selectedLinks.empty() ||
          !this->selectedNestedModels.empty());

      if (!vis->IsPlane())
        return true;
    }
  }
  return false;
}

/////////////////////////////////////////////////
void ModelCreator::EmitNestedModelInsertedEvent(
    const rendering::VisualPtr &_vis) const
{
  if (!_vis)
    return;

  auto modelIt = this->allNestedModels.find(_vis->GetName());
  if (modelIt != this->allNestedModels.end())
    gui::model::Events::nestedModelInserted(_vis->GetName());
  else
    return;

  for (unsigned int i = 0; i < _vis->GetChildCount(); ++i)
    this->EmitNestedModelInsertedEvent(_vis->GetChild(i));
}

/////////////////////////////////////////////////
void ModelCreator::ShowContextMenu(const std::string &_entity)
{
  QMenu menu;
  auto linkIt = this->allLinks.find(_entity);
  bool isLink = linkIt != this->allLinks.end();
  bool isNestedModel = false;
  if (!isLink)
  {
    auto nestedModelIt = this->allNestedModels.find(_entity);
    isNestedModel = nestedModelIt != this->allNestedModels.end();
    if (!isNestedModel)
      return;
  }
  else
  {
    // disable interacting with nested links for now
    LinkData *link = linkIt->second;
    if (link->nested)
      return;
  }

  // context menu for link
  if (isLink)
  {
    this->inspectName = _entity;
    if (this->inspectAct)
    {
      menu.addAction(this->inspectAct);

      menu.addSeparator();
      menu.addAction(g_copyAct);
      menu.addAction(g_pasteAct);
      menu.addSeparator();

      if (this->jointMaker)
      {
        std::vector<JointData *> joints = this->jointMaker->JointDataByLink(
            _entity);

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
      }
    }
  }
  // context menu for nested model
  else if (isNestedModel)
  {
    this->inspectName = _entity;
    menu.addAction(g_copyAct);
    menu.addAction(g_pasteAct);
  }

  // delete menu option
  menu.addSeparator();

  QAction *deleteAct = new QAction(tr("Delete"), this);
  connect(deleteAct, SIGNAL(triggered()), this, SLOT(OnDelete()));
  menu.addAction(deleteAct);

  menu.exec(QCursor::pos());
}

/////////////////////////////////////////////////
void ModelCreator::ShowModelPluginContextMenu(const std::string &_name)
{
  auto it = this->allModelPlugins.find(_name);
  if (it == this->allModelPlugins.end())
    return;

  // Open inspector
  QAction *inspectorAct = new QAction(tr("Open Model Plugin Inspector"), this);

  // Map signals to pass argument
  QSignalMapper *inspectorMapper = new QSignalMapper(this);

  connect(inspectorAct, SIGNAL(triggered()), inspectorMapper, SLOT(map()));
  inspectorMapper->setMapping(inspectorAct, QString::fromStdString(_name));

  connect(inspectorMapper, SIGNAL(mapped(QString)), this,
      SLOT(OnOpenModelPluginInspector(QString)));

  // Delete
  QAction *deleteAct = new QAction(tr("Delete"), this);

  // Map signals to pass argument
  QSignalMapper *deleteMapper = new QSignalMapper(this);

  connect(deleteAct, SIGNAL(triggered()), deleteMapper, SLOT(map()));
  deleteMapper->setMapping(deleteAct, QString::fromStdString(_name));

  connect(deleteMapper, SIGNAL(mapped(QString)), this,
      SLOT(OnRemoveModelPlugin(QString)));

  // Menu
  QMenu menu;
  menu.addAction(inspectorAct);
  menu.addAction(deleteAct);

  menu.exec(QCursor::pos());
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
    rendering::VisualPtr vis = userCamera->GetVisual(_event.Pos());
    if (vis && !vis->IsPlane())
    {
      rendering::VisualPtr topLevelVis = vis->GetNthAncestor(2);
      if (!topLevelVis)
        return false;

      // Main window models always handled here
      if (this->allLinks.find(topLevelVis->GetName()) ==
          this->allLinks.end() &&
          this->allNestedModels.find(topLevelVis->GetName()) ==
          this->allNestedModels.end())
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
        else if (_event.Dragging())
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

  if (!_event.Shift())
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
  rendering::VisualPtr vis = gui::get_active_camera()->GetVisual(_event.Pos());
  if (!vis)
    return false;

  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);

  auto it = this->allLinks.find(vis->GetParent()->GetName());
  if (it != this->allLinks.end())
  {
    this->OpenInspector(vis->GetParent()->GetName());
    return true;
  }

  return false;
}

/////////////////////////////////////////////////
void ModelCreator::OnOpenInspector()
{
  if (this->inspectName.empty())
    return;

  this->OpenInspector(this->inspectName);
  this->inspectName = "";
}

/////////////////////////////////////////////////
void ModelCreator::OpenInspector(const std::string &_name)
{
  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);
  auto it = this->allLinks.find(_name);
  if (it == this->allLinks.end())
  {
    gzerr << "Link [" << _name << "] not found." << std::endl;
    return;
  }

  // disable interacting with nested links for now
  LinkData *link = it->second;
  if (link->nested)
    return;

  link->SetPose((link->linkVisual->GetWorldPose()-this->modelPose).Ign());
  link->UpdateConfig();
  link->inspector->Open();
}

/////////////////////////////////////////////////
void ModelCreator::OnCopy()
{
  if (!g_editModelAct->isChecked())
    return;

  if (this->selectedLinks.empty()  && this->selectedNestedModels.empty())
    return;

  this->copiedNames.clear();

  for (auto vis : this->selectedLinks)
  {
    this->copiedNames.push_back(vis->GetName());
  }
  for (auto vis : this->selectedNestedModels)
  {
    this->copiedNames.push_back(vis->GetName());
  }
  g_pasteAct->setEnabled(true);
}

/////////////////////////////////////////////////
void ModelCreator::OnPaste()
{
  if (this->copiedNames.empty() || !g_editModelAct->isChecked())
  {
    return;
  }

  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);

  ignition::math::Pose3d clonePose;
  rendering::UserCameraPtr userCamera = gui::get_active_camera();
  if (userCamera)
  {
    ignition::math::Vector3d mousePosition =
        ModelManipulator::GetMousePositionOnPlane(
        userCamera, this->lastMouseEvent).Ign();
    clonePose.Pos().X() = mousePosition.X();
    clonePose.Pos().Y() = mousePosition.Y();
  }

  // For now, only copy the last selected (nested models come after)
  auto it = this->allLinks.find(this->copiedNames.back());
  // Copy a link
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

    // Propagate copied entity's Z position and rotation
    ignition::math::Pose3d copiedPose = copiedLink->Pose();
    clonePose.Pos().Z() = this->modelPose.Pos().Z() + copiedPose.Pos().Z();
    clonePose.Rot() = copiedPose.Rot();

    LinkData *clonedLink = this->CloneLink(it->first);
    clonedLink->linkVisual->SetWorldPose(clonePose);

    this->addEntityType = ENTITY_MESH;
    this->mouseVisual = clonedLink->linkVisual;
  }
  else
  {
    auto it2 = this->allNestedModels.find(this->copiedNames.back());
    if (it2 != this->allNestedModels.end())
    {
      NestedModelData *copiedNestedModel = it2->second;
      if (!copiedNestedModel)
        return;

      this->Stop();
      this->DeselectAll();

      if (!this->previewVisual)
      {
        this->Reset();
      }

      // Propagate copied entity's Z position and rotation
      ignition::math::Pose3d copiedPose = copiedNestedModel->Pose();
      clonePose.Pos().Z() = this->modelPose.Pos().Z() + copiedPose.Pos().Z();
      clonePose.Rot() = copiedPose.Rot();

      NestedModelData *clonedNestedModel = this->CloneNestedModel(it2->first);
      clonedNestedModel->modelVisual->SetWorldPose(clonePose);
      this->addEntityType = ENTITY_MODEL;
      this->mouseVisual = clonedNestedModel->modelVisual;
    }
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

  modelElem = this->modelSDF->Root()->GetElement("model");

  modelElem->ClearElements();
  modelElem->GetAttribute("name")->Set(this->folderName);

  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);

  if (this->serverModelName.empty())
  {
    // set center of all links and nested models to be origin
    /// \todo issue #1485 set a better origin other than the centroid
    ignition::math::Vector3d mid;
    int entityCount = 0;
    for (auto &linksIt : this->allLinks)
    {
      LinkData *link = linksIt.second;
      if (link->nested)
        continue;
      mid += link->Pose().Pos();
      entityCount++;
    }
    for (auto &nestedModelsIt : this->allNestedModels)
    {
      NestedModelData *modelData = nestedModelsIt.second;

      // get only top level nested models
      if (modelData->Depth() != 2)
        continue;

      mid += modelData->Pose().Pos();
      entityCount++;
    }

    if (!(this->allLinks.empty() && this->allNestedModels.empty()))
    {
      mid /= entityCount;
    }

    this->modelPose.Pos() = mid;
  }

  // Update poses in case they changed
  for (auto &linksIt : this->allLinks)
  {
    LinkData *link = linksIt.second;
    if (link->nested)
      continue;
    ignition::math::Pose3d linkPose =
        link->linkVisual->GetWorldPose().Ign() - this->modelPose;
    link->SetPose(linkPose);
    link->linkVisual->SetPose(linkPose);
  }
  for (auto &nestedModelsIt : this->allNestedModels)
  {
    NestedModelData *modelData = nestedModelsIt.second;

    if (!modelData->modelVisual)
      continue;

    // get only top level nested models
    if (modelData->Depth() != 2)
      continue;

    ignition::math::Pose3d nestedModelPose =
        modelData->modelVisual->GetWorldPose().Ign() - this->modelPose;
    modelData->SetPose(nestedModelPose);
    modelData->modelVisual->SetPose(nestedModelPose);
  }

  // generate canonical link sdf first.
  if (!this->canonicalLink.empty())
  {
    auto canonical = this->allLinks.find(this->canonicalLink);
    if (canonical != this->allLinks.end())
    {
      LinkData *link = canonical->second;
      if (!link->nested)
      {
        link->UpdateConfig();
        sdf::ElementPtr newLinkElem = this->GenerateLinkSDF(link);
        modelElem->InsertElement(newLinkElem);
      }
    }
  }

  // loop through rest of all links and generate sdf
  for (auto &linksIt : this->allLinks)
  {
    LinkData *link = linksIt.second;

    if (linksIt.first == this->canonicalLink || link->nested)
      continue;

    link->UpdateConfig();

    sdf::ElementPtr newLinkElem = this->GenerateLinkSDF(link);
    modelElem->InsertElement(newLinkElem);
  }

  // generate canonical model sdf first.
  if (!this->canonicalModel.empty())
  {
    auto canonical = this->allNestedModels.find(this->canonicalModel);
    if (canonical != this->allNestedModels.end())
    {
      NestedModelData *nestedModelData = canonical->second;
      modelElem->InsertElement(nestedModelData->modelSDF);
    }
  }

  // loop through rest of all nested models and add sdf
  for (auto &nestedModelsIt : this->allNestedModels)
  {
    NestedModelData *nestedModelData = nestedModelsIt.second;

    if (nestedModelsIt.first == this->canonicalModel ||
        nestedModelData->Depth() != 2)
      continue;

    modelElem->InsertElement(nestedModelData->modelSDF);
  }

  // Add joint sdf elements
  this->jointMaker->GenerateSDF();
  sdf::ElementPtr jointsElem = this->jointMaker->SDF();

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

  // Add plugin elements
  for (auto modelPlugin : this->allModelPlugins)
    modelElem->InsertElement(modelPlugin.second->modelPluginSDF->Clone());

  // update root visual pose at the end after link, model, joint visuals
  this->previewVisual->SetWorldPose(this->modelPose);
}

/////////////////////////////////////////////////
sdf::ElementPtr ModelCreator::GenerateLinkSDF(LinkData *_link)
{
  std::stringstream visualNameStream;
  std::stringstream collisionNameStream;
  visualNameStream.str("");
  collisionNameStream.str("");

  sdf::ElementPtr newLinkElem = _link->linkSDF->Clone();
  newLinkElem->GetElement("pose")->Set(_link->Pose());

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
    const std::string &_config, const std::string &_target, const bool _preview,
    const bool _inverted)
{
  ModelAlign::Instance()->AlignVisuals(this->selectedLinks, _axis, _config,
      _target, !_preview, _inverted);
}

/////////////////////////////////////////////////
void ModelCreator::DeselectAll()
{
  this->DeselectAllLinks();
  this->DeselectAllNestedModels();
  this->DeselectAllModelPlugins();
}

/////////////////////////////////////////////////
void ModelCreator::DeselectAllLinks()
{
  while (!this->selectedLinks.empty())
  {
    rendering::VisualPtr vis = this->selectedLinks[0];
    vis->SetHighlighted(false);
    this->selectedLinks.erase(this->selectedLinks.begin());
    model::Events::setSelectedLink(vis->GetName(), false);
  }
  this->selectedLinks.clear();
}

/////////////////////////////////////////////////
void ModelCreator::DeselectAllNestedModels()
{
  while (!this->selectedNestedModels.empty())
  {
    rendering::VisualPtr vis = this->selectedNestedModels[0];
    vis->SetHighlighted(false);
    this->selectedNestedModels.erase(this->selectedNestedModels.begin());
    model::Events::setSelectedLink(vis->GetName(), false);
  }
  this->selectedNestedModels.clear();
}

/////////////////////////////////////////////////
void ModelCreator::DeselectAllModelPlugins()
{
  while (!this->selectedModelPlugins.empty())
  {
    auto it = this->selectedModelPlugins.begin();
    std::string name = this->selectedModelPlugins[0];
    this->selectedModelPlugins.erase(it);
    model::Events::setSelectedModelPlugin(name, false);
  }
}

/////////////////////////////////////////////////
void ModelCreator::SetSelected(const std::string &_name, const bool _selected)
{
  auto it = this->allLinks.find(_name);
  if (it != this->allLinks.end())
  {
    this->SetSelected((*it).second->linkVisual, _selected);
  }
  else
  {
    auto itNestedModel = this->allNestedModels.find(_name);
    if (itNestedModel != this->allNestedModels.end())
      this->SetSelected((*itNestedModel).second->modelVisual, _selected);
  }
}

/////////////////////////////////////////////////
void ModelCreator::SetSelected(rendering::VisualPtr _entityVis,
    const bool _selected)
{
  if (!_entityVis)
    return;

  _entityVis->SetHighlighted(_selected);

  auto itLink = this->allLinks.find(_entityVis->GetName());
  auto itNestedModel = this->allNestedModels.find(_entityVis->GetName());

  auto itLinkSelected = std::find(this->selectedLinks.begin(),
      this->selectedLinks.end(), _entityVis);
  auto itNestedModelSelected = std::find(this->selectedNestedModels.begin(),
      this->selectedNestedModels.end(), _entityVis);

  if (_selected)
  {
    if (itLink != this->allLinks.end() &&
        itLinkSelected == this->selectedLinks.end())
    {
      this->selectedLinks.push_back(_entityVis);
      model::Events::setSelectedLink(_entityVis->GetName(), _selected);
    }
    else if (itNestedModel != this->allNestedModels.end() &&
             itNestedModelSelected == this->selectedNestedModels.end())
    {
      this->selectedNestedModels.push_back(_entityVis);
      model::Events::setSelectedLink(_entityVis->GetName(), _selected);
    }
  }
  else
  {
    if (itLink != this->allLinks.end() &&
        itLinkSelected != this->selectedLinks.end())
    {
      this->selectedLinks.erase(itLinkSelected);
      model::Events::setSelectedLink(_entityVis->GetName(), _selected);
    }
    else if (itNestedModel != this->allNestedModels.end() &&
             itNestedModelSelected != this->selectedNestedModels.end())
    {
      this->selectedNestedModels.erase(itNestedModelSelected);
      model::Events::setSelectedLink(_entityVis->GetName(), _selected);
    }
  }

  g_copyAct->setEnabled(this->selectedLinks.size() +
      this->selectedNestedModels.size() > 0u);
  g_alignAct->setEnabled(this->selectedLinks.size() +
      this->selectedNestedModels.size() > 1u);
}

/////////////////////////////////////////////////
void ModelCreator::OnManipMode(const std::string &_mode)
{
  if (!this->active)
    return;

  this->manipMode = _mode;

  if (!this->selectedLinks.empty())
  {
    ModelManipulator::Instance()->SetAttachedVisual(
        this->selectedLinks.back());
  }

  ModelManipulator::Instance()->SetManipulationMode(_mode);
  ModelSnap::Instance()->Reset();

  // deselect 0 to n-1 models.
  if (!this->selectedLinks.empty())
  {
    rendering::VisualPtr link =
        this->selectedLinks[this->selectedLinks.size()-1];
    this->DeselectAll();
    this->SetSelected(link, true);
  }
  else if (!this->selectedNestedModels.empty())
  {
    rendering::VisualPtr nestedModel =
        this->selectedNestedModels[this->selectedNestedModels.size()-1];
    this->DeselectAll();
    this->SetSelected(nestedModel, true);
  }
}

/////////////////////////////////////////////////
void ModelCreator::OnSetSelectedEntity(const std::string &/*_name*/,
    const std::string &/*_mode*/)
{
  this->DeselectAll();
}

/////////////////////////////////////////////////
void ModelCreator::OnSetSelectedLink(const std::string &_name,
    const bool _selected)
{
  this->SetSelected(_name, _selected);
}

/////////////////////////////////////////////////
void ModelCreator::OnSetSelectedModelPlugin(const std::string &_name,
    const bool _selected)
{
  auto plugin = this->allModelPlugins.find(_name);
  if (plugin == this->allModelPlugins.end())
    return;

  auto it = std::find(this->selectedModelPlugins.begin(),
      this->selectedModelPlugins.end(), _name);
  if (_selected && it == this->selectedModelPlugins.end())
  {
    this->selectedModelPlugins.push_back(_name);
  }
  else if (!_selected && it != this->selectedModelPlugins.end())
  {
    this->selectedModelPlugins.erase(it);
  }
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
  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);

  // Check if any links have been moved or resized and trigger ModelChanged
  for (auto &linksIt : this->allLinks)
  {
    LinkData *link = linksIt.second;
    if (link->Pose() != link->linkVisual->GetPose().Ign())
    {
      link->SetPose((link->linkVisual->GetWorldPose() - this->modelPose).Ign());
      this->ModelChanged();
    }
    for (auto &scaleIt : this->linkScaleUpdate)
    {
      if (link->linkVisual->GetName() == scaleIt.first)
        link->SetScale(scaleIt.second.Ign());
    }
  }
  if (!this->linkScaleUpdate.empty())
    this->ModelChanged();
  this->linkScaleUpdate.clear();

  // check nested model
  for (auto &nestedModelIt : this->allNestedModels)
  {
    NestedModelData *nestedModel = nestedModelIt.second;
    if (nestedModel->Pose() != nestedModel->modelVisual->GetPose().Ign())
    {
      nestedModel->SetPose(
            (nestedModel->modelVisual->GetWorldPose() - this->modelPose).Ign());
      this->ModelChanged();
    }
  }
}

/////////////////////////////////////////////////
void ModelCreator::OnEntityScaleChanged(const std::string &_name,
  const math::Vector3 &_scale)
{
  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);
  for (auto linksIt : this->allLinks)
  {
    std::string linkName;
    size_t pos = _name.rfind("::");
    if (pos != std::string::npos)
      linkName = _name.substr(0, pos);
    if (_name == linksIt.first || linkName == linksIt.first)
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

/////////////////////////////////////////////////
void ModelCreator::OnAddModelPlugin(const std::string &_name,
    const std::string &_filename, const std::string &_innerxml)
{
  if (_name.empty() || _filename.empty())
  {
    gzerr << "Cannot add model plugin. Empty name or filename" << std::endl;
    return;
  }

  // Use the SDF parser to read all the inner xml.
  sdf::ElementPtr modelPluginSDF(new sdf::Element);
  sdf::initFile("plugin.sdf", modelPluginSDF);
  std::stringstream tmp;
  tmp << "<sdf version='" << SDF_VERSION << "'>";
  tmp << "<plugin name='" << _name << "' filename='" << _filename << "'>";
  tmp << _innerxml;
  tmp << "</plugin></sdf>";

  if (sdf::readString(tmp.str(), modelPluginSDF))
  {
    this->AddModelPlugin(modelPluginSDF);
    this->ModelChanged();
  }
  else
  {
    gzerr << "Error reading Plugin SDF. Unable to parse Innerxml:\n"
        << _innerxml << std::endl;
  }
}

/////////////////////////////////////////////////
void ModelCreator::AddModelPlugin(const sdf::ElementPtr &_pluginElem)
{
  if (_pluginElem->HasAttribute("name"))
  {
    std::string name = _pluginElem->Get<std::string>("name");

    // Create data
    ModelPluginData *modelPlugin = new ModelPluginData();
    modelPlugin->Load(_pluginElem);

    // Add to map
    {
      std::lock_guard<std::recursive_mutex> lock(this->updateMutex);
      this->allModelPlugins[name] = modelPlugin;
    }

    // Notify addition
    gui::model::Events::modelPluginInserted(name);
  }
}

/////////////////////////////////////////////////
ModelPluginData *ModelCreator::ModelPlugin(const std::string &_name)
{
  auto it = this->allModelPlugins.find(_name);
  if (it != this->allModelPlugins.end())
    return it->second;
  return NULL;
}

/////////////////////////////////////////////////
void ModelCreator::OnOpenModelPluginInspector(const QString &_name)
{
  this->OpenModelPluginInspector(_name.toStdString());
}

/////////////////////////////////////////////////
void ModelCreator::OpenModelPluginInspector(const std::string &_name)
{
  std::lock_guard<std::recursive_mutex> lock(this->updateMutex);

  auto it = this->allModelPlugins.find(_name);
  if (it == this->allModelPlugins.end())
  {
    gzerr << "Model plugin [" << _name << "] not found." << std::endl;
    return;
  }

  ModelPluginData *modelPlugin = it->second;
  modelPlugin->inspector->move(QCursor::pos());
  modelPlugin->inspector->show();
}
