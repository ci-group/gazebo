/*
 * Copyright 2015 Open Source Robotics Foundation
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

#include "gazebo/transport/Node.hh"
#include "gazebo/transport/Publisher.hh"

#include "gazebo/rendering/UserCamera.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/Visual.hh"
#include "gazebo/rendering/SelectionObj.hh"
#include "gazebo/rendering/ApplyWrenchVisual.hh"

#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/MainWindow.hh"
#include "gazebo/gui/GuiIface.hh"
#include "gazebo/gui/KeyEventHandler.hh"
#include "gazebo/gui/MouseEventHandler.hh"
#include "gazebo/gui/ApplyWrenchDialogPrivate.hh"
#include "gazebo/gui/ApplyWrenchDialog.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
ApplyWrenchDialog::ApplyWrenchDialog(QWidget *_parent)
  : QDialog(_parent), dataPtr(new ApplyWrenchDialogPrivate)
{
  this->setObjectName("ApplyWrenchDialog");
  this->dataPtr->mainWindow = gui::get_main_window();

  this->setWindowTitle(tr("Apply Force and Torque"));
  this->setWindowFlags(Qt::WindowStaysOnTopHint);
  this->setWindowModality(Qt::NonModal);
  this->setStyleSheet(
    "QPushButton {\
      border-radius: 5px;\
      border-radius: 5px;\
    }");

  this->dataPtr->modelLabel = new QLabel();

  // Links list
  QHBoxLayout *linkLayout = new QHBoxLayout();
  QLabel *linkLabel = new QLabel(tr("<b>Apply to link:<b> "));
  this->dataPtr->linksComboBox = new QComboBox();
  this->dataPtr->linksComboBox->setMinimumWidth(200);
  connect(this->dataPtr->linksComboBox, SIGNAL(currentIndexChanged(QString)),
      this, SLOT(SetLink(QString)));

  linkLayout->addWidget(linkLabel);
  linkLayout->addWidget(this->dataPtr->linksComboBox);

  // Force
  QLabel *forceLabel = new QLabel(tr(
       "<font size=4>Force</font>"));
  forceLabel->setObjectName("forceLabel");
  forceLabel->setStyleSheet(
    "QLabel#forceLabel {\
      background-color: #444;\
      border-radius: 5px;\
      padding-left: 10px;\
      min-height: 40px;\
    }");

  // Force vector layout
  QGridLayout *forceVectorLayout = new QGridLayout();

  // Force Vector
  this->dataPtr->forceXSpin = new QDoubleSpinBox();
  this->dataPtr->forceYSpin = new QDoubleSpinBox();
  this->dataPtr->forceZSpin = new QDoubleSpinBox();

  std::vector<QDoubleSpinBox *> forceSpins;
  forceSpins.push_back(this->dataPtr->forceXSpin);
  forceSpins.push_back(this->dataPtr->forceYSpin);
  forceSpins.push_back(this->dataPtr->forceZSpin);

  for (unsigned int i = 0; i < forceSpins.size(); ++i)
  {
    QLabel *forceElementLabel = new QLabel();
    if (i == 0)
      forceElementLabel->setText(tr("X:"));
    else if (i == 1)
      forceElementLabel->setText(tr("Y:"));
    else if (i == 2)
      forceElementLabel->setText(tr("Z:"));
    QLabel *forceUnitLabel = new QLabel(tr("N"));

    forceSpins[i]->setRange(-GZ_DBL_MAX, GZ_DBL_MAX);
    forceSpins[i]->setSingleStep(100);
    forceSpins[i]->setDecimals(3);
    forceSpins[i]->setValue(0);
    forceSpins[i]->setMaximumWidth(100);
    forceSpins[i]->installEventFilter(this);
    connect(forceSpins[i], SIGNAL(valueChanged(double)), this,
        SLOT(OnForceChanged(double)));

    forceVectorLayout->addWidget(forceElementLabel, i, 0, Qt::AlignRight);
    forceVectorLayout->addWidget(forceSpins[i], i, 1);
    forceVectorLayout->addWidget(forceUnitLabel, i, 2);
  }

  // Force total
  QLabel *forceMagLabel = new QLabel(tr("Mag:"));
  QLabel *forceMagUnitLabel = new QLabel(tr("N"));

  this->dataPtr->forceMagSpin = new QDoubleSpinBox();
  this->dataPtr->forceMagSpin->setRange(0, GZ_DBL_MAX);
  this->dataPtr->forceMagSpin->setSingleStep(100);
  this->dataPtr->forceMagSpin->setDecimals(3);
  this->dataPtr->forceMagSpin->setValue(0);
  this->dataPtr->forceMagSpin->setMaximumWidth(100);
  this->dataPtr->forceMagSpin->installEventFilter(this);
  connect(this->dataPtr->forceMagSpin, SIGNAL(valueChanged(double)), this,
      SLOT(OnForceMagChanged(double)));

  forceVectorLayout->addWidget(forceMagLabel, 3, 0, Qt::AlignRight);
  forceVectorLayout->addWidget(this->dataPtr->forceMagSpin, 3, 1);
  forceVectorLayout->addWidget(forceMagUnitLabel, 3, 2);

  // Clear force
  QPushButton *forceClearButton = new QPushButton(tr("Clear"));
  connect(forceClearButton, SIGNAL(clicked()), this, SLOT(OnForceClear()));
  forceVectorLayout->addWidget(forceClearButton, 4, 0, 1, 3, Qt::AlignLeft);

  // Vertical separator
  QFrame *separator = new QFrame();
  separator->setFrameShape(QFrame::VLine);
  separator->setLineWidth(10);

  // Force Position
  QLabel *forcePosLabel = new QLabel(tr("Application Point:"));
  forcePosLabel->setObjectName("forcePosLabel");
  forcePosLabel->setStyleSheet(
    "QLabel#forcePosLabel {\
      max-height: 15px;\
    }");

  // CoM
  QLabel *comLabel = new QLabel(tr("Center of mass"));

  QLabel *comPixLabel = new QLabel();
  QPixmap comPixmap(":images/com.png");
  comPixmap = comPixmap.scaled(QSize(20, 20));
  comPixLabel->setPixmap(comPixmap);
  comPixLabel->setMask(comPixmap.mask());

  QHBoxLayout *comLabelLayout = new QHBoxLayout();
  comLabelLayout->addWidget(comLabel);
  comLabelLayout->addWidget(comPixLabel);

  this->dataPtr->comRadio = new QRadioButton();
  this->dataPtr->forcePosRadio = new QRadioButton();
  this->dataPtr->comRadio->setChecked(true);
  connect(this->dataPtr->comRadio, SIGNAL(toggled(bool)), this,
      SLOT(ToggleComRadio(bool)));


  // Force Position layout
  QGridLayout *forcePosLayout = new QGridLayout();
  forcePosLayout->setContentsMargins(0, 0, 0, 0);
  forcePosLayout->addWidget(forcePosLabel, 0, 0, 1, 4, Qt::AlignLeft);
  forcePosLayout->addWidget(this->dataPtr->comRadio, 1, 0);
  forcePosLayout->addLayout(comLabelLayout, 1, 1, 1, 3, Qt::AlignLeft);
  forcePosLayout->addWidget(this->dataPtr->forcePosRadio, 2, 0);

  // Force Position Vector
  this->dataPtr->forcePosXSpin = new QDoubleSpinBox();
  this->dataPtr->forcePosYSpin = new QDoubleSpinBox();
  this->dataPtr->forcePosZSpin = new QDoubleSpinBox();

  std::vector<QDoubleSpinBox *> forcePosSpins;
  forcePosSpins.push_back(this->dataPtr->forcePosXSpin);
  forcePosSpins.push_back(this->dataPtr->forcePosYSpin);
  forcePosSpins.push_back(this->dataPtr->forcePosZSpin);

  for (unsigned int i = 0; i < forcePosSpins.size(); ++i)
  {
    QLabel *forcePosElementLabel = new QLabel();
    if (i == 0)
      forcePosElementLabel->setText(tr("X:"));
    else if (i == 1)
      forcePosElementLabel->setText(tr("Y:"));
    else if (i == 2)
      forcePosElementLabel->setText(tr("Z:"));
    QLabel *forcePosUnitLabel = new QLabel(tr("m"));

    forcePosSpins[i]->setRange(-GZ_DBL_MAX, GZ_DBL_MAX);
    forcePosSpins[i]->setSingleStep(0.1);
    forcePosSpins[i]->setDecimals(3);
    forcePosSpins[i]->setValue(0);
    forcePosSpins[i]->setMaximumWidth(100);
    forcePosSpins[i]->installEventFilter(this);
    connect(forcePosSpins[i], SIGNAL(valueChanged(double)), this,
        SLOT(OnForcePosChanged(double)));

    forcePosLayout->addWidget(forcePosElementLabel, i+2, 1, Qt::AlignRight);
    forcePosLayout->addWidget(forcePosSpins[i], i+2, 2);
    forcePosLayout->addWidget(forcePosUnitLabel, i+2, 3);
  }

  // Apply force
  QPushButton *applyForceButton = new QPushButton("Apply Force");
  connect(applyForceButton, SIGNAL(clicked()), this, SLOT(OnApplyForce()));

  // Force layout
  QGridLayout *forceLayout = new QGridLayout();
  forceLayout->setContentsMargins(0, 0, 0, 0);
  forceLayout->addWidget(forceLabel, 0, 0, 1, 5);
  forceLayout->addItem(new QSpacerItem(10, 10), 1, 0, 1, 5);
  forceLayout->addLayout(forceVectorLayout, 2, 1);
  forceLayout->addWidget(separator, 2, 2);
  forceLayout->addLayout(forcePosLayout, 2, 3);
  forceLayout->addItem(new QSpacerItem(10, 10), 3, 0, 1, 5);
  forceLayout->addWidget(applyForceButton, 4, 1, 1, 3, Qt::AlignRight);
  forceLayout->addItem(new QSpacerItem(5, 10), 5, 0);
  forceLayout->addItem(new QSpacerItem(7, 10), 5, 4);

  QFrame *forceFrame = new QFrame();
  forceFrame->setLayout(forceLayout);
  forceFrame->setObjectName("forceLayout");
  forceFrame->setFrameShape(QFrame::StyledPanel);

  forceFrame->setStyleSheet(
    "QFrame#forceLayout {\
      background-color: #666;\
      border-radius: 10px;\
    }");

  QGraphicsDropShadowEffect *forceEffect = new QGraphicsDropShadowEffect;
  forceEffect->setBlurRadius(5);
  forceEffect->setXOffset(5);
  forceEffect->setYOffset(5);
  forceEffect->setColor(Qt::black);
  forceFrame->setGraphicsEffect(forceEffect);

  // Torque
  QLabel *torqueLabel = new QLabel(tr(
       "<font size=4>Torque</font>"));
  torqueLabel->setObjectName("torqueLabel");
  torqueLabel->setStyleSheet(
    "QLabel#torqueLabel {\
      background-color: #444;\
      border-radius: 5px;\
      padding-left: 10px;\
      min-height: 40px;\
    }");

  // Torque vector layout
  QGridLayout *torqueVectorLayout = new QGridLayout();

  // Torque Vector
  this->dataPtr->torqueXSpin = new QDoubleSpinBox();
  this->dataPtr->torqueYSpin = new QDoubleSpinBox();
  this->dataPtr->torqueZSpin = new QDoubleSpinBox();

  std::vector<QDoubleSpinBox *> torqueSpins;
  torqueSpins.push_back(this->dataPtr->torqueXSpin);
  torqueSpins.push_back(this->dataPtr->torqueYSpin);
  torqueSpins.push_back(this->dataPtr->torqueZSpin);

  for (unsigned int i = 0; i < torqueSpins.size(); ++i)
  {
    QLabel *torqueElementLabel = new QLabel();
    if (i == 0)
      torqueElementLabel->setText(tr("X:"));
    else if (i == 1)
      torqueElementLabel->setText(tr("Y:"));
    else if (i == 2)
      torqueElementLabel->setText(tr("Z:"));
    QLabel *torqueUnitLabel = new QLabel(tr("Nm"));

    torqueSpins[i]->setRange(-GZ_DBL_MAX, GZ_DBL_MAX);
    torqueSpins[i]->setSingleStep(100);
    torqueSpins[i]->setDecimals(3);
    torqueSpins[i]->setValue(0);
    torqueSpins[i]->setMaximumWidth(100);
    torqueSpins[i]->installEventFilter(this);
    connect(torqueSpins[i], SIGNAL(valueChanged(double)), this,
        SLOT(OnTorqueChanged(double)));

    torqueVectorLayout->addWidget(torqueElementLabel, i, 0, Qt::AlignRight);
    torqueVectorLayout->addWidget(torqueSpins[i], i, 1);
    torqueVectorLayout->addWidget(torqueUnitLabel, i, 2);
  }

  // Torque magnitude
  QLabel *torqueMagLabel = new QLabel(tr("Mag:"));
  QLabel *torqueMagUnitLabel = new QLabel(tr("Nm"));

  this->dataPtr->torqueMagSpin = new QDoubleSpinBox();
  this->dataPtr->torqueMagSpin->setRange(0, GZ_DBL_MAX);
  this->dataPtr->torqueMagSpin->setSingleStep(100);
  this->dataPtr->torqueMagSpin->setDecimals(3);
  this->dataPtr->torqueMagSpin->setValue(0);
  this->dataPtr->torqueMagSpin->setMaximumWidth(100);
  this->dataPtr->torqueMagSpin->installEventFilter(this);
  connect(this->dataPtr->torqueMagSpin, SIGNAL(valueChanged(double)), this,
      SLOT(OnTorqueMagChanged(double)));

  torqueVectorLayout->addWidget(torqueMagLabel, 3, 0, Qt::AlignRight);
  torqueVectorLayout->addWidget(this->dataPtr->torqueMagSpin, 3, 1);
  torqueVectorLayout->addWidget(torqueMagUnitLabel, 3, 2);

  // Clear torque
  QPushButton *torqueClearButton = new QPushButton(tr("Clear"));
  connect(torqueClearButton, SIGNAL(clicked()), this, SLOT(OnTorqueClear()));
  torqueVectorLayout->addWidget(torqueClearButton, 4, 0, 1, 3, Qt::AlignLeft);

  // Apply torque
  QPushButton *applyTorqueButton = new QPushButton("Apply Torque");
  connect(applyTorqueButton, SIGNAL(clicked()), this, SLOT(OnApplyTorque()));

  // Torque layout
  QGridLayout *torqueLayout = new QGridLayout();
  torqueLayout->setContentsMargins(0, 0, 0, 0);
  torqueLayout->addWidget(torqueLabel, 0, 0, 1, 3);
  torqueLayout->addItem(new QSpacerItem(10, 10), 1, 0, 1, 3);
  torqueLayout->addLayout(torqueVectorLayout, 2, 1);
  torqueLayout->addItem(new QSpacerItem(10, 10), 3, 0, 1, 3);
  torqueLayout->addWidget(applyTorqueButton, 4, 1, 1, 1, Qt::AlignRight);
  torqueLayout->addItem(new QSpacerItem(5, 10), 5, 0);
  torqueLayout->addItem(new QSpacerItem(5, 10), 5, 2);

  QFrame *torqueFrame = new QFrame();
  torqueFrame->setLayout(torqueLayout);
  torqueFrame->setObjectName("torqueLayout");
  torqueFrame->setFrameShape(QFrame::StyledPanel);

  torqueFrame->setStyleSheet(
    "QFrame#torqueLayout {\
      background-color: #666;\
      border-radius: 10px;\
    }");

  QGraphicsDropShadowEffect *torqueEffect = new QGraphicsDropShadowEffect;
  torqueEffect->setBlurRadius(5);
  torqueEffect->setXOffset(5);
  torqueEffect->setYOffset(5);
  torqueEffect->setColor(Qt::black);
  torqueFrame->setGraphicsEffect(torqueEffect);

  // Buttons
  QPushButton *cancelButton = new QPushButton(tr("Cancel"));
  connect(cancelButton, SIGNAL(clicked()), this, SLOT(OnCancel()));

  QPushButton *applyAllButton = new QPushButton("Apply All");
  applyAllButton->setDefault(true);
  connect(applyAllButton, SIGNAL(clicked()), this, SLOT(OnApplyAll()));

  QHBoxLayout *buttonsLayout = new QHBoxLayout;
  buttonsLayout->addWidget(cancelButton);
  buttonsLayout->addWidget(applyAllButton);

  // Main layout
  QGridLayout *mainLayout = new QGridLayout();
  mainLayout->setSizeConstraint(QLayout::SetFixedSize);
  mainLayout->addWidget(this->dataPtr->modelLabel, 0, 0, 1, 2, Qt::AlignLeft);
  mainLayout->addLayout(linkLayout, 1, 0, 1, 2, Qt::AlignLeft);
  mainLayout->addWidget(forceFrame, 2, 0);
  mainLayout->addWidget(torqueFrame, 2, 1);
  mainLayout->addLayout(buttonsLayout, 3, 0, 1, 2, Qt::AlignRight);

  this->setLayout(mainLayout);

  this->dataPtr->node = transport::NodePtr(new transport::Node());
  this->dataPtr->node->Init();

  this->dataPtr->requestMsg = NULL;
  this->dataPtr->requestPub.reset();
  this->dataPtr->responseSub.reset();

  this->dataPtr->mode = "none";
  this->dataPtr->comVector = math::Vector3::Zero;
  this->dataPtr->forceVector = math::Vector3::Zero;
  this->dataPtr->torqueVector = math::Vector3::Zero;
}

/////////////////////////////////////////////////
ApplyWrenchDialog::~ApplyWrenchDialog()
{
  if (this->dataPtr->applyWrenchVisual)
    MouseEventHandler::Instance()->RemoveReleaseFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName());

  this->dataPtr->node->Fini();
  this->dataPtr->connections.clear();
  delete this->dataPtr;
  this->dataPtr = NULL;
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::Init(const std::string &_modelName,
    const std::string &_linkName)
{
  if (!this->SetModel(_modelName))
    return;

  if (!this->SetLink(_linkName))
    return;

  connect(this, SIGNAL(rejected()), this, SLOT(OnCancel()));
  connect(g_rotateAct, SIGNAL(triggered()), this, SLOT(OnManipulation()));
  connect(g_translateAct, SIGNAL(triggered()), this, SLOT(OnManipulation()));
  connect(g_scaleAct, SIGNAL(triggered()), this, SLOT(OnManipulation()));

  this->move(QCursor::pos());
  this->show();
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::Fini()
{
  this->close();
  this->dataPtr->connections.clear();
  this->dataPtr->mainWindow->removeEventFilter(this);

  if (this->dataPtr->applyWrenchVisual)
  {
    MouseEventHandler::Instance()->RemoveReleaseFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName());
    MouseEventHandler::Instance()->RemovePressFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName());
    MouseEventHandler::Instance()->RemoveMoveFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName());
    KeyEventHandler::Instance()->RemovePressFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName());
    this->dataPtr->applyWrenchVisual->Fini();
  }
  this->dataPtr->applyWrenchVisual.reset();

  this->deleteLater();
}

/////////////////////////////////////////////////
bool ApplyWrenchDialog::SetModel(const std::string &_modelName)
{
  if (!gui::get_active_camera() || !gui::get_active_camera()->GetScene())
    return false;

  rendering::VisualPtr vis = gui::get_active_camera()->GetScene()->
      GetVisual(_modelName);

  if (!vis)
  {
    gzerr << "Model [" << _modelName << "] could not be found." << std::endl;
    return false;
  }

  this->dataPtr->modelName = _modelName;

  // Check if model/link hasn't been deleted on PreRender
  this->dataPtr->connections.push_back(
      event::Events::ConnectPreRender(
      boost::bind(&ApplyWrenchDialog::OnPreRender, this)));

  this->dataPtr->modelLabel->setText(
      ("<b>Model:</b> " + _modelName).c_str());

  // Don't fire signals while inserting items
  this->dataPtr->linksComboBox->blockSignals(true);
  this->dataPtr->linksComboBox->clear();

  for (unsigned int i = 0; i < vis->GetChildCount(); ++i)
  {
    rendering::VisualPtr childVis = vis->GetChild(i);
    std::string linkName = childVis->GetName();

    // FIXME: This is failing to get real links sometimes:
    // uint32_t flags = childVis->GetVisibilityFlags();
    // if (!((flags != GZ_VISIBILITY_ALL) && (flags & GZ_VISIBILITY_GUI)))
    if (linkName.find("_GL_MANIP_") == std::string::npos)
    {
      std::string unscopedLinkName = linkName.substr(linkName.find("::") + 2);
      this->dataPtr->linksComboBox->addItem(
          QString::fromStdString(unscopedLinkName));
    }
  }
  // Sort alphabetically
  QSortFilterProxyModel *proxy = new QSortFilterProxyModel(
      this->dataPtr->linksComboBox);
  proxy->setSourceModel(this->dataPtr->linksComboBox->model());
  this->dataPtr->linksComboBox->model()->setParent(proxy);
  this->dataPtr->linksComboBox->setModel(proxy);
  this->dataPtr->linksComboBox->model()->sort(0);

  this->dataPtr->linksComboBox->blockSignals(false);

  if (this->dataPtr->linksComboBox->count() > 0)
    return true;

  gzerr << "Couldn't find links in model ' [" << _modelName << "]."
        << std::endl;

  return false;
}

/////////////////////////////////////////////////
bool ApplyWrenchDialog::SetLink(const std::string &_linkName)
{
  if (!gui::get_active_camera() || !gui::get_active_camera()->GetScene())
    return false;

  // Select on combo box
  std::string unscopedLinkName = _linkName.substr(_linkName.find("::") + 2);
  int index = -1;
  for (int i = 0; i < this->dataPtr->linksComboBox->count(); ++i)
  {
    if ((this->dataPtr->linksComboBox->itemText(i)).toStdString() ==
        unscopedLinkName)
    {
      index = i;
      break;
    }
  }
  if (index == -1)
  {
    gzerr << "Link [" << _linkName << "] could not be found in the combo box."
          << std::endl;
    return false;
  }
  this->dataPtr->linksComboBox->setCurrentIndex(index);

  // Request link message to get CoM
  this->dataPtr->requestPub.reset();
  this->dataPtr->responseSub.reset();
  this->dataPtr->requestPub = this->dataPtr->node->Advertise<msgs::Request>(
      "~/request");
  this->dataPtr->responseSub = this->dataPtr->node->Subscribe(
      "~/response", &ApplyWrenchDialog::OnResponse, this);

  this->dataPtr->requestMsg = msgs::CreateRequest("entity_info", _linkName);
  this->dataPtr->requestPub->Publish(*this->dataPtr->requestMsg);

  // Visual
  this->dataPtr->linkName = _linkName;
  rendering::VisualPtr vis = gui::get_active_camera()->GetScene()->
      GetVisual(this->dataPtr->linkName);

  if (!vis)
  {
    gzerr << "A visual named [" << this->dataPtr->linkName
          << "] could not be found." << std::endl;
    return false;
  }
  this->dataPtr->linkVisual = vis;
  this->AttachVisuals();

  // Main window
  this->dataPtr->mainWindow->installEventFilter(this);

  // MouseRelease even when it's inactive, to regain focus
  if (this->dataPtr->applyWrenchVisual)
  {
    MouseEventHandler::Instance()->AddReleaseFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName(),
        boost::bind(&ApplyWrenchDialog::OnMouseRelease, this, _1));
  }

  return true;
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::SetLink(const QString _linkName)
{
  // Remove previous link's filter
  if (this->dataPtr->applyWrenchVisual)
    MouseEventHandler::Instance()->RemoveReleaseFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName());

  if (!this->SetLink(this->dataPtr->modelName + "::" + _linkName.toStdString()))
    this->Fini();
}

///////////////////////////////////////////////////
void ApplyWrenchDialog::OnResponse(ConstResponsePtr &_msg)
{
  if (!this->dataPtr->requestMsg ||
      _msg->id() != this->dataPtr->requestMsg->id())
    return;

  if (_msg->has_type() && _msg->type() == "gazebo.msgs.Link")
  {
    msgs::Link linkMsg;
    linkMsg.ParseFromString(_msg->serialized_data());

    // Name
    if (!linkMsg.has_name())
      return;

    this->dataPtr->linkName = linkMsg.name();
    this->SetPublisher();

    // CoM
    if (linkMsg.has_inertial() &&
        linkMsg.inertial().has_pose())
    {
      this->SetCoM(msgs::Convert(
          linkMsg.inertial().pose()).pos);
      // Apply force at com by default
      this->SetForcePos(this->dataPtr->comVector);
    }
  }
  delete this->dataPtr->requestMsg;
  this->dataPtr->requestMsg = NULL;
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnApplyAll()
{
  // publish wrench msg
  msgs::Wrench msg;
  msgs::Set(msg.mutable_force(), this->dataPtr->forceVector);
  msgs::Set(msg.mutable_torque(), this->dataPtr->torqueVector);
  msgs::Set(msg.mutable_force_offset(), this->dataPtr->forcePosVector);

  this->dataPtr->wrenchPub->Publish(msg);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnApplyForce()
{
  msgs::Wrench msg;
  msgs::Set(msg.mutable_force(), this->dataPtr->forceVector);
  msgs::Set(msg.mutable_torque(), math::Vector3::Zero);
  msgs::Set(msg.mutable_force_offset(), this->dataPtr->forcePosVector);

  this->dataPtr->wrenchPub->Publish(msg);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnApplyTorque()
{
  // publish wrench msg
  msgs::Wrench msg;
  msgs::Set(msg.mutable_force(), math::Vector3::Zero);
  msgs::Set(msg.mutable_torque(), this->dataPtr->torqueVector);

  this->dataPtr->wrenchPub->Publish(msg);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnCancel()
{
  this->Fini();
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnForcePosChanged(double /*_value*/)
{
  // Update forcePos vector with values from XYZ spins
  this->SetForcePos(
      math::Vector3(this->dataPtr->forcePosXSpin->value(),
                    this->dataPtr->forcePosYSpin->value(),
                    this->dataPtr->forcePosZSpin->value()));
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnForceMagChanged(double /*_magnitude*/)
{
  // Update force vector proportionally

  // Normalize current vector
  math::Vector3 v = this->dataPtr->forceVector;
  if (v == math::Vector3::Zero)
    v = math::Vector3::UnitX;
  else
    v.Normalize();

  // Multiply by new magnitude
  this->SetForce(v * this->dataPtr->forceMagSpin->value());
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnForceChanged(double /*_value*/)
{
  // Update force vector with values from XYZ spins
  this->SetForce(math::Vector3(this->dataPtr->forceXSpin->value(),
                               this->dataPtr->forceYSpin->value(),
                               this->dataPtr->forceZSpin->value()));
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnForceClear()
{
  this->SetForce(math::Vector3::Zero);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnTorqueMagChanged(double /*_magnitude*/)
{
  // Update torque vector proportionally

  // Normalize current vector
  math::Vector3 v = this->dataPtr->torqueVector;
  if (v == math::Vector3::Zero)
    v = math::Vector3::UnitX;
  else
    v.Normalize();

  // Multiply by new magnitude
  this->SetTorque(v * this->dataPtr->torqueMagSpin->value());
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnTorqueChanged(double /*_value*/)
{
  // Update torque vector with values from XYZ spins
  this->SetTorque(math::Vector3(this->dataPtr->torqueXSpin->value(),
                               this->dataPtr->torqueYSpin->value(),
                               this->dataPtr->torqueZSpin->value()));
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnTorqueClear()
{
  this->SetTorque(math::Vector3::Zero);
}

//////////////////////////////////////////////////
void ApplyWrenchDialog::SetPublisher()
{
  std::string topicName = "~/";
  topicName += this->dataPtr->linkName + "/wrench";
  boost::replace_all(topicName, "::", "/");

  this->dataPtr->wrenchPub =
      this->dataPtr->node->Advertise<msgs::Wrench>(topicName);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::AttachVisuals()
{
  if (!gui::get_active_camera() || !gui::get_active_camera()->GetScene())
  {
    gzerr << "Camera or scene missing" << std::endl;
    return;
  }
  if (!this->dataPtr->linkVisual)
  {
    gzerr << "No link visual specified." << std::endl;
    return;
  }

  // Attaching for the first time
  if (!this->dataPtr->applyWrenchVisual)
  {
    // Generate unique name
    std::string visNameBase = this->dataPtr->modelName + "__APPLY_WRENCH__";
    rendering::VisualPtr vis = gui::get_active_camera()->GetScene()->
        GetVisual(visNameBase);

    std::string visName(visNameBase);
    int count = 0;
    while (vis)
    {
      visName = visNameBase + std::to_string(count);
      vis = gui::get_active_camera()->GetScene()->
        GetVisual(visName);
      ++count;
    }

    this->dataPtr->applyWrenchVisual.reset(new rendering::ApplyWrenchVisual(
        visName, this->dataPtr->linkVisual));

    this->dataPtr->applyWrenchVisual->Load();
  }
  // Same link as before: just make sure it is visible
  else if (this->dataPtr->applyWrenchVisual->GetParent() &&
      this->dataPtr->applyWrenchVisual->GetParent() ==
      this->dataPtr->linkVisual)
  {
    this->dataPtr->applyWrenchVisual->SetVisible(true);
  }
  // Different link
  else
  {
    this->dataPtr->linkVisual->AttachVisual(this->dataPtr->applyWrenchVisual);
    this->dataPtr->applyWrenchVisual->Resize();
  }

  if (!this->dataPtr->applyWrenchVisual)
  {
    gzerr << "Failed to attach visual. Closing dialog." << std::endl;
    this->Fini();
  }

  this->SetTorque(this->dataPtr->torqueVector, true);
  this->SetForce(this->dataPtr->forceVector, true);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::ToggleComRadio(bool _checked)
{
  if (_checked)
  {
    this->SetForcePos(this->dataPtr->comVector);
  }
}

/////////////////////////////////////////////////
bool ApplyWrenchDialog::OnMousePress(const common::MouseEvent & _event)
{
  rendering::UserCameraPtr userCamera = gui::get_active_camera();
  if (!userCamera || !this->dataPtr->applyWrenchVisual)
    return false;

  this->dataPtr->draggingTool = false;

  rendering::VisualPtr vis = userCamera->GetVisual(_event.pos,
        this->dataPtr->manipState);

  if (vis)
    return false;

  // Register drag start 2D point and tool pose if on top of a handle
  if (this->dataPtr->manipState == "rot_z" ||
      this->dataPtr->manipState == "rot_y")
  {
    this->dataPtr->draggingTool = true;
    this->dataPtr->applyWrenchVisual->GetRotTool()->SetState(
        this->dataPtr->manipState);

    math::Pose rotToolPose = this->dataPtr->applyWrenchVisual->GetRotTool()
        ->GetWorldPose();
    this->dataPtr->dragStartPose = rotToolPose;
  }
  return false;
}

/////////////////////////////////////////////////
bool ApplyWrenchDialog::OnMouseRelease(const common::MouseEvent & _event)
{
  rendering::UserCameraPtr userCamera = gui::get_active_camera();
  if (!userCamera || !this->dataPtr->applyWrenchVisual)
    return false;

  rendering::VisualPtr vis = userCamera->GetVisual(_event.pos,
      this->dataPtr->manipState);

  if (!vis || _event.dragging)
    return false;

  // Set active and change mode
  if (vis == this->dataPtr->applyWrenchVisual->GetForceVisual())
  {
    this->ActivateWindow();

    if (this->dataPtr->forceVector == math::Vector3::Zero)
      this->SetForce(math::Vector3::UnitX);
    else
      this->SetForce(this->dataPtr->forceVector);

    return true;
  }
  else if (vis == this->dataPtr->applyWrenchVisual->GetTorqueVisual())
  {
    this->ActivateWindow();

    if (this->dataPtr->torqueVector == math::Vector3::Zero)
      this->SetTorque(math::Vector3::UnitX);
    else
      this->SetTorque(this->dataPtr->torqueVector);

    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool ApplyWrenchDialog::OnMouseMove(const common::MouseEvent & _event)
{
  rendering::UserCameraPtr userCamera = gui::get_active_camera();
  if (!userCamera || !this->dataPtr->applyWrenchVisual)
    return false;

  // Dragging tool, slightly modified from ModelManipulator::RotateEntity
  if (_event.dragging && _event.button == common::MouseEvent::LEFT &&
      this->dataPtr->draggingTool)
  {
    math::Vector3 normal;
    math::Vector3 axis;
    if (this->dataPtr->manipState == "rot_z")
    {
      normal = this->dataPtr->dragStartPose.rot.GetZAxis();
      axis = math::Vector3::UnitZ;
    }
    else if (this->dataPtr->manipState == "rot_y")
    {
      normal = this->dataPtr->dragStartPose.rot.GetYAxis();
      axis = math::Vector3::UnitY;
    }
    else
    {
      return false;
    }

    double offset = this->dataPtr->dragStartPose.pos.Dot(normal);

    math::Vector3 pressPoint;
    userCamera->GetWorldPointOnPlane(_event.pressPos.x, _event.pressPos.y,
          math::Plane(normal, offset), pressPoint);

    math::Vector3 newPoint;
    userCamera->GetWorldPointOnPlane(_event.pos.x, _event.pos.y,
        math::Plane(normal, offset), newPoint);

    math::Vector3 v1 = pressPoint - this->dataPtr->dragStartPose.pos;
    math::Vector3 v2 = newPoint - this->dataPtr->dragStartPose.pos;
    v1 = v1.Normalize();
    v2 = v2.Normalize();
    double signTest = v1.Cross(v2).Dot(normal);
    double angle = atan2((v1.Cross(v2)).GetLength(), v1.Dot(v2));

    if (signTest < 0)
      angle *= -1;

    if (QApplication::keyboardModifiers() & Qt::ControlModifier)
      angle = rint(angle / (M_PI * 0.25)) * (M_PI * 0.25);

    math::Quaternion rot(axis, angle);
    rot = this->dataPtr->dragStartPose.rot * rot;

    // Must rotate the tool here to make sure we have proper roll,
    // once the rotation gets transformed into a vector we lose a DOF
    this->dataPtr->applyWrenchVisual->GetRotTool()->SetWorldRotation(rot);

    math::Vector3 vec;
    math::Vector3 rotEuler;
    rotEuler = rot.GetAsEuler();
    vec.x = cos(rotEuler.z)*cos(rotEuler.y);
    vec.y = sin(rotEuler.z)*cos(rotEuler.y);
    vec.z = -sin(rotEuler.y);

    // To local frame
    vec = this->dataPtr->linkVisual->GetWorldPose().rot.RotateVectorReverse(
        vec);

    // Normalize new vector;
    if (vec == math::Vector3::Zero)
      vec = math::Vector3::UnitX;
    else
      vec.Normalize();

    if (this->dataPtr->mode == "force")
    {
      this->NewForceDirection(vec);
    }
    else if (this->dataPtr->mode == "torque")
    {
      this->NewTorqueDirection(vec);
    }
    return true;
  }
  // Highlight hovered tools
  else
  {
    userCamera->GetVisual(_event.pos, this->dataPtr->manipState);

    if (this->dataPtr->manipState == "rot_z" ||
      this->dataPtr->manipState == "rot_y")
    {
      this->dataPtr->applyWrenchVisual->GetRotTool()->SetState(
          this->dataPtr->manipState);
    }
    else
    {
      this->dataPtr->applyWrenchVisual->GetRotTool()->SetState("");
    }
  }

  return false;
}

/////////////////////////////////////////////////
bool ApplyWrenchDialog::eventFilter(QObject *_object, QEvent *_event)
{
  // Attach rotation tool to focused mode
  if (_event->type() == QEvent::FocusIn)
  {
    if (_object == this->dataPtr->forceMagSpin ||
        _object == this->dataPtr->forceXSpin ||
        _object == this->dataPtr->forceYSpin ||
        _object == this->dataPtr->forceZSpin ||
        _object == this->dataPtr->forcePosXSpin ||
        _object == this->dataPtr->forcePosYSpin ||
        _object == this->dataPtr->forcePosZSpin)
    {
      this->SetForce(this->dataPtr->forceVector);
    }
    else if (_object == this->dataPtr->torqueMagSpin ||
             _object == this->dataPtr->torqueXSpin ||
             _object == this->dataPtr->torqueYSpin ||
             _object == this->dataPtr->torqueZSpin)
    {
      this->SetTorque(this->dataPtr->torqueVector);
    }
  }
  // Deactivate this when another dialog is focused
  else if (_event->type() == QEvent::ActivationChange)
  {
    if (!this->dataPtr->mainWindow)
      return false;

    if (_object == this->dataPtr->mainWindow)
    {
      if (!this->isActiveWindow() &&
          !this->dataPtr->mainWindow->isActiveWindow())
      {
        this->SetActive(false);
      }
    }
  }
  // Activate when changing spinboxes with mousewheel
  else if (_event->type() == QEvent::Wheel)
  {
    this->ActivateWindow();
  }

  return false;
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::changeEvent(QEvent *_event)
{
  // Focus in this dialog
  if (_event->type() == QEvent::ActivationChange)
  {
    if (!this->dataPtr->mainWindow)
      return;

    this->SetActive(this->isActiveWindow() ||
        this->dataPtr->mainWindow->isActiveWindow());
  }
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::SetSpinValue(QDoubleSpinBox *_spin, double _value)
{
  _spin->blockSignals(true);
  _spin->setValue(_value);
  _spin->blockSignals(false);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::SetMode(const std::string &_mode)
{
  this->dataPtr->mode = _mode;
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::SetCoM(const math::Vector3 &_com)
{
  // Set com vector and send it to visuals
  this->dataPtr->comVector = _com;

  // Visuals
  if (!this->dataPtr->applyWrenchVisual)
    return;

  this->dataPtr->applyWrenchVisual->SetCoM(this->dataPtr->comVector);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::SetForcePos(const math::Vector3 &_forcePos)
{
  this->dataPtr->forcePosVector = _forcePos;

  // Spins
  this->SetSpinValue(this->dataPtr->forcePosXSpin, _forcePos.x);
  this->SetSpinValue(this->dataPtr->forcePosYSpin, _forcePos.y);
  this->SetSpinValue(this->dataPtr->forcePosZSpin, _forcePos.z);

  // Mode
  this->SetMode("force");

  // Check com box
  if (_forcePos == this->dataPtr->comVector)
  {
    this->dataPtr->comRadio->setChecked(true);
  }
  else
  {
    this->dataPtr->forcePosRadio->setChecked(true);
  }

  // Visuals
  if (!this->dataPtr->applyWrenchVisual)
    return;

  this->dataPtr->applyWrenchVisual->SetForcePos(this->dataPtr->forcePosVector);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::SetForce(const math::Vector3 &_force,
    bool _rotatedByMouse)
{
  this->dataPtr->forceVector = _force;

  // Spins
  this->SetSpinValue(this->dataPtr->forceXSpin, _force.x);
  this->SetSpinValue(this->dataPtr->forceYSpin, _force.y);
  this->SetSpinValue(this->dataPtr->forceZSpin, _force.z);
  this->SetSpinValue(this->dataPtr->forceMagSpin, _force.GetLength());

  // Mode
  if (_force == math::Vector3::Zero)
  {
    if (this->dataPtr->torqueVector == math::Vector3::Zero)
      this->SetMode("none");
    else
      this->SetMode("torque");
  }
  else
  {
    this->SetMode("force");
  }

  // Visuals
  if (!this->dataPtr->applyWrenchVisual)
    return;

  this->dataPtr->applyWrenchVisual->SetForce(_force, _rotatedByMouse);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::NewForceDirection(const math::Vector3 &_dir)
{
  // Normalize direction
  math::Vector3 v = _dir;
  if (v == math::Vector3::Zero)
    v = math::Vector3::UnitX;
  else
    v.Normalize();

  // Multiply by magnitude
  this->SetForce(v * this->dataPtr->forceMagSpin->value(), true);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::SetTorque(const math::Vector3 &_torque,
    bool _rotatedByMouse)
{
  this->dataPtr->torqueVector = _torque;

  // Spins
  this->SetSpinValue(this->dataPtr->torqueXSpin, _torque.x);
  this->SetSpinValue(this->dataPtr->torqueYSpin, _torque.y);
  this->SetSpinValue(this->dataPtr->torqueZSpin, _torque.z);
  this->SetSpinValue(this->dataPtr->torqueMagSpin, _torque.GetLength());

  // Mode
  if (_torque == math::Vector3::Zero)
  {
    if (this->dataPtr->forceVector == math::Vector3::Zero)
      this->SetMode("none");
    else
      this->SetMode("force");
  }
  else
  {
    this->SetMode("torque");
  }

  // Visuals
  if (!this->dataPtr->applyWrenchVisual)
    return;

  this->dataPtr->applyWrenchVisual->SetTorque(_torque, _rotatedByMouse);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::NewTorqueDirection(const math::Vector3 &_dir)
{
  // Normalize direction
  math::Vector3 v = _dir;
  if (v == math::Vector3::Zero)
    v = math::Vector3::UnitX;
  else
    v.Normalize();

  // Multiply by magnitude
  this->SetTorque(v * this->dataPtr->torqueMagSpin->value(), true);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::SetActive(bool _active)
{
  if (!this->dataPtr->applyWrenchVisual)
  {
    gzerr << "No apply wrench visual." << std::endl;
    this->Fini();
    return;
  }
  if (_active)
  {
    // Set visible
    this->dataPtr->applyWrenchVisual->SetVisible(true);

    // Set selected
    event::Events::setSelectedEntity(this->dataPtr->linkName, "normal");

    // Set arrow mode
    g_arrowAct->trigger();

    MouseEventHandler::Instance()->AddPressFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName(),
        boost::bind(&ApplyWrenchDialog::OnMousePress, this, _1));

    MouseEventHandler::Instance()->AddMoveFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName(),
        boost::bind(&ApplyWrenchDialog::OnMouseMove, this, _1));
  }
  else
  {
    this->dataPtr->applyWrenchVisual->SetVisible(false);

    MouseEventHandler::Instance()->RemovePressFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName());
    MouseEventHandler::Instance()->RemoveMoveFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName());

    KeyEventHandler::Instance()->RemovePressFilter(
        "dialog_"+this->dataPtr->applyWrenchVisual->GetName());
  }
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnPreRender()
{
  if (!gui::get_active_camera() || !gui::get_active_camera()->GetScene())
    return;

  rendering::VisualPtr vis = gui::get_active_camera()->GetScene()->
      GetVisual(this->dataPtr->linkName);

  if (!vis)
    this->Fini();
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::OnManipulation()
{
  this->SetActive(false);
}

/////////////////////////////////////////////////
void ApplyWrenchDialog::ActivateWindow()
{
  if (!this->isActiveWindow())
  {
    // Clear focus before activating not to trigger mode change due to FucusIn
    QWidget *focusedWidget = this->focusWidget();
    if (focusedWidget)
      focusedWidget->clearFocus();
    this->activateWindow();
  }
}
