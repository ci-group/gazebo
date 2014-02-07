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

#include <iostream>

#include "gazebo/common/Console.hh"
#include "gazebo/gui/model/PartVisualTab.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
PartVisualTab::PartVisualTab()
{
  this->setObjectName("partVisualTab");
  QVBoxLayout *mainLayout = new QVBoxLayout;

  this->visualsTreeWidget = new QTreeWidget();
  this->visualsTreeWidget->setColumnCount(1);
  this->visualsTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  this->visualsTreeWidget->header()->hide();

  this->visualsTreeWidget->setSelectionMode(QAbstractItemView::NoSelection);
  connect(this->visualsTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
      this, SLOT(OnItemSelection(QTreeWidgetItem *, int)));

  QPushButton *addVisualButton = new QPushButton(tr("+ &Another Visual"));
  connect(addVisualButton, SIGNAL(clicked()), this, SLOT(OnAddVisual()));

  mainLayout->addWidget(this->visualsTreeWidget);
  mainLayout->addWidget(addVisualButton);
  this->setLayout(mainLayout);

  this->counter = 0;
  this->signalMapper = new QSignalMapper(this);
  this->OnAddVisual();

  connect(this->signalMapper, SIGNAL(mapped(int)),
     this, SLOT(OnRemoveVisual(int)));
}

/////////////////////////////////////////////////
PartVisualTab::~PartVisualTab()
{
}

