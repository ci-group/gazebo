/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
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

#include "gazebo/transport/transport.hh"
#include "gazebo/rendering/UserCamera.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/Visual.hh"

#include "gazebo/gui/KeyEventHandler.hh"
#include "gazebo/gui/GuiEvents.hh"
#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/GuiIface.hh"
#include "gazebo/gui/ModelRightMenu.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
ModelRightMenu::ModelRightMenu()
{
  KeyEventHandler::Instance()->AddReleaseFilter("ModelRightMenu",
        boost::bind(&ModelRightMenu::OnKeyRelease, this, _1));

  this->moveToAct = new QAction(tr("Move To"), this);
  this->moveToAct->setStatusTip(tr("Move camera to the selection"));
  connect(this->moveToAct, SIGNAL(triggered()), this, SLOT(OnMoveTo()));

  this->followAct = new QAction(tr("Follow"), this);
  this->followAct->setStatusTip(tr("Follow the selection"));
  connect(this->followAct, SIGNAL(triggered()), this, SLOT(OnFollow()));


  // \todo Reimplement
  // this->snapBelowAct = new QAction(tr("Snap"), this);
  // this->snapBelowAct->setStatusTip(tr("Snap to object below"));
  // connect(this->snapBelowAct, SIGNAL(triggered()), this,
  //          SLOT(OnSnapBelow()));

  // Create the delete action
  g_deleteAct = new DeleteAction(tr("Delete"), this);
  g_deleteAct->setStatusTip(tr("Delete a model"));
  connect(g_deleteAct, SIGNAL(DeleteSignal(const std::string &)), this,
          SLOT(OnDelete(const std::string &)));
  connect(g_deleteAct, SIGNAL(triggered()), this,
          SLOT(OnDelete()));

  ViewState *state = new ViewState(this, "set_transparent", "set_opaque");
  state->action = new QAction(tr("Transparent"), this);
  state->action->setStatusTip(tr("Make model transparent"));
  state->action->setCheckable(true);
  connect(state->action, SIGNAL(triggered()), state, SLOT(Callback()));
  this->viewStates.push_back(state);

  state = new ViewState(this, "set_wireframe", "set_solid");
  state->action = new QAction(tr("Wireframe"), this);
  state->action->setStatusTip(tr("Wireframe mode"));
  state->action->setCheckable(true);
  connect(state->action, SIGNAL(triggered()), state, SLOT(Callback()));
  this->viewStates.push_back(state);

  state = new ViewState(this, "show_collision", "hide_collision");
  state->action = new QAction(tr("Collisions"), this);
  state->action->setStatusTip(tr("Show collision objects"));
  state->action->setCheckable(true);
  connect(state->action, SIGNAL(triggered()), state, SLOT(Callback()));
  this->viewStates.push_back(state);

  state = new ViewState(this, "show_joints", "hide_joints");
  state->action = new QAction(tr("Joints"), this);
  state->action->setStatusTip(tr("Show joints"));
  state->action->setCheckable(true);
  connect(state->action, SIGNAL(triggered()), state, SLOT(Callback()));
  this->viewStates.push_back(state);

  state = new ViewState(this, "show_com", "hide_com");
  state->action = new QAction(tr("Center of mass"), this);
  state->action->setStatusTip(tr("Show center of mass"));
  state->action->setCheckable(true);
  connect(state->action, SIGNAL(triggered()), state, SLOT(Callback()));
  this->viewStates.push_back(state);

  // \todo Reimplement
  // this->skeletonAction = new QAction(tr("Skeleton"), this);
  // this->skeletonAction->setStatusTip(tr("Show model skeleton"));
  // this->skeletonAction->setCheckable(true);
  // connect(this->skeletonAction, SIGNAL(triggered()), this,
  //         SLOT(OnSkeleton()));
}

//////////////////////////////////////////////////
bool ModelRightMenu::Init()
{
  this->node = transport::NodePtr(new transport::Node());
  this->node->Init();
  this->requestSub = this->node->Subscribe("~/request",
      &ModelRightMenu::OnRequest, this);

  return true;
}

/////////////////////////////////////////////////
bool ModelRightMenu::OnKeyRelease(const common::KeyEvent &_event)
{
  if (_event.key == Qt::Key_Escape)
  {
    rendering::UserCameraPtr cam = gui::get_active_camera();
    cam->TrackVisual("");
    gui::Events::follow("");
  }

  return false;
}

/////////////////////////////////////////////////
ModelRightMenu::~ModelRightMenu()
{
  this->node->Fini();
}

