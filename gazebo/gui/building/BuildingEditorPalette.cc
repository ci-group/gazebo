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

#include "gazebo/gui/building/BuildingEditorPalette.hh"
#include "gazebo/gui/building/BuildingEditorEvents.hh"
#include "gazebo/gui/building/ImportImageDialog.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
BuildingEditorPalette::BuildingEditorPalette(QWidget *_parent)
    : QWidget(_parent)
{
  this->setObjectName("buildingEditorPalette");

  this->buildingDefaultName = "BuildingDefaultName";
  this->currentMode = std::string();

  QVBoxLayout *mainLayout = new QVBoxLayout;

  QHBoxLayout *modelNameLayout = new QHBoxLayout;
  QLabel *modelLabel = new QLabel(tr("Model: "));
  this->modelNameEdit = new QLineEdit();
  this->modelNameEdit->setText(tr(this->buildingDefaultName.c_str()));
  modelNameLayout->addWidget(modelLabel);
  modelNameLayout->addWidget(this->modelNameEdit);

  QFont underLineFont;
  underLineFont.setUnderline(true);
  underLineFont.setPointSize(14);

  // Add a wall button
  QPushButton *addWallButton = new QPushButton(tr("Add Wall"), this);
  addWallButton->setCheckable(true);
  addWallButton->setChecked(false);
  connect(addWallButton, SIGNAL(clicked()), this, SLOT(OnDrawWall()));

  // Add a window button
  QPushButton *addWindowButton = new QPushButton(tr("Add Window"), this);
  addWindowButton->setCheckable(true);
  addWindowButton->setChecked(false);
  connect(addWindowButton, SIGNAL(clicked()), this, SLOT(OnAddWindow()));

  // Add a door button
  QPushButton *addDoorButton = new QPushButton(tr("Add Door"), this);
  addDoorButton->setCheckable(true);
  addDoorButton->setChecked(false);
  connect(addDoorButton, SIGNAL(clicked()), this, SLOT(OnAddDoor()));

  // Add a stair button
  QPushButton *addStairButton = new QPushButton(tr("Add Stairs"), this);
  addStairButton->setCheckable(true);
  addStairButton->setChecked(false);
  connect(addStairButton, SIGNAL(clicked()), this, SLOT(OnAddStair()));

  // Import floorplan
  QPushButton *importImageButton = new QPushButton(tr("Import Floorplan"),
      this);
  importImageButton->setCheckable(true);
  importImageButton->setChecked(false);
  connect(importImageButton, SIGNAL(clicked()), this, SLOT(OnImportImage()));

  // Layout to hold the drawing buttons
  QGridLayout *gridLayout = new QGridLayout;
  gridLayout->addWidget(addWallButton, 0, 0);
  gridLayout->addWidget(addWindowButton, 0, 1);
  gridLayout->addWidget(addDoorButton, 1, 0);
  gridLayout->addWidget(addStairButton, 1, 1);
  gridLayout->addWidget(importImageButton, 2, 0);

  QPushButton *discardButton = new QPushButton(tr("Discard"));
  connect(discardButton, SIGNAL(clicked()), this, SLOT(OnDiscard()));

  this->saveButton = new QPushButton(tr("Save As"));
  connect(this->saveButton, SIGNAL(clicked()), this, SLOT(OnSave()));

  QPushButton *doneButton = new QPushButton(tr("Done"));
  connect(doneButton, SIGNAL(clicked()), this, SLOT(OnDone()));

  QHBoxLayout *buttonsLayout = new QHBoxLayout;
  buttonsLayout->addWidget(discardButton);
  buttonsLayout->addWidget(this->saveButton);
  buttonsLayout->addWidget(doneButton);
  buttonsLayout->setAlignment(Qt::AlignCenter);

  mainLayout->addLayout(modelNameLayout);
  mainLayout->addItem(new QSpacerItem(10, 20, QSizePolicy::Expanding,
                      QSizePolicy::Minimum));
  mainLayout->addLayout(gridLayout);
  mainLayout->addItem(new QSpacerItem(10, 20, QSizePolicy::Expanding,
                      QSizePolicy::Minimum));
  mainLayout->addLayout(buttonsLayout);
  mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

  this->setLayout(mainLayout);

  this->connections.push_back(
      gui::editor::Events::ConnectSaveBuildingModel(
      boost::bind(&BuildingEditorPalette::OnSaveModel, this, _1, _2)));

  this->connections.push_back(
      gui::editor::Events::ConnectDiscardBuildingModel(
      boost::bind(&BuildingEditorPalette::OnDiscardModel, this)));

  this->connections.push_back(
      gui::editor::Events::ConnectCreateBuildingEditorItem(
    boost::bind(&BuildingEditorPalette::OnCreateEditorItem, this, _1)));

  brushes = new QButtonGroup();
  brushes->addButton(addWallButton);
  brushes->addButton(addWindowButton);
  brushes->addButton(addDoorButton);
  brushes->addButton(addStairButton);
  brushes->addButton(importImageButton);
}

/////////////////////////////////////////////////
BuildingEditorPalette::~BuildingEditorPalette()
{
}

/////////////////////////////////////////////////
std::string BuildingEditorPalette::GetModelName() const
{
  return this->modelNameEdit->text().toStdString();
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnDrawWall()
{
  if (this->currentMode != "wall")
    gui::editor::Events::createBuildingEditorItem("wall");
  else
    gui::editor::Events::createBuildingEditorItem(std::string());
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnAddWindow()
{
  if (this->currentMode != "window")
    gui::editor::Events::createBuildingEditorItem("window");
  else
    gui::editor::Events::createBuildingEditorItem(std::string());
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnAddDoor()
{
  if (this->currentMode != "door")
    gui::editor::Events::createBuildingEditorItem("door");
  else
    gui::editor::Events::createBuildingEditorItem(std::string());
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnImportImage()
{
  if (this->currentMode != "image")
    gui::editor::Events::createBuildingEditorItem("image");
  else
    gui::editor::Events::createBuildingEditorItem(std::string());
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnAddStair()
{
  if (this->currentMode != "stairs")
    gui::editor::Events::createBuildingEditorItem("stairs");
  else
    gui::editor::Events::createBuildingEditorItem(std::string());
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnDiscard()
{
  gui::editor::Events::discardBuildingEditor();
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnSave()
{
  gui::editor::Events::saveBuildingEditor(
      this->modelNameEdit->text().toStdString());
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnDone()
{
  gui::editor::Events::doneBuildingEditor(
      this->modelNameEdit->text().toStdString());
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnDiscardModel()
{
  this->saveButton->setText("&Save As");
  this->modelNameEdit->setText(tr(this->buildingDefaultName.c_str()));
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnSaveModel(const std::string &_saveName,
    const std::string &/*_saveLocation*/)
{
  this->saveButton->setText("Save");
  this->modelNameEdit->setText(tr(_saveName.c_str()));
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnCreateEditorItem(const std::string &_mode)
{
  if (_mode.empty() || this->currentMode == _mode)
  {
    this->brushes->setExclusive(false);
    if (this->brushes->checkedButton())
      this->brushes->checkedButton()->setChecked(false);
    this->brushes->setExclusive(true);

    this->currentMode = std::string();
  }
  else
  {
    this->currentMode = _mode;
  }
}

/////////////////////////////////////////////////
void BuildingEditorPalette::mousePressEvent(QMouseEvent * /*_event*/)
{
  // Cancel draw mode
  gui::editor::Events::createBuildingEditorItem(std::string());
}
