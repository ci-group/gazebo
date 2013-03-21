/*
 * Copyright 2012 Open Source Robotics Foundation
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
#include "gazebo/gui/building/EditorEvents.hh"
#include "gazebo/gui/building/BuildingEditorPalette.hh"
#include "gazebo/gui/MainWindow.hh"
#include "gazebo/gui/RenderWidget.hh"
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

  QAction *saveAct = new QAction(tr("&Save (As)"), this->mainWindow);
  saveAct->setStatusTip(tr("Save (As)"));
  saveAct->setShortcut(tr("Ctrl+S"));
  saveAct->setCheckable(false);
  connect(saveAct, SIGNAL(triggered()), this, SLOT(Save()));

  QAction *discardAct = new QAction(tr("&Discard"), this->mainWindow);
  discardAct->setStatusTip(tr("Discard"));
  discardAct->setShortcut(tr("Ctrl+D"));
  discardAct->setCheckable(false);
  connect(discardAct, SIGNAL(triggered()), this, SLOT(Discard()));

  QAction *doneAct = new QAction(tr("Don&e"), this->mainWindow);
  doneAct->setShortcut(tr("Ctrl+E"));
  doneAct->setStatusTip(tr("Done"));
  doneAct->setCheckable(false);
  connect(doneAct, SIGNAL(triggered()), this, SLOT(Done()));

  QAction *exitAct = new QAction(tr("E&xit Building Editor"), this->mainWindow);
  exitAct->setStatusTip(tr("Exit Building Editor"));
  exitAct->setShortcut(tr("Ctrl+X"));
  exitAct->setCheckable(false);
  connect(exitAct, SIGNAL(triggered()), this, SLOT(Exit()));

  this->menuBar = new QMenuBar;
  this->menuBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QMenu *fileMenu = this->menuBar->addMenu(tr("&File"));
  fileMenu->addAction(saveAct);
  fileMenu->addAction(discardAct);
  fileMenu->addAction(doneAct);
  fileMenu->addAction(exitAct);

  connect(g_editBuildingAct, SIGNAL(toggled(bool)), this, SLOT(OnEdit(bool)));

  this->connections.push_back(
      gui::editor::Events::ConnectFinishBuildingModel(
      boost::bind(&BuildingEditor::OnFinish, this)));
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
void BuildingEditor::OnEdit(bool _checked)
{
  if (_checked)
  {
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