/////////////////////////////////////////////////
void ModelRightMenu::Run(const std::string &_modelName, const QPoint &_pt)
{
  this->modelName = _modelName.substr(0, _modelName.find("::"));

  QMenu menu;
  menu.addAction(this->moveToAct);
  menu.addAction(this->followAct);

  // menu.addAction(this->snapBelowAct);

  // Create the view menu
  QMenu *viewMenu = menu.addMenu(tr("View"));
  for (std::vector<ViewState*>::iterator iter = this->viewStates.begin();
       iter != this->viewStates.end(); ++iter)
  {
    viewMenu->addAction((*iter)->action);

    std::map<std::string, bool>::iterator modelIter =
      (*iter)->modelStates.find(this->modelName);

    if (modelIter == (*iter)->modelStates.end())
      (*iter)->action->setChecked((*iter)->globalEnable);
    else
      (*iter)->action->setChecked(modelIter->second);
  }

  if (g_copyAct && g_pasteAct)
  {
    menu.addSeparator();
    menu.addAction(g_copyAct);
    menu.addAction(g_pasteAct);
  }

  menu.addSeparator();
  menu.addAction(g_deleteAct);

  // \todo Reimplement these features.
  // menu.addAction(this->skeletonAction);

  menu.exec(_pt);
}

/////////////////////////////////////////////////
void ModelRightMenu::OnMoveTo()
{
  rendering::UserCameraPtr cam = gui::get_active_camera();
  cam->MoveToVisual(this->modelName);
}

/////////////////////////////////////////////////
void ModelRightMenu::OnFollow()
{
  rendering::UserCameraPtr cam = gui::get_active_camera();
  cam->TrackVisual(this->modelName);
  gui::Events::follow(this->modelName);
}

/////////////////////////////////////////////////
// void ModelRightMenu::OnSnapBelow()
// {
//   rendering::UserCameraPtr cam = gui::get_active_camera();
//   if (!cam)
//     gzerr << "Invalid user camera\n";
//
//   if (!cam->GetScene())
//     gzerr << "Invalid user camera scene\n";
//
//   // cam->GetScene()->SnapVisualToNearestBelow(this->modelName);
// }

/////////////////////////////////////////////////
void ModelRightMenu::OnDelete(const std::string &_name)
{
  std::string name = _name;
  if (name.empty())
    name = this->modelName;

  // Delete the entity
  if (!name.empty())
    transport::requestNoReply(this->node, "entity_delete", name);
}

/////////////////////////////////////////////////
void ModelRightMenu::OnRequest(ConstRequestPtr &_msg)
{
  // Process the request by looking at all the view states.
  for (std::vector<ViewState*>::iterator iter = this->viewStates.begin();
       iter != this->viewStates.end(); ++iter)
  {
    // Only proceed if the request matches one of the check or uncheck
    // requests of the view state
    if (_msg->request() == (*iter)->checkRequest ||
        _msg->request() == (*iter)->uncheckRequest)
    {
      // Determine the value(state) of the view states
      bool value = _msg->request() == (*iter)->checkRequest ? true : false;

      // If the request is for all objects...
      if (_msg->data() == "all")
      {
        // Set all model states within the view state to the value.
        for (std::map<std::string, bool>::iterator modelIter =
            (*iter)->modelStates.begin();
            modelIter != (*iter)->modelStates.end(); ++modelIter)
        {
          modelIter->second = value;
        }

        // Use a globalEnable to handle the case when new models are added
        (*iter)->globalEnable = value;
      }
      // Otherwise the request is for a single model...
      else
      {
        // Set the state of the given model
        (*iter)->modelStates[_msg->data()] = value;
      }
    }
  }
}

/////////////////////////////////////////////////
ViewState::ViewState(ModelRightMenu *_parent,
                     const std::string &_checkRequest,
                     const std::string &_uncheckRequest)
  : QObject(_parent)
{
  this->globalEnable = false;
  this->action = NULL;
  this->parent = _parent;
  this->checkRequest = _checkRequest;
  this->uncheckRequest = _uncheckRequest;
}

/////////////////////////////////////////////////
void ViewState::Callback()
{
  // Store the check state for the model
  this->modelStates[this->parent->modelName] = this->action->isChecked();

  // Send a message with the new check state. The Scene listens to these
  // messages and updates the visualizations accordingly.
  if (this->action->isChecked())
  {
    transport::requestNoReply(this->parent->node, this->checkRequest,
                              this->parent->modelName);
  }
  else
  {
    transport::requestNoReply(this->parent->node, this->uncheckRequest,
                              this->parent->modelName);
  }
}

/// \todo Reimplement these functions.
/////////////////////////////////////////////////
// void ModelRightMenu::OnSkeleton()
// {
//   this->skeletonActionState[this->modelName] =
//     this->skeletonAction->isChecked();
//
//   if (this->skeletonAction->isChecked())
//   {
//     this->requestMsg = msgs::CreateRequest("show_skeleton", this->modelName);
//     this->requestMsg->set_dbl_data(1.0);
//   }
//   else
//   {
//     this->requestMsg = msgs::CreateRequest("show_skeleton", this->modelName);
//     this->requestMsg->set_dbl_data(0.0);
//   }
//
//   this->requestPub->Publish(*this->requestMsg);
// }
