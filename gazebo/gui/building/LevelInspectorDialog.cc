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

#include "gazebo/gui/building/LevelInspectorDialog.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
LevelInspectorDialog::LevelInspectorDialog(QWidget *_parent) : QDialog(_parent)
{
  this->setObjectName("levelInspectorDialog");
  this->setWindowTitle(tr("Level Inspector"));

  QLabel *levelLabel = new QLabel(tr("Level Name: "));
  this->levelNameLineEdit = new QLineEdit;
  this->levelNameLineEdit->setPlaceholderText(tr("Level X"));

  /// TODO add the widgets back in after the functions is implemented
/*  QLabel *floorThicknessLabel = new QLabel(tr("Floor Thickness: "));
  this->floorThicknessSpinBox = new QDoubleSpinBox;
  this->floorThicknessSpinBox->setRange(-1000, 1000);
  this->floorThicknessSpinBox->setSingleStep(0.001);
  this->floorThicknessSpinBox->setDecimals(3);
  this->floorThicknessSpinBox->setValue(0.000);

  QLabel *heightLabel = new QLabel(tr("Height: "));
  this->heightSpinBox = new QDoubleSpinBox;
  this->heightSpinBox->setRange(-1000, 1000);
  this->heightSpinBox->setSingleStep(0.001);
  this->heightSpinBox->setDecimals(3);
  this->heightSpinBox->setValue(0.000);

  QLabel *materialLabel = new QLabel(tr("Floor Material: "));
  this->materialComboBox = new QComboBox;
  this->materialComboBox->addItem(QString("Hardwood"));*/

  QGridLayout *levelLayout = new QGridLayout;
  levelLayout->addWidget(levelLabel, 0, 0);
  levelLayout->addWidget(this->levelNameLineEdit, 0, 1);
/*  levelLayout->addWidget(floorThicknessLabel, 1, 0);
  levelLayout->addWidget(this->floorThicknessSpinBox, 1, 1);
  levelLayout->addWidget(heightLabel, 2, 0);
  levelLayout->addWidget(this->heightSpinBox, 2, 1);
  levelLayout->addWidget(materialLabel, 3, 0);
  levelLayout->addWidget(this->materialComboBox, 3, 1);*/

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
  mainLayout->addLayout(levelLayout);
  mainLayout->addLayout(buttonsLayout);

  this->setLayout(mainLayout);
}

/////////////////////////////////////////////////
LevelInspectorDialog::~LevelInspectorDialog()
{
}

/////////////////////////////////////////////////
std::string LevelInspectorDialog::GetLevelName() const
{
  return this->levelNameLineEdit->text().toStdString();
}

/////////////////////////////////////////////////
double LevelInspectorDialog::GetHeight() const
{
  return this->heightSpinBox->value();
}

/////////////////////////////////////////////////
void LevelInspectorDialog::SetLevelName(const std::string &_levelName)
{
  this->levelNameLineEdit->setText(QString(_levelName.c_str()));
}


/////////////////////////////////////////////////
void LevelInspectorDialog::SetHeight(double _height)
{
  this->heightSpinBox->setValue(_height);
}

/////////////////////////////////////////////////
void LevelInspectorDialog::OnCancel()
{
  this->close();
}

/////////////////////////////////////////////////
void LevelInspectorDialog::OnApply()
{
  emit Applied();
}

/////////////////////////////////////////////////
void LevelInspectorDialog::OnOK()
{
  emit Applied();
  this->accept();
}
