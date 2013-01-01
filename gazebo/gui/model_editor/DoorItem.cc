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

#include "gui/model_editor/BuildingItem.hh"
#include "gui/model_editor/RectItem.hh"
#include "gui/model_editor/DoorItem.hh"
#include "gui/model_editor/WindowDoorInspectorDialog.hh"
#include "gui/model_editor/BuildingMaker.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
DoorItem::DoorItem(): RectItem(), BuildingItem()
{
  this->editorType = "Door";
  this->scale = BuildingMaker::conversionScale;

  this->level = 0;
  this->levelBaseHeight = 0;

  this->doorDepth = 20;
  this->doorHeight = 200;
  this->doorWidth = 100;

  this->width = this->doorWidth;
//  this->height = this->doorDepth + this->doorWidth;
  this->height = this->doorDepth;
  this->drawingWidth = this->width;
  this->drawingHeight = this->height;

  this->UpdateCornerPositions();

  this->doorPos = this->scenePos();

  this->zValueIdle = 3;
  this->setZValue(this->zValueIdle);
}

/////////////////////////////////////////////////
DoorItem::~DoorItem()
{
}

/////////////////////////////////////////////////
QVector3D DoorItem::GetSize()
{
  return QVector3D(this->doorWidth, this->doorDepth, this->doorHeight);
}

/////////////////////////////////////////////////
QVector3D DoorItem::GetScenePosition()
{
  return QVector3D(this->scenePos().x(), this->scenePos().y(), 0);
}

/////////////////////////////////////////////////
double DoorItem::GetSceneRotation()
{
  return this->rotationAngle;
}

/////////////////////////////////////////////////
void DoorItem::paint (QPainter *_painter,
    const QStyleOptionGraphicsItem */*_option*/, QWidget */*_widget*/)
{
  if (this->isSelected())
    this->DrawBoundingBox(_painter);
  this->showCorners(this->isSelected());

  QPointF topLeft(this->drawingOriginX - this->drawingWidth/2,
      this->drawingOriginY - this->drawingHeight/2);
  QPointF topRight(this->drawingOriginX + this->drawingWidth/2,
      this->drawingOriginY - this->drawingHeight/2);
  QPointF bottomLeft(this->drawingOriginX - this->drawingWidth/2,
      this->drawingOriginY + this->drawingHeight/2);
  QPointF bottomRight(this->drawingOriginX  + this->drawingWidth/2,
      this->drawingOriginY + this->drawingHeight/2);

  QPen doorPen;
  doorPen.setStyle(Qt::SolidLine);
  doorPen.setColor(this->borderColor);
  _painter->setPen(doorPen);

  _painter->drawLine(topLeft, bottomLeft + QPointF(0, this->drawingWidth));
  QRect arcRect(topLeft.x() - this->drawingWidth,
      topLeft.y() + this->drawingHeight - this->drawingWidth,
      this->drawingWidth*2, this->drawingWidth*2);
  _painter->drawArc(arcRect, 0, -90 * 16);

  doorPen.setWidth(this->doorDepth);
  _painter->setPen(doorPen);
  _painter->drawLine(topLeft + QPointF(this->doorDepth/2.0,
      this->doorDepth/2.0), topRight - QPointF(this->doorDepth/2.0,
      -this->doorDepth/2.0));

  double borderSize = 1.0;
  doorPen.setColor(Qt::white);
  doorPen.setWidth(this->doorDepth - borderSize*2);
  _painter->setPen(doorPen);
  _painter->drawLine(topLeft + QPointF(this->doorDepth/2.0,
      this->doorDepth/2.0), topRight - QPointF(this->doorDepth/2.0,
      -this->doorDepth/2.0));

  this->doorWidth = this->drawingWidth;
  this->doorDepth = this->drawingHeight;
  this->doorPos = this->scenePos();

//  QGraphicsPolygonItem::paint(_painter, _option, _widget);
}

/////////////////////////////////////////////////
void DoorItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *_event)
{
  WindowDoorInspectorDialog dialog(WindowDoorInspectorDialog::DOOR);
  dialog.SetWidth(this->doorWidth * this->scale);
  dialog.SetDepth(this->doorDepth * this->scale);
  dialog.SetHeight(this->doorHeight * this->scale);
  QPointF itemPos = this->doorPos * this->scale;
  itemPos.setY(-itemPos.y());
  dialog.SetPosition(itemPos);
  if (dialog.exec() == QDialog::Accepted)
  {
    this->SetSize(QSize(dialog.GetWidth() / this->scale,
        (dialog.GetDepth() / this->scale)));
    this->doorWidth = dialog.GetWidth() / this->scale;
    this->doorHeight = dialog.GetHeight() / this->scale;
    this->doorDepth = dialog.GetDepth() / this->scale;
    if ((fabs(dialog.GetPosition().x() - itemPos.x()) >= 0.01)
        || (fabs(dialog.GetPosition().y() - itemPos.y()) >= 0.01))
    {
      itemPos = dialog.GetPosition() / this->scale;
      itemPos.setY(-itemPos.y());
      this->doorPos = itemPos;
      this->setPos(this->doorPos);
      this->setParentItem(NULL);
    }
    this->DoorChanged();
  }
  _event->setAccepted(true);
}

/////////////////////////////////////////////////
void DoorItem::DoorChanged()
{
  emit widthChanged(this->doorWidth);
  emit depthChanged(this->doorDepth);
  emit heightChanged(this->doorHeight);
  emit positionChanged(this->doorPos.x(), this->doorPos.y(),
      this->levelBaseHeight/* + this->doorElevation*/);
}

/////////////////////////////////////////////////
void DoorItem::SizeChanged()
{
  emit widthChanged(this->doorWidth);
  emit depthChanged(this->doorDepth);
}
