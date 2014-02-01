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

#include "gazebo/gui/building/StairsInspectorDialog.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
StairsInspectorDialog::StairsInspectorDialog(QWidget *_parent)
  : QDialog(_parent)
{
  this->setObjectName("stairsInspectorDialog");

  this->setWindowTitle(tr("Stairs Inspector"));

  QLabel *stairsLabel = new QLabel(tr("Stairs Name: "));
  this->stairsNameLabel = new QLabel(tr(""));

  QHBoxLayout *nameLayout = new QHBoxLayout;
  nameLayout->addWidget(stairsLabel);
  nameLayout->addWidget(stairsNameLabel);

  QLabel *startPointLabel = new QLabel(tr("Start Point: "));

  QLabel *startXLabel = new QLabel(tr("x: "));
  QLabel *startYLabel = new QLabel(tr("y: "));

  this->startXSpinBox = new QDoubleSpinBox;
  this->startXSpinBox->setRange(-1000, 1000);
  this->startXSpinBox->setSingleStep(0.001);
  this->startXSpinBox->setDecimals(3);
  this->startXSpinBox->setValue(0.000);

  this->startYSpinBox = new QDoubleSpinBox;
  this->startYSpinBox->setRange(-1000, 1000);
  this->startYSpinBox->setSingleStep(0.001);
  this->startYSpinBox->setDecimals(3);
  this->startYSpinBox->setValue(0.000);

  QGridLayout *startXYLayout = new QGridLayout;
  startXYLayout->addWidget(startXLabel, 0, 0);
  startXYLayout->addWidget(startXSpinBox, 0, 1);
  startXYLayout->addWidget(startYLabel, 1, 0);
  startXYLayout->addWidget(startYSpinBox, 1, 1);
  startXYLayout->setColumnStretch(1, 1);
  startXYLayout->setAlignment(startXSpinBox, Qt::AlignLeft);
  startXYLayout->setAlignment(startYSpinBox, Qt::AlignLeft);

  QVBoxLayout *xyLayout = new QVBoxLayout;
  xyLayout->addWidget(startPointLabel);
  xyLayout->addLayout(startXYLayout);

  QGroupBox *positionGroupBox = new QGroupBox(tr("Position"));
  positionGroupBox->setLayout(xyLayout);

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

  QLabel *stepsLabel = new QLabel(tr("# Steps: "));
  this->stepsSpinBox = new QSpinBox;
  this->stepsSpinBox->setRange(1, 1000);
  this->stepsSpinBox->setSingleStep(1);
  this->stepsSpinBox->setValue(1);

  QGridLayout *stepsLayout = new QGridLayout;
  stepsLayout->addWidget(stepsLabel, 0, 0);
  stepsLayout->addWidget(stepsSpinBox, 0, 1);

  QVBoxLayout *sizeStepsLayout = new QVBoxLayout;
  sizeStepsLayout->addLayout(sizeLayout);
  sizeStepsLayout->addLayout(stepsLayout);

  QGroupBox *sizeGroupBox = new QGroupBox(tr("Size"));
  sizeGroupBox->setLayout(sizeStepsLayout);

  QHBoxLayout *buttonsLayout = new QHBoxLayout;
  QPushButton *cancelButton = new QPushButton(tr("&Cancel"));
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(OnCancel()));
  QPushButton *applyButton = new QPushButton(tr("&Apply"));
  connect(applyButton, SIGNAL(clicked()), this, SLOT(OnApply()));
  QPushButton *OKButton = new QPushButton(tr("&OK"));
  OKButton->setDefault(true);
  connect(OKButton, SIGNAL(clicked()), this, SLOT(OnOK()));
  buttonsLayout->addWidget(cancelButton);
  buttonsLayout->addWidget(applyButton);
  buttonsLayout->addWidget(OKButton);
  buttonsLayout->setAlignment(Qt::AlignRight);

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addLayout(nameLayout);
  mainLayout->addWidget(positionGroupBox);
  mainLayout->addWidget(sizeGroupBox);
  mainLayout->addLayout(buttonsLayout);

  this->setLayout(mainLayout);
}

/////////////////////////////////////////////////
StairsInspectorDialog::~StairsInspectorDialog()
{
}

/////////////////////////////////////////////////
QPointF StairsInspectorDialog::GetStartPosition() const
{
  return QPointF(this->startXSpinBox->value(),
      this->startYSpinBox->value());
}

/////////////////////////////////////////////////
double StairsInspectorDialog::GetWidth() const
{
  return this->widthSpinBox->value();
}

/////////////////////////////////////////////////
double StairsInspectorDialog::GetDepth() const
{
  return this->depthSpinBox->value();
}

/////////////////////////////////////////////////
double StairsInspectorDialog::GetHeight() const
{
  return this->heightSpinBox->value();
}

/////////////////////////////////////////////////
int StairsInspectorDialog::GetSteps() const
{
  return this->stepsSpinBox->value();
}

/////////////////////////////////////////////////
void StairsInspectorDialog::SetName(const std::string &_name)
{
  this->stairsNameLabel->setText(tr(_name.c_str()));
}

/////////////////////////////////////////////////
void StairsInspectorDialog::SetStartPosition(const QPointF &_pos)
{
  this->startXSpinBox->setValue(_pos.x());
  this->startYSpinBox->setValue(_pos.y());
}

/////////////////////////////////////////////////
void StairsInspectorDialog::SetWidth(double _width)
{
  this->widthSpinBox->setValue(_width);
}

/////////////////////////////////////////////////
void StairsInspectorDialog::SetDepth(double _depth)
{
  this->depthSpinBox->setValue(_depth);
}


/////////////////////////////////////////////////
void StairsInspectorDialog::SetHeight(double _height)
{
  this->heightSpinBox->setValue(_height);
}

/////////////////////////////////////////////////
void StairsInspectorDialog::SetSteps(int _steps)
{
  this->stepsSpinBox->setValue(_steps);
}

/////////////////////////////////////////////////
void StairsInspectorDialog::OnCancel()
{
  this->close();
}

/////////////////////////////////////////////////
void StairsInspectorDialog::OnApply()
{
  emit Applied();
}

/////////////////////////////////////////////////
void StairsInspectorDialog::OnOK()
{
  emit Applied();
  this->accept();
}
