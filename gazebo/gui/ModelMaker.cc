/*
 * Copyright (C) 2012-2016 Open Source Robotics Foundation
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

#include <sstream>

#include "gazebo/msgs/msgs.hh"

#include "gazebo/common/Console.hh"
#include "gazebo/common/Exception.hh"

#include "gazebo/rendering/UserCamera.hh"
#include "gazebo/rendering/Visual.hh"
#include "gazebo/rendering/Scene.hh"

#include "gazebo/transport/Publisher.hh"
#include "gazebo/transport/Node.hh"

#include "gazebo/gui/ModelManipulator.hh"
#include "gazebo/gui/GuiIface.hh"
#include "gazebo/gui/MainWindow.hh"
#include "gazebo/gui/ModelMakerPrivate.hh"
#include "gazebo/gui/ModelMaker.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
ModelMaker::ModelMaker() : EntityMaker(*new ModelMakerPrivate)
{
  ModelMakerPrivate *dPtr =
      static_cast<ModelMakerPrivate *>(this->dataPtr);

  dPtr->clone = false;
  dPtr->makerPub = dPtr->node->Advertise<msgs::Factory>("~/factory");
}

/////////////////////////////////////////////////
ModelMaker::~ModelMaker()
{
  ModelMakerPrivate *dPtr =
      static_cast<ModelMakerPrivate *>(this->dataPtr);

  dPtr->makerPub.reset();
}

/////////////////////////////////////////////////
bool ModelMaker::InitFromModel(const std::string &_modelName)
{
  ModelMakerPrivate *dPtr =
      static_cast<ModelMakerPrivate *>(this->dataPtr);

  rendering::ScenePtr scene = gui::get_active_camera()->GetScene();
  if (!scene)
    return false;

  if (dPtr->modelVisual)
  {
    scene->RemoveVisual(dPtr->modelVisual);
    dPtr->modelVisual.reset();
    dPtr->visuals.clear();
  }

  rendering::VisualPtr vis = scene->GetVisual(_modelName);
  if (!vis)
  {
    gzerr << "Model: '" << _modelName << "' does not exist." << std::endl;
    return false;
  }

  dPtr->modelVisual = vis->Clone(
      _modelName + "_clone_tmp", scene->WorldVisual());

  if (!dPtr->modelVisual)
  {
    gzerr << "Unable to clone\n";
    return false;
  }
  dPtr->clone = true;
  return true;
}

/////////////////////////////////////////////////
bool ModelMaker::InitFromFile(const std::string &_filename)
{
  ModelMakerPrivate *dPtr =
      static_cast<ModelMakerPrivate *>(this->dataPtr);

  dPtr->clone = false;
  rendering::ScenePtr scene = gui::get_active_camera()->GetScene();

  if (dPtr->modelVisual)
  {
    scene->RemoveVisual(dPtr->modelVisual);
    dPtr->modelVisual.reset();
  }

  dPtr->modelSDF.reset(new sdf::SDF);
  sdf::initFile("root.sdf", dPtr->modelSDF);

  if (!sdf::readFile(_filename, dPtr->modelSDF))
  {
    gzerr << "Unable to load file[" << _filename << "]\n";
    return false;
  }

  return this->Init();
}

/////////////////////////////////////////////////
bool ModelMaker::InitSimpleShape(SimpleShapes _shape)
{
  ModelMakerPrivate *dPtr =
      static_cast<ModelMakerPrivate *>(this->dataPtr);

  dPtr->clone = false;
  rendering::ScenePtr scene = gui::get_active_camera()->GetScene();
  if (!scene)
    return false;

  if (dPtr->modelVisual)
  {
    scene->RemoveVisual(dPtr->modelVisual);
    dPtr->modelVisual.reset();
  }

  // Unique name
  std::string prefix;
  if (_shape == BOX)
    prefix = "unit_box_";
  else if (_shape == SPHERE)
    prefix = "unit_sphere_";
  else if (_shape == CYLINDER)
    prefix = "unit_cylinder_";

  int counter = 0;
  std::string modelName;
  do
  {
    modelName = prefix + std::to_string(counter++);
  } while (scene->GetVisual(modelName));

  // Model message
  msgs::Model model;
  model.set_name(modelName);
  msgs::Set(model.mutable_pose(), ignition::math::Pose3d(0, 0, 0.5, 0, 0, 0));
  if (_shape == BOX)
    msgs::AddBoxLink(model, 1.0, ignition::math::Vector3d::One);
  else if (_shape == SPHERE)
    msgs::AddSphereLink(model, 1.0, 0.5);
  else if (_shape == CYLINDER)
    msgs::AddCylinderLink(model, 1.0, 0.5, 1.0);
  model.mutable_link(0)->set_name("link");

  // Model SDF
  std::string modelString = "<sdf version='" + std::string(SDF_VERSION) + "'>"
       + msgs::ModelToSDF(model)->ToString("") + "</sdf>";

  dPtr->modelSDF.reset(new sdf::SDF);
  sdf::initFile("root.sdf", dPtr->modelSDF);

  if (!sdf::readString(modelString, dPtr->modelSDF))
  {
    gzerr << "Unable to load SDF [" << modelString << "]" << std::endl;
    return false;
  }

  return this->Init();
}

/////////////////////////////////////////////////
bool ModelMaker::Init()
{
  ModelMakerPrivate *dPtr =
      static_cast<ModelMakerPrivate *>(this->dataPtr);

  rendering::ScenePtr scene = gui::get_active_camera()->GetScene();
  if (!scene)
    return false;

  // Load the world file
  std::string modelName;
  ignition::math::Pose3d modelPose, linkPose, visualPose;
  sdf::ElementPtr modelElem;

  if (dPtr->modelSDF->Root()->HasElement("model"))
    modelElem = dPtr->modelSDF->Root()->GetElement("model");
  else if (dPtr->modelSDF->Root()->HasElement("light"))
    modelElem = dPtr->modelSDF->Root()->GetElement("light");
  else
  {
    gzerr << "No model or light in SDF\n";
    return false;
  }

  if (modelElem->HasElement("pose"))
    modelPose = modelElem->Get<ignition::math::Pose3d>("pose");

  modelName = modelElem->Get<std::string>("name");

  dPtr->modelVisual.reset(new rendering::Visual(
      dPtr->node->GetTopicNamespace() + "::" + modelName,
      scene->WorldVisual()));
  dPtr->modelVisual->Load();
  dPtr->modelVisual->SetPose(modelPose);

  modelName = dPtr->modelVisual->GetName();
  modelElem->GetAttribute("name")->Set(modelName);

  if (modelElem->GetName() == "model")
  {
    this->CreateModelFromSDF(modelElem);
  }
  else if (modelElem->GetName() == "light")
  {
    dPtr->modelVisual->AttachMesh("unit_sphere");
  }
  else
  {
    return false;
  }

  return true;
}

/////////////////////////////////////////////////
void ModelMaker::CreateModelFromSDF(sdf::ElementPtr _modelElem)
{
  ModelMakerPrivate *dPtr =
      static_cast<ModelMakerPrivate *>(this->dataPtr);

  ignition::math::Pose3d linkPose, visualPose;
  std::list<std::pair<sdf::ElementPtr, rendering::VisualPtr> > modelElemList;

  std::pair<sdf::ElementPtr, rendering::VisualPtr> pair(
      _modelElem, dPtr->modelVisual);
  modelElemList.push_back(pair);

  while (!modelElemList.empty())
  {
    sdf::ElementPtr modelElem = modelElemList.front().first;
    rendering::VisualPtr modelVis = modelElemList.front().second;
    modelElemList.pop_front();

    std::string modelName = modelVis->GetName();

    // create model
    sdf::ElementPtr linkElem;
    if (modelElem->HasElement("link"))
      linkElem = modelElem->GetElement("link");

    try
    {
      while (linkElem)
      {
        std::string linkName = linkElem->Get<std::string>("name");
        if (linkElem->HasElement("pose"))
          linkPose = linkElem->Get<ignition::math::Pose3d>("pose");
        else
          linkPose.Set(0, 0, 0, 0, 0, 0);

        rendering::VisualPtr linkVisual(new rendering::Visual(modelName + "::" +
              linkName, modelVis));
        linkVisual->Load();
        linkVisual->SetPose(linkPose);
        dPtr->visuals.push_back(linkVisual);

        int visualIndex = 0;
        sdf::ElementPtr visualElem;

        if (linkElem->HasElement("visual"))
          visualElem = linkElem->GetElement("visual");

        while (visualElem)
        {
          if (visualElem->HasElement("pose"))
            visualPose = visualElem->Get<ignition::math::Pose3d>("pose");
          else
            visualPose.Set(0, 0, 0, 0, 0, 0);

          std::ostringstream visualName;
          visualName << modelName << "::" << linkName << "::Visual_"
            << visualIndex++;
          rendering::VisualPtr visVisual(new rendering::Visual(visualName.str(),
                linkVisual));

          visVisual->Load(visualElem);
          visVisual->SetPose(visualPose);
          dPtr->visuals.push_back(visVisual);

          visualElem = visualElem->GetNextElement("visual");
        }

        linkElem = linkElem->GetNextElement("link");
      }
    }
    catch(common::Exception &_e)
    {
      this->Stop();
    }

    // append other model elems to the list
    if (modelElem->HasElement("model"))
    {
      sdf::ElementPtr childElem = modelElem->GetElement("model");
      while (childElem)
      {
        rendering::VisualPtr childVis;
        std::string childName = childElem->Get<std::string>("name");
        childVis.reset(new rendering::Visual(modelName + "::" + childName,
            modelVis));
        childVis->Load();
        dPtr->visuals.push_back(childVis);

        math::Pose childPose;
        if (childElem->HasElement("pose"))
          childPose = childElem->Get<math::Pose>("pose");
        childVis->SetPose(childPose);

        std::pair<sdf::ElementPtr, rendering::VisualPtr> childPair(
            childElem, childVis);
        modelElemList.push_back(childPair);

        childElem = childElem->GetNextElement("model");
      }
    }
  }
}

/////////////////////////////////////////////////
void ModelMaker::Stop()
{
  ModelMakerPrivate *dPtr =
      static_cast<ModelMakerPrivate *>(this->dataPtr);

  // Remove the temporary visual from the scene
  rendering::ScenePtr scene = gui::get_active_camera()->GetScene();
  if (scene)
  {
    for (auto vis : dPtr->visuals)
    {
      auto v = vis.lock();
      if (v)
        scene->RemoveVisual(v);
    }
    scene->RemoveVisual(dPtr->modelVisual);
  }
  dPtr->modelVisual.reset();
  dPtr->modelSDF.reset();

  EntityMaker::Stop();
}

/////////////////////////////////////////////////
void ModelMaker::CreateTheEntity()
{
  ModelMakerPrivate *dPtr =
      static_cast<ModelMakerPrivate *>(this->dataPtr);

  msgs::Factory msg;
  if (!dPtr->clone)
  {
    sdf::ElementPtr modelElem;
    bool isModel = false;
    bool isLight = false;
    if (dPtr->modelSDF->Root()->HasElement("model"))
    {
      modelElem = dPtr->modelSDF->Root()->GetElement("model");
      isModel = true;
    }
    else if (dPtr->modelSDF->Root()->HasElement("light"))
    {
      modelElem = dPtr->modelSDF->Root()->GetElement("light");
      isLight = true;
    }

    std::string modelName = modelElem->Get<std::string>("name");

    // Automatically create a new name if the model exists
    rendering::ScenePtr scene = gui::get_active_camera()->GetScene();
    gui::MainWindow *mainWindow = gui::get_main_window();
    if (scene && mainWindow)
    {
      int i = 0;
      while ((isModel && mainWindow->HasEntityName(modelName)) ||
          (isLight && scene->GetLight(modelName)))
      {
        modelName = modelElem->Get<std::string>("name") + "_" +
            std::to_string(i++);
      }
    }

    // Remove the topic namespace from the model name. This will get re-inserted
    // by the World automatically
    modelName.erase(0, dPtr->node->GetTopicNamespace().size()+2);

    // The the SDF model's name
    modelElem->GetAttribute("name")->Set(modelName);
    modelElem->GetElement("pose")->Set(
        dPtr->modelVisual->GetWorldPose().Ign());

    // Spawn the model in the physics server
    msg.set_sdf(dPtr->modelSDF->ToString());
  }
  else
  {
    msgs::Set(msg.mutable_pose(), dPtr->modelVisual->GetWorldPose().Ign());
    msg.set_clone_model_name(dPtr->modelVisual->GetName().substr(0,
          dPtr->modelVisual->GetName().find("_clone_tmp")));
  }

  dPtr->makerPub->Publish(msg);
}

/////////////////////////////////////////////////
ignition::math::Vector3d ModelMaker::EntityPosition() const
{
  ModelMakerPrivate *dPtr =
      static_cast<ModelMakerPrivate *>(this->dataPtr);

  return dPtr->modelVisual->GetWorldPose().pos.Ign();
}

/////////////////////////////////////////////////
void ModelMaker::SetEntityPosition(const ignition::math::Vector3d &_pos)
{
  ModelMakerPrivate *dPtr =
      static_cast<ModelMakerPrivate *>(this->dataPtr);

  dPtr->modelVisual->SetWorldPosition(_pos);
}
