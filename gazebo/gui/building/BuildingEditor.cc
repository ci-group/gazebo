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

#include "gazebo/gui/qt.h"
#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/MainWindow.hh"
#include "gazebo/gui/RenderWidget.hh"
#include "gazebo/gui/building/BuildingEditorEvents.hh"
#include "gazebo/gui/building/BuildingEditorPalette.hh"
#include "gazebo/gui/building/BuildingEditor.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
BuildingEditor::BuildingEditor(MainWindow *_mainWindow)
  : Editor(_mainWindow)
{
  // Create the building editor tab
  this->buildingPalette = new BuildingEditorPalette;
  this->Init("buildingEditorTab", "Building Editor", this->buildingPalette);

  this->saveAct = new QAction(tr("&Save (As)"), this->mainWindow);
  this->saveAct->setStatusTip(tr("Save (As)"));
  this->saveAct->setShortcut(tr("Ctrl+S"));
  this->saveAct->setCheckable(false);
  connect(this->saveAct, SIGNAL(triggered()), this, SLOT(Save()));

  this->discardAct = new QAction(tr("&Discard"), this->mainWindow);
  this->discardAct->setStatusTip(tr("Discard"));
  this->discardAct->setShortcut(tr("Ctrl+D"));
  this->discardAct->setCheckable(false);
  connect(this->discardAct, SIGNAL(triggered()), this, SLOT(Discard()));

  this->doneAct = new QAction(tr("Don&e"), this->mainWindow);
  this->doneAct->setShortcut(tr("Ctrl+E"));
  this->doneAct->setStatusTip(tr("Done"));
  this->doneAct->setCheckable(false);
  connect(this->doneAct, SIGNAL(triggered()), this, SLOT(Done()));

  this->exitAct = new QAction(tr("E&xit Building Editor"), this->mainWindow);
  this->exitAct->setStatusTip(tr("Exit Building Editor"));
  this->exitAct->setShortcut(tr("Ctrl+X"));
  this->exitAct->setCheckable(false);
  connect(this->exitAct, SIGNAL(triggered()), this, SLOT(Exit()));

  connect(g_editBuildingAct, SIGNAL(toggled(bool)), this, SLOT(OnEdit(bool)));

  this->connections.push_back(
      gui::editor::Events::ConnectFinishBuildingModel(
      boost::bind(&BuildingEditor::OnFinish, this)));

  this->menuBar = NULL;
}

/////////////////////////////////////////////////
BuildingEditor::~BuildingEditor()
{
}

////////////////////////////////////////////////
void BuildingEditor::Save()
{
  gui::editor::Events::saveBuildingEditor();
}

/////////////////////////////////////////////////
void BuildingEditor::Discard()
{
  gui::editor::Events::discardBuildingEditor();
}

/////////////////////////////////////////////////
void BuildingEditor::Done()
{
  gui::editor::Events::doneBuildingEditor();
}

/////////////////////////////////////////////////
void BuildingEditor::Exit()
{
  gui::editor::Events::exitBuildingEditor();
}

/////////////////////////////////////////////////
void BuildingEditor::OnFinish()
{
  g_editBuildingAct->setChecked(!g_editBuildingAct->isChecked());
  this->OnEdit(false);
}

/////////////////////////////////////////////////
void BuildingEditor::CreateMenus()
{
  delete this->menuBar;

  this->menuBar = new QMenuBar;
  this->menuBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QMenu *fileMenu = this->menuBar->addMenu(tr("&File"));
  fileMenu->addAction(this->saveAct);
  fileMenu->addAction(this->discardAct);
  fileMenu->addAction(this->doneAct);
  fileMenu->addAction(this->exitAct);
}

/////////////////////////////////////////////////
void BuildingEditor::OnEdit(bool _checked)
{
  if (_checked)
  {
    this->CreateMenus();
    this->mainWindow->Pause();
    this->mainWindow->ShowLeftColumnWidget("buildingEditorTab");
    this->mainWindow->ShowMenuBar(this->menuBar);
    this->mainWindow->GetRenderWidget()->ShowEditor(true);
  }
  else
  {
    this->mainWindow->ShowLeftColumnWidget();
    this->mainWindow->GetRenderWidget()->ShowEditor(false);
    this->mainWindow->ShowMenuBar();
    this->mainWindow->Play();
  }
}
