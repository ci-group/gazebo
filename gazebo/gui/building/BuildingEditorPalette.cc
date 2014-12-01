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
 * Brick-wall, Window and Door designed by Juan Pablo Bravo from the
 * thenounproject.com
 * Stairs designed by Brian Oppenlander from the thenounproject.com
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

  this->buildingDefaultName = "Building Default Name";
  this->currentMode = std::string();

  QVBoxLayout *mainLayout = new QVBoxLayout;

  // Model name layout
  QHBoxLayout *modelNameLayout = new QHBoxLayout;
  QLabel *modelLabel = new QLabel(tr("Model: "));
  this->modelNameEdit = new QLineEdit();
  this->modelNameEdit->setText(tr(this->buildingDefaultName.c_str()));
  modelNameLayout->addWidget(modelLabel);
  modelNameLayout->addWidget(this->modelNameEdit);
  connect(this->modelNameEdit, SIGNAL(textChanged(QString)), this,
          SLOT(OnNameChanged(QString)));

  QSize toolButtonSize(100, 100);
  QSize iconSize(65, 65);

  // Walls label
  QLabel *wallsLabel = new QLabel(tr(
       "<font size=4 color='white'>Create Walls</font>"));

  // Wall button
  QToolButton *wallButton = new QToolButton(this);
  wallButton->setFixedSize(toolButtonSize);
  wallButton->setCheckable(true);
  wallButton->setChecked(false);
  wallButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  wallButton->setIcon(QPixmap(":/images/wall.svg"));
  wallButton->setText("Wall");
  wallButton->setIconSize(QSize(iconSize));
  wallButton->setToolTip("Hold Shift to snap while drawing");
  connect(wallButton, SIGNAL(clicked()), this, SLOT(OnDrawWall()));

  // Features label
  QLabel *featuresLabel = new QLabel(tr(
       "<font size=4 color='white'>Add Features</font>"));

  // Window button
  QToolButton *windowButton = new QToolButton(this);
  windowButton->setFixedSize(toolButtonSize);
  windowButton->setCheckable(true);
  windowButton->setChecked(false);
  windowButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  windowButton->setIcon(QPixmap(":/images/window.svg"));
  windowButton->setText("Window");
  windowButton->setIconSize(QSize(iconSize));
  connect(windowButton, SIGNAL(clicked()), this, SLOT(OnAddWindow()));

  // Door button
  QToolButton *doorButton = new QToolButton(this);
  doorButton->setFixedSize(toolButtonSize);
  doorButton->setCheckable(true);
  doorButton->setChecked(false);
  doorButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  doorButton->setIcon(QPixmap(":/images/door.svg"));
  doorButton->setText("Door");
  doorButton->setIconSize(QSize(iconSize));
  connect(doorButton, SIGNAL(clicked()), this, SLOT(OnAddDoor()));

  // Stairs button
  QToolButton *stairsButton = new QToolButton(this);
  stairsButton->setFixedSize(toolButtonSize);
  stairsButton->setCheckable(true);
  stairsButton->setChecked(false);
  stairsButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  stairsButton->setIcon(QPixmap(":/images/stairs.svg"));
  stairsButton->setText("Stairs");
  stairsButton->setIconSize(QSize(iconSize));
  connect(stairsButton, SIGNAL(clicked()), this, SLOT(OnAddStair()));

  // Features layout
  QGridLayout *featuresLayout = new QGridLayout;
  featuresLayout->addWidget(windowButton, 0, 0);
  featuresLayout->addWidget(doorButton, 0, 1);
  featuresLayout->addWidget(stairsButton, 1, 0);

  // Import button
  QPushButton *importImageButton = new QPushButton(tr("Import"),
      this);
  importImageButton->setCheckable(true);
  importImageButton->setChecked(false);
  importImageButton->setToolTip(tr(
      "Import an existing floor plan to use as a guide"));
  connect(importImageButton, SIGNAL(clicked()), this, SLOT(OnImportImage()));

  QHBoxLayout *buttonsLayout = new QHBoxLayout;
  buttonsLayout->addWidget(importImageButton);
  buttonsLayout->setAlignment(Qt::AlignHCenter);
  buttonsLayout->setContentsMargins(30, 11, 30, 11);

  // Main layout
  mainLayout->addLayout(modelNameLayout);
  mainLayout->addItem(new QSpacerItem(10, 20, QSizePolicy::Expanding,
                      QSizePolicy::Minimum));
  mainLayout->addWidget(wallsLabel);
  mainLayout->addWidget(wallButton);
  mainLayout->addWidget(featuresLabel);
  mainLayout->addLayout(featuresLayout);
  mainLayout->addItem(new QSpacerItem(10, 20, QSizePolicy::Expanding,
                      QSizePolicy::Minimum));
  mainLayout->addLayout(buttonsLayout);
  mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

  this->setLayout(mainLayout);

  // Connections
  this->connections.push_back(
      gui::editor::Events::ConnectSaveBuildingModel(
      boost::bind(&BuildingEditorPalette::OnSaveModel, this, _1, _2)));

  this->connections.push_back(
      gui::editor::Events::ConnectNewBuildingModel(
      boost::bind(&BuildingEditorPalette::OnNewModel, this)));

  this->connections.push_back(
      gui::editor::Events::ConnectCreateBuildingEditorItem(
    boost::bind(&BuildingEditorPalette::OnCreateEditorItem, this, _1)));

  // Brushes (button group)
  brushes = new QButtonGroup();
  brushes->addButton(wallButton);
  brushes->addButton(windowButton);
  brushes->addButton(doorButton);
  brushes->addButton(stairsButton);
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
void BuildingEditorPalette::OnNewModel()
{
  this->modelNameEdit->setText(tr(this->buildingDefaultName.c_str()));
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnSaveModel(const std::string &_saveName,
    const std::string &/*_saveLocation*/)
{
  this->modelNameEdit->setText(tr(_saveName.c_str()));
}

/////////////////////////////////////////////////
void BuildingEditorPalette::OnNameChanged(const QString &_name)
{
  gui::editor::Events::buildingNameChanged(_name.toStdString());
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
