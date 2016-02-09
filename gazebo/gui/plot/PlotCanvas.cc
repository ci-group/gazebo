/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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

#include <map>

#include "gazebo/common/Assert.hh"
#include "gazebo/common/Console.hh"

#include "gazebo/gui/plot/VariablePill.hh"
#include "gazebo/gui/plot/VariablePillContainer.hh"
#include "gazebo/gui/plot/PlotCanvas.hh"

using namespace gazebo;
using namespace gui;

namespace gazebo
{
  namespace gui
  {
    /// \internal
    /// \brief PlotCanvas private data
    class PlotCanvasPrivate
    {
      /// \brief Text label
      public: QLabel *title;

      /// \brief Layout that contains all the plots.
      public: QLayout *plotLayout;

      /// \brief Container for all the variables on the Y axis.
      public: VariablePillContainer *yVariableContainer = NULL;
    };
  }
}

/////////////////////////////////////////////////
PlotCanvas::PlotCanvas(QWidget *_parent)
  : QWidget(_parent),
    dataPtr(new PlotCanvasPrivate())
{
  // Plot title
  this->dataPtr->title = new QLabel("Plot Name");
  QHBoxLayout *titleLayout = new QHBoxLayout;
  titleLayout->addWidget(this->dataPtr->title);
  titleLayout->setAlignment(Qt::AlignHCenter);

  // Settings
  QMenu *settingsMenu = new QMenu;
  QAction *clearPlotAct = new QAction("Clear all fields", settingsMenu);
  clearPlotAct->setStatusTip(tr("Clear variables and all plots on canvas"));
  connect(clearPlotAct, SIGNAL(triggered()), this, SLOT(OnClearCanvas()));
  QAction *deletePlotAct = new QAction("Delete Plot", settingsMenu);
  deletePlotAct->setStatusTip(tr("Delete entire canvas"));
  connect(deletePlotAct, SIGNAL(triggered()), this, SLOT(OnDeleteCanvas()));

  settingsMenu->addAction(clearPlotAct);
  settingsMenu->addAction(deletePlotAct);

  QToolButton *settingsButton = new QToolButton();
  settingsButton->installEventFilter(this);
  settingsButton->setToolTip(tr("Settings"));
  settingsButton->setIcon(QIcon(":/images/settings.png"));
  settingsButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
  settingsButton->setPopupMode(QToolButton::InstantPopup);
  settingsButton->setMenu(settingsMenu);

  QHBoxLayout *settingsLayout = new QHBoxLayout;
  settingsLayout->addWidget(settingsButton);

  QHBoxLayout *titleSettingsLayout = new QHBoxLayout;
  titleSettingsLayout->addLayout(titleLayout);
  titleSettingsLayout->addLayout(settingsLayout);

  // X and Y variable containers
  VariablePillContainer *xVariableContainer = new VariablePillContainer(this);
  xVariableContainer->SetText("x: ");
  xVariableContainer->SetMaxSize(1);
  xVariableContainer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
  // hardcode x axis for now
  xVariableContainer->AddVariablePill("SimTime");
  xVariableContainer->setEnabled(false);

  this->dataPtr->yVariableContainer = new VariablePillContainer(this);
  this->dataPtr->yVariableContainer->SetText("y: ");
  this->dataPtr->yVariableContainer->setSizePolicy(
      QSizePolicy::Minimum, QSizePolicy::Fixed);

  QVBoxLayout *variableContainerLayout = new QVBoxLayout;
  variableContainerLayout->addWidget(xVariableContainer);
  variableContainerLayout->addWidget(this->dataPtr->yVariableContainer);

  // plot
  QScrollArea *plotScrollArea = new QScrollArea(this);
  plotScrollArea->setLineWidth(0);
  plotScrollArea->setFrameShape(QFrame::NoFrame);
  plotScrollArea->setFrameShadow(QFrame::Plain);
  plotScrollArea->setSizePolicy(QSizePolicy::Minimum,
                                QSizePolicy::Expanding);

  plotScrollArea->setWidgetResizable(true);
  plotScrollArea->viewport()->installEventFilter(this);

  QFrame *plotFrame = new QFrame(plotScrollArea);
  plotFrame->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  this->dataPtr->plotLayout = new QVBoxLayout;
  plotFrame->setLayout(this->dataPtr->plotLayout);

  plotScrollArea->setWidget(plotFrame);


  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addLayout(titleSettingsLayout);
  mainLayout->addLayout(variableContainerLayout);
  mainLayout->addWidget(plotScrollArea);
  this->setLayout(mainLayout);
}

/////////////////////////////////////////////////
PlotCanvas::~PlotCanvas()
{
}

/////////////////////////////////////////////////
void PlotCanvas::Update()
{
}

/////////////////////////////////////////////////
bool PlotCanvas::eventFilter(QObject *_o, QEvent *_e)
{
  if (_e->type() == QEvent::Wheel)
  {
    _e->ignore();
    return true;
  }

  return QWidget::eventFilter(_o, _e);
}

/////////////////////////////////////////////////
void PlotCanvas::OnClearCanvas()
{
}

/////////////////////////////////////////////////
void PlotCanvas::OnDeleteCanvas()
{
  emit CanvasDeleted();
}
