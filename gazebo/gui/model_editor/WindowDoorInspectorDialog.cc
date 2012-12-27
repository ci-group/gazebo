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

#include "gui/model_editor/WindowDoorInspectorDialog.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
WindowDoorInspectorDialog::WindowDoorInspectorDialog(int _type,
  QWidget *_parent) : QDialog(_parent)
{
  this->setObjectName("windowDoorInspectorDialog");

  this->modelType = _type;

  if (this->modelType == WINDOW)
    this->modelTypeStr = "Window";
  else if (this->modelType == DOOR)
    this->modelTypeStr = "Door";

  std::string titleStr = modelTypeStr + " Inspector";
  this->setWindowTitle(tr(titleStr.c_str()));

  std::string modelLabelStr = modelTypeStr + " Name:";
  QLabel *modelLabel = new QLabel(tr(modelLabelStr.c_str()));
  this->modelNameLabel = new QLabel(tr(""));

  QHBoxLayout *nameLayout = new QHBoxLayout;
  nameLayout->addWidget(modelLabel);
  nameLayout->addWidget(modelNameLabel);

  QLabel *widthLabel = new QLabel(tr("Width: "));
  QLabel *depthLabel = new QLabel(tr("Depth: "));
  QLabel *heightLabel = new QLabel(tr("Height: "));

  this->widthSpinBox = new QDoubleSpinBox;
  this->widthSpinBox->setRange(-1000, 1000);
  this->widthSpinBox->setSingleStep(0.001);
  this->widthSpinBox->setDecimals(3);
  this->widthSpinBox->setValue(0.000);

  this->depthSpinBox = new QDoubleSpinBox;
  this->depthSpinBox->setRange(-1000, 1000);
  this->depthSpinBox->setSingleStep(0.001);
  this->depthSpinBox->setDecimals(3);
  this->depthSpinBox->setValue(0.000);

  this->heightSpinBox = new QDoubleSpinBox;
  this->heightSpinBox->setRange(-1000, 1000);
  this->heightSpinBox->setSingleStep(0.001);
  this->heightSpinBox->setDecimals(3);
  this->heightSpinBox->setValue(0.000);

  QGridLayout *sizeLayout = new QGridLayout;
  sizeLayout->addWidget(widthLabel, 0, 0);
  sizeLayout->addWidget(widthSpinBox, 0, 1);
  sizeLayout->addWidget(depthLabel, 1, 0);
  sizeLayout->addWidget(depthSpinBox, 1, 1);
  sizeLayout->addWidget(heightLabel, 2, 0);
  sizeLayout->addWidget(heightSpinBox, 2, 1);

  QGroupBox *sizeGroupBox = new QGroupBox(tr("Size"));
  sizeGroupBox->setLayout(sizeLayout);

  QLabel *positionXLabel = new QLabel(tr("x: "));
  QLabel *positionYLabel = new QLabel(tr("y: "));
  QLabel *elevationLabel = new QLabel(tr("Elevation: "));

  this->positionXSpinBox = new QDoubleSpinBox;
  this->positionXSpinBox->setRange(-1000, 1000);
  this->positionXSpinBox->setSingleStep(0.001);
  this->positionXSpinBox->setDecimals(3);
  this->positionXSpinBox->setValue(0.000);

  this->positionYSpinBox = new QDoubleSpinBox;
  this->positionYSpinBox->setRange(-1000, 1000);
  this->positionYSpinBox->setSingleStep(0.001);
  this->positionYSpinBox->setDecimals(3);
  this->positionYSpinBox->setValue(0.000);

  this->elevationSpinBox = new QDoubleSpinBox;
  this->elevationSpinBox->setRange(-1000, 1000);
  this->elevationSpinBox->setSingleStep(0.001);
  this->elevationSpinBox->setDecimals(3);
  this->elevationSpinBox->setValue(0.000);

  QHBoxLayout *positionLayout = new QHBoxLayout;
  positionLayout->addWidget(positionXLabel);
  positionLayout->addWidget(positionXSpinBox);
  positionLayout->addWidget(positionYLabel);
  positionLayout->addWidget(positionYSpinBox);
  positionLayout->addWidget(elevationLabel);
  positionLayout->addWidget(elevationSpinBox);

  QGroupBox *positionGroupBox = new QGroupBox(tr("Position"));
  positionGroupBox->setLayout(positionLayout);

  QLabel *typeLabel = new QLabel(tr("Type: "));
  this->typeComboBox = new QComboBox;
  this->typeComboBox->addItem(QString("Single"));

  QHBoxLayout *typeLayout = new QHBoxLayout;
  typeLayout->addWidget(typeLabel);
  typeLayout->addWidget(typeComboBox);

  QHBoxLayout *buttonsLayout = new QHBoxLayout;
  QPushButton *cancelButton = new QPushButton(tr("&Cancel"));
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(OnCancel()));
  QPushButton *OKButton = new QPushButton(tr("&OK"));
  OKButton->setDefault(true);
  connect(OKButton, SIGNAL(clicked()), this, SLOT(OnOK()));
  buttonsLayout->addWidget(cancelButton);
  buttonsLayout->addWidget(OKButton);
  buttonsLayout->setAlignment(Qt::AlignRight);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addLayout(nameLayout);
  mainLayout->addWidget(sizeGroupBox);
  mainLayout->addWidget(positionGroupBox);
  mainLayout->addLayout(typeLayout);
  mainLayout->addLayout(buttonsLayout);

  this->setLayout(mainLayout);
}

