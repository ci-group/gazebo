/*
 * Copyright (C) 2012-2015 Open Source Robotics Foundation
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

#ifdef _WIN32
  // Ensure that Winsock2.h is included before Windows.h, which can get
  // pulled in by anybody (e.g., Boost).
  #include <Winsock2.h>
#endif

#include <iostream>

#include "gazebo/rendering/RenderEvents.hh"
#include "gazebo/gui/LayersWidget.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
LayersWidget::LayersWidget(QWidget *_parent)
  : QWidget(_parent)
{
  this->setObjectName("layersList");

  QVBoxLayout *mainLayout = new QVBoxLayout;
  this->layerList = new QListWidget(this);

  QListWidgetItem *item = new QListWidgetItem("Layer 0", this->layerList);
  item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
  item->setCheckState(Qt::Checked);
  item->setData(Qt::UserRole, QVariant(0));
  this->layerList->addItem(item);

  item = new QListWidgetItem("Layer 1", this->layerList);
  item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
  item->setCheckState(Qt::Checked);
  item->setData(Qt::UserRole, QVariant(1));
  this->layerList->addItem(item);

  item = new QListWidgetItem("Layer 2", this->layerList);
  item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
  item->setCheckState(Qt::Checked);
  item->setData(Qt::UserRole, QVariant(2));
  this->layerList->addItem(item);

  mainLayout->addWidget(this->layerList);

  this->setLayout(mainLayout);
  this->layout()->setContentsMargins(0, 0, 0, 0);

  connect(this->layerList, SIGNAL(itemClicked(QListWidgetItem *)),
          this, SLOT(OnLayerSelected(QListWidgetItem *)));

  this->connections.push_back(
      rendering::Events::ConnectNewLayer(
        boost::bind(&LayersWidget::OnNewLayer, this, _1)));
}

/////////////////////////////////////////////////
LayersWidget::~LayersWidget()
{
}

/////////////////////////////////////////////////
void LayersWidget::OnLayerSelected(QListWidgetItem *_layer)
{
  rendering::Events::toggleLayer(_layer->data(Qt::UserRole).toInt());
}

/////////////////////////////////////////////////
void LayersWidget::OnNewLayer(const int32_t _layer)
{
  std::ostringstream stream;
  stream << "Layer " << _layer;

  QListWidgetItem *item = new QListWidgetItem(stream.str().c_str(),
      this->layerList);

  item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
  item->setCheckState(Qt::Checked);
  item->setData(Qt::UserRole, QVariant(_layer));
  this->layerList->addItem(item);
}