/////////////////////////////////////////////////
void PartVisualTab::OnAddVisual()
{
  // Create a top-level tree item for the path
  std::stringstream visualIndex;
  visualIndex << "Visual " << counter;

  QTreeWidgetItem *visualItem =
    new QTreeWidgetItem(static_cast<QTreeWidgetItem*>(0));
  this->visualsTreeWidget->addTopLevelItem(visualItem);

  this->visualItems[counter] = visualItem;

  QWidget *visualItemWidget = new QWidget;
  QHBoxLayout *visualItemLayout = new QHBoxLayout;
  QLabel *visualLabel = new QLabel(QString(visualIndex.str().c_str()));

  QPushButton *removeVisualButton = new QPushButton(tr("Remove"));
  connect(removeVisualButton, SIGNAL(clicked()), this->signalMapper,
      SLOT(map()));
  this->signalMapper->setMapping(removeVisualButton, counter);

  VisualDataWidget *dataWidget = new VisualDataWidget;
  dataWidget->id = counter;
  this->dataWidgets.push_back(dataWidget);
  counter++;

  visualItemLayout->addWidget(visualLabel);
  visualItemLayout->addWidget(removeVisualButton);
  visualItemWidget->setLayout(visualItemLayout);
  this->visualsTreeWidget->setItemWidget(visualItem, 0, visualItemWidget);

  QTreeWidgetItem *visualChildItem =
    new QTreeWidgetItem(visualItem);

  QWidget *visualWidget = new QWidget;
  QVBoxLayout *visualLayout = new QVBoxLayout;

  QLabel *nameLabel = new QLabel(tr("Name:"));
  QLabel *visualNameLabel = new QLabel(tr(""));

  QLabel *geometryLabel = new QLabel(tr("Geometry:"));
  dataWidget->geometryComboBox = new QComboBox;
  dataWidget->geometryComboBox->addItem(tr("unit_box"));
  dataWidget->geometryComboBox->addItem(tr("unit_cylinder"));
  dataWidget->geometryComboBox->addItem(tr("unit_sphere"));
  // geometryComboBox->addItem(tr("custom"));

  QLabel *transparencyLabel = new QLabel(tr("Transparency:"));
  dataWidget->transparencySpinBox = new QDoubleSpinBox;
  dataWidget->transparencySpinBox->setRange(-1000, 1000);
  dataWidget->transparencySpinBox->setSingleStep(0.01);
  dataWidget->transparencySpinBox->setDecimals(3);
  dataWidget->transparencySpinBox->setValue(0.000);

  QLabel *materialLabel = new QLabel(tr("Material:"));
  dataWidget->materialLineEdit = new QLineEdit;

  QGridLayout *visualGeneralLayout = new QGridLayout;
  visualGeneralLayout->addWidget(nameLabel, 0, 0);
  visualGeneralLayout->addWidget(visualNameLabel, 0, 1);
  visualGeneralLayout->addWidget(geometryLabel, 1, 0);
  visualGeneralLayout->addWidget(dataWidget->geometryComboBox, 1, 1);
  visualGeneralLayout->addWidget(transparencyLabel, 2, 0);
  visualGeneralLayout->addWidget(dataWidget->transparencySpinBox, 2, 1);
  visualGeneralLayout->addWidget(materialLabel, 3, 0);
  visualGeneralLayout->addWidget(dataWidget->materialLineEdit, 3, 1);

  QLabel *posXLabel = new QLabel(tr("x: "));
  QLabel *posYLabel = new QLabel(tr("y: "));
  QLabel *posZLabel = new QLabel(tr("z: "));
  QLabel *rotRLabel = new QLabel(tr("roll: "));
  QLabel *rotPLabel = new QLabel(tr("pitch: "));
  QLabel *rotYLabel = new QLabel(tr("yaw: "));

  dataWidget->posXSpinBox = new QDoubleSpinBox;
  dataWidget->posXSpinBox->setRange(-1000, 1000);
  dataWidget->posXSpinBox->setSingleStep(0.01);
  dataWidget->posXSpinBox->setDecimals(3);
  dataWidget->posXSpinBox->setValue(0.000);

  dataWidget->posYSpinBox = new QDoubleSpinBox;
  dataWidget->posYSpinBox->setRange(-1000, 1000);
  dataWidget->posYSpinBox->setSingleStep(0.01);
  dataWidget->posYSpinBox->setDecimals(3);
  dataWidget->posYSpinBox->setValue(0.000);

  dataWidget->posZSpinBox = new QDoubleSpinBox;
  dataWidget->posZSpinBox->setRange(-1000, 1000);
  dataWidget->posZSpinBox->setSingleStep(0.01);
  dataWidget->posZSpinBox->setDecimals(3);
  dataWidget->posZSpinBox->setValue(0.000);

  dataWidget->rotRSpinBox = new QDoubleSpinBox;
  dataWidget->rotRSpinBox->setRange(-1000, 1000);
  dataWidget->rotRSpinBox->setSingleStep(0.01);
  dataWidget->rotRSpinBox->setDecimals(3);
  dataWidget->rotRSpinBox->setValue(0.000);

  dataWidget->rotPSpinBox = new QDoubleSpinBox;
  dataWidget->rotPSpinBox->setRange(-1000, 1000);
  dataWidget->rotPSpinBox->setSingleStep(0.01);
  dataWidget->rotPSpinBox->setDecimals(3);
  dataWidget->rotPSpinBox->setValue(0.000);

  dataWidget->rotYSpinBox = new QDoubleSpinBox;
  dataWidget->rotYSpinBox->setRange(-1000, 1000);
  dataWidget->rotYSpinBox->setSingleStep(0.01);
  dataWidget->rotYSpinBox->setDecimals(3);
  dataWidget->rotYSpinBox->setValue(0.000);

  QGridLayout *poseGroupLayout = new QGridLayout;
  poseGroupLayout->addWidget(posXLabel, 0, 0);
  poseGroupLayout->addWidget(dataWidget->posXSpinBox, 0, 1);
  poseGroupLayout->addWidget(posYLabel, 0, 2);
  poseGroupLayout->addWidget(dataWidget->posYSpinBox, 0, 3);
  poseGroupLayout->addWidget(posZLabel, 0, 4);
  poseGroupLayout->addWidget(dataWidget->posZSpinBox, 0, 5);
  poseGroupLayout->addWidget(rotRLabel, 1, 0);
  poseGroupLayout->addWidget(dataWidget->rotRSpinBox, 1, 1);
  poseGroupLayout->addWidget(rotPLabel, 1, 2);
  poseGroupLayout->addWidget(dataWidget->rotPSpinBox, 1, 3);
  poseGroupLayout->addWidget(rotYLabel, 1, 4);
  poseGroupLayout->addWidget(dataWidget->rotYSpinBox, 1, 5);

  poseGroupLayout->setColumnStretch(1, 1);
  poseGroupLayout->setAlignment(dataWidget->posXSpinBox, Qt::AlignLeft);
  poseGroupLayout->setAlignment(dataWidget->posYSpinBox, Qt::AlignLeft);
  poseGroupLayout->setAlignment(dataWidget->posZSpinBox, Qt::AlignLeft);
  poseGroupLayout->setAlignment(dataWidget->rotRSpinBox, Qt::AlignLeft);
  poseGroupLayout->setAlignment(dataWidget->rotPSpinBox, Qt::AlignLeft);
  poseGroupLayout->setAlignment(dataWidget->rotYSpinBox, Qt::AlignLeft);

  QGroupBox *poseGroupBox = new QGroupBox(tr("Pose"));
  poseGroupBox->setLayout(poseGroupLayout);

  visualLayout->addLayout(visualGeneralLayout);
  visualLayout->addWidget(poseGroupBox);
  visualWidget->setLayout(visualLayout);

  this->visualsTreeWidget->setItemWidget(visualChildItem, 0, visualWidget);
  visualItem->setExpanded(true);
  visualChildItem->setExpanded(true);
}