/////////////////////////////////////////////////
WindowDoorInspectorDialog::~WindowDoorInspectorDialog()
{
}

/////////////////////////////////////////////////
double WindowDoorInspectorDialog::GetWidth()
{
  return this->widthSpinBox->value();
}

/////////////////////////////////////////////////
double WindowDoorInspectorDialog::GetDepth()
{
  return this->depthSpinBox->value();
}

/////////////////////////////////////////////////
double WindowDoorInspectorDialog::GetHeight()
{
  return this->heightSpinBox->value();
}

/////////////////////////////////////////////////
QPointF WindowDoorInspectorDialog::GetPosition()
{
  return QPointF(this->positionXSpinBox->value(),
      this->positionYSpinBox->value());
}

/////////////////////////////////////////////////
double WindowDoorInspectorDialog::GetElevation()
{
  return this->elevationSpinBox->value();
}

/////////////////////////////////////////////////
std::string WindowDoorInspectorDialog::GetType()
{
  return this->typeComboBox->currentText().toStdString();
}

/////////////////////////////////////////////////
void WindowDoorInspectorDialog::SetWidth(double _width)
{
  this->widthSpinBox->setValue(_width);
}

/////////////////////////////////////////////////
void WindowDoorInspectorDialog::SetHeight(double _height)
{
  this->heightSpinBox->setValue(_height);
}

/////////////////////////////////////////////////
void WindowDoorInspectorDialog::SetDepth(double _depth)
{
  this->depthSpinBox->setValue(_depth);
}

/////////////////////////////////////////////////
void WindowDoorInspectorDialog::SetPosition(QPointF _pos)
{
  this->positionXSpinBox->setValue(_pos.x());
  this->positionYSpinBox->setValue(_pos.y());
}

/////////////////////////////////////////////////
void WindowDoorInspectorDialog::SetElevation(double _elevation)
{
  this->elevationSpinBox->setValue(_elevation);
}

/////////////////////////////////////////////////
void WindowDoorInspectorDialog::SetType(std::string _type)
{
  int index = this->typeComboBox->findText(
    QString::fromStdString(_type));
  this->typeComboBox->setCurrentIndex(index);
}

/////////////////////////////////////////////////
void WindowDoorInspectorDialog::OnCancel()
{
  this->close();
}

/////////////////////////////////////////////////
void WindowDoorInspectorDialog::OnOK()
{
  /// TODO:
  this->accept();
}