/////////////////////////////////////////////////
void PartVisualTab::OnItemSelection(QTreeWidgetItem *_item,
                                         int /*_column*/)
{
  if (_item && _item->childCount() > 0)
    _item->setExpanded(!_item->isExpanded());
}

/////////////////////////////////////////////////
void PartVisualTab::OnRemoveVisual(int _id)
{
  std::map<int, QTreeWidgetItem*>::iterator it = this->visualItems.find(_id);
  if (it == this->visualItems.end())
  {
    gzerr << "No visual item found" << std::endl;
    return;
  }
  QTreeWidgetItem *item = it->second;
  int index = this->visualsTreeWidget->indexOfTopLevelItem(item);
  this->visualsTreeWidget->takeTopLevelItem(index);

  this->visualItems.erase(it);

  for (unsigned int i = 0; i < this->dataWidgets.size(); ++i)
  {
    if (this->dataWidgets[i]->id == _id)
    {
      this->dataWidgets.erase(this->dataWidgets.begin() + i);
    }
  }
}

/////////////////////////////////////////////////
void PartVisualTab::SetPose(unsigned int _index, const math::Pose &_pose)
{
  if (_index >= this->dataWidgets.size())
  {
    gzerr << "Index is out of range" << std::endl;
  }

  this->dataWidgets[_index]->posXSpinBox->setValue(_pose.pos.x);
  this->dataWidgets[_index]->posYSpinBox->setValue(_pose.pos.y);
  this->dataWidgets[_index]->posZSpinBox->setValue(_pose.pos.z);

  this->dataWidgets[_index]->rotRSpinBox->setValue(_pose.rot.GetAsEuler().x);
  this->dataWidgets[_index]->rotPSpinBox->setValue(_pose.rot.GetAsEuler().y);
  this->dataWidgets[_index]->rotYSpinBox->setValue(_pose.rot.GetAsEuler().z);
}

/////////////////////////////////////////////////
math::Pose PartVisualTab::GetPose(unsigned int _index) const
{
  if (_index >= this->dataWidgets.size())
  {
    gzerr << "Index is out of range" << std::endl;
    return math::Pose::Zero;
  }

  return math::Pose(this->dataWidgets[_index]->posXSpinBox->value(),
      this->dataWidgets[_index]->posYSpinBox->value(),
      this->dataWidgets[_index]->posZSpinBox->value(),
      this->dataWidgets[_index]->rotRSpinBox->value(),
      this->dataWidgets[_index]->rotPSpinBox->value(),
      this->dataWidgets[_index]->rotYSpinBox->value());
}

/////////////////////////////////////////////////
void PartVisualTab::SetTransparency(unsigned int _index, double _transparency)
{
  if (_index >= this->dataWidgets.size())
  {
    gzerr << "Index is out of range" << std::endl;
  }

  this->dataWidgets[_index]->transparencySpinBox->setValue(_transparency);
}

/////////////////////////////////////////////////
double PartVisualTab::GetTransparency(unsigned int _index) const
{
  if (_index >= this->dataWidgets.size())
  {
    gzerr << "Index is out of range" << std::endl;
    return -1;
  }

  return this->dataWidgets[_index]->transparencySpinBox->value();
}

/////////////////////////////////////////////////
std::string PartVisualTab::GetMaterial(unsigned int _index) const
{
  if (_index >= this->dataWidgets.size())
  {
    gzerr << "Index is out of range" << std::endl;
    return "";
  }

  return this->dataWidgets[_index]->materialLineEdit->text().toStdString();
}

/////////////////////////////////////////////////
void PartVisualTab::SetMaterial(unsigned int _index,
    const std::string &_material)
{
  if (_index >= this->dataWidgets.size())
  {
    gzerr << "Index is out of range" << std::endl;
  }

  this->dataWidgets[_index]->materialLineEdit->setText(tr(_material.c_str()));
}


/////////////////////////////////////////////////
void PartVisualTab::SetGeometry(unsigned int _index,
    const std::string &_geometry)
{
  if (_index >= this->dataWidgets.size())
  {
    gzerr << "Index is out of range" << std::endl;
  }
  int dataIndex = this->dataWidgets[_index]->geometryComboBox->findText(
      QString(tr(_geometry.c_str())));
  if (dataIndex >= 0)
  {
    this->dataWidgets[_index]->geometryComboBox->setCurrentIndex(dataIndex);
  }
}

/////////////////////////////////////////////////
std::string PartVisualTab::GetGeometry(unsigned int _index) const
{
  if (_index >= this->dataWidgets.size())
  {
    gzerr << "Index is out of range" << std::endl;
    return NULL;
  }

  int dataIndex = this->dataWidgets[_index]->geometryComboBox->currentIndex();
  return this->dataWidgets[_index]->geometryComboBox->itemText(
      dataIndex).toStdString();
}
