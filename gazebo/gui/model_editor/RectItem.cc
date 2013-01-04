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

#include "gazebo/gui/model_editor/RectItem.hh"
#include "gazebo/gui/model_editor/CornerGrabber.hh"
#include "gazebo/gui/model_editor/RotateHandle.hh"
#include "gazebo/gui/model_editor/EditorItem.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
RectItem::RectItem():
    borderColor(Qt::black),
    location(0,0),
    gridSpace(10)
{
  this->editorType = "Rect";

  this->width = 100;
  this->height = 100;

  this->drawingOriginX = 0;
  this->drawingOriginY = 0;

  this->drawingWidth = this->width;
  this->drawingHeight = this->height;

  for (int i = 0; i < 8; ++i)
  {
    CornerGrabber *corner = new CornerGrabber(this, i);
    this->corners.push_back(corner);
  }
  this->rotateHandle = new RotateHandle(this);

  this->setSelected(false);
  this->setFlags(this->flags() | QGraphicsItem::ItemIsSelectable);
  this->setFlag(QGraphicsItem::ItemSendsScenePositionChanges);

  this->UpdateCornerPositions();
  this->setAcceptHoverEvents(true);

  this->cursors.push_back(Qt::SizeFDiagCursor);
  this->cursors.push_back(Qt::SizeVerCursor);
  this->cursors.push_back(Qt::SizeBDiagCursor);
  this->cursors.push_back(Qt::SizeHorCursor);

  this->setCursor(Qt::SizeAllCursor);

  this->rotationAngle = 0;

  this->zValueIdle = 1;
  this->zValueSelected = 5;

  this->SetResizeFlag(ITEM_WIDTH | ITEM_HEIGHT);

  this->openInspectorAct = new QAction(tr("&Open Inspector"), this);
  this->openInspectorAct->setStatusTip(tr("Open Inspector"));
  connect(this->openInspectorAct, SIGNAL(triggered()),
    this, SLOT(OnOpenInspector()));

}

 /////////////////////////////////////////////////
RectItem::~RectItem()
{
  for (int i = 0; i < 8; ++i)
  {
    this->corners[i]->setParentItem(NULL);
    delete this->corners[i];
  }
  this->rotateHandle->setParentItem(NULL);
  delete this->rotateHandle;
}

/////////////////////////////////////////////////
void RectItem::showCorners(bool _show)
{
  for (int i = 0; i < 8; ++i)
    this->corners[i]->setVisible(_show);
  this->rotateHandle->setVisible(_show);
}

/////////////////////////////////////////////////
void RectItem::AdjustSize(double _x, double _y)
{
  this->width += _x;
  this->height += _y;
  this->drawingWidth = this->width;
  this->drawingHeight = this->height;
}

/////////////////////////////////////////////////
QVariant RectItem::itemChange(GraphicsItemChange _change,
  const QVariant &_value)
{
  if (_change == QGraphicsItem::ItemSelectedChange && this->scene())
  {

    if (_value.toBool())
    {
      this->setZValue(zValueSelected);
      for (int i = 0; i < 8; ++i)
      {
        if (this->corners[i]->isEnabled())
          this->corners[i]->installSceneEventFilter(this);
      }
      this->rotateHandle->installSceneEventFilter(this);
    }
    else
    {
      this->setZValue(zValueIdle);
      for (int i = 0; i < 8; ++i)
      {
        if (this->corners[i]->isEnabled())
          this->corners[i]->removeSceneEventFilter(this);
      }
      this->rotateHandle->removeSceneEventFilter(this);
    }
  }
  else if (_change == QGraphicsItem::ItemScenePositionHasChanged
      && this->scene())
  {
    emit posXChanged(this->scenePos().x());
    emit posYChanged(this->scenePos().y());
  }
  if (_change == QGraphicsItem::ItemParentChange && this->scene())
  {

  }
  return QGraphicsItem::itemChange(_change, _value);
}

/////////////////////////////////////////////////
bool RectItem::sceneEventFilter(QGraphicsItem * _watched, QEvent *_event)
{
  RotateHandle *rotateH = dynamic_cast<RotateHandle *>(_watched);
  if (rotateH != NULL)
    return this->rotateEventFilter(rotateH, _event);

  CornerGrabber *corner = dynamic_cast<CornerGrabber *>(_watched);
  if (corner != NULL && corner->isEnabled())
    return this->cornerEventFilter(corner, _event);

  return false;
}

/////////////////////////////////////////////////
bool RectItem::rotateEventFilter(RotateHandle *_rotate,
    QEvent *_event)
{
  QGraphicsSceneMouseEvent *mouseEvent =
    dynamic_cast<QGraphicsSceneMouseEvent*>(_event);

  switch (_event->type())
  {
    case QEvent::GraphicsSceneMousePress:
    {
      _rotate->SetMouseState(QEvent::GraphicsSceneMousePress);
      _rotate->SetMouseDownX(mouseEvent->pos().x());
      _rotate->SetMouseDownY(mouseEvent->pos().y());

      break;
    }
    case QEvent::GraphicsSceneMouseRelease:
    {
      _rotate->SetMouseState(QEvent::GraphicsSceneMouseRelease);
      break;
    }
    case QEvent::GraphicsSceneMouseMove:
    {
      _rotate->SetMouseState(QEvent::GraphicsSceneMouseMove);
      break;
    }
    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHoverMove:
    {
      QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));
      return true;
    }
    case QEvent::GraphicsSceneHoverLeave:
    {
      QApplication::restoreOverrideCursor();
      return true;
    }
    default:
      return false;
      break;
  }

  if (mouseEvent == NULL)
    return false;

  if (_rotate->GetMouseState() == QEvent::GraphicsSceneMouseMove)
  {
    QPoint localCenter(this->drawingOriginX, this->drawingOriginY);
    QPointF center = this->mapToScene(localCenter);

    QPointF newPoint = mouseEvent->scenePos();
    QLineF line(center.x(), center.y(), newPoint.x(), newPoint.y());

    double angle = 0;

    if (this->parentItem())
    {
      QPointF localCenterTop(this->drawingOriginX, this->drawingOriginY
          + this->drawingHeight);
      QPointF centerTop = this->mapToScene(localCenterTop);
      QLineF lineCenter(center.x(), center.y(), centerTop.x(), centerTop.y());
      angle = -lineCenter.angleTo(line);

      if (angle < 0)
        angle += 360;
      if (angle < 90 || angle > 270)
      {
        angle = 180;
        this->SetRotation(this->GetRotation() + angle);
      }
    }
    else
    {
      QLineF prevLine(center.x(), center.y(),
          mouseEvent->lastScenePos().x(), mouseEvent->lastScenePos().y());
      angle = -prevLine.angleTo(line);
      this->SetRotation(this->GetRotation() + angle);
    }
//    this->setTransformOriginPoint(localCenter);
//    this->setRotation(this->rotation() -prevLine.angleTo(line));
  }
  return true;
}

/////////////////////////////////////////////////
bool RectItem::cornerEventFilter(CornerGrabber *_corner,
    QEvent *_event)
{
  QGraphicsSceneMouseEvent *mouseEvent =
    dynamic_cast<QGraphicsSceneMouseEvent*>(_event);

  switch (_event->type())
  {
    case QEvent::GraphicsSceneMousePress:
    {
      _corner->SetMouseState(QEvent::GraphicsSceneMousePress);
      _corner->SetMouseDownX(mouseEvent->pos().x());
      _corner->SetMouseDownY(mouseEvent->pos().y());
      break;
    }
    case QEvent::GraphicsSceneMouseRelease:
    {
      _corner->SetMouseState(QEvent::GraphicsSceneMouseRelease);
      break;
    }
    case QEvent::GraphicsSceneMouseMove:
    {
      _corner->SetMouseState(QEvent::GraphicsSceneMouseMove);
      break;
    }
    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHoverMove:
    {
      double angle = this->rotationAngle
          - static_cast<int>(rotationAngle/360) * 360;
      double range = 22.5;
      if (angle < 0)
        angle += 360;

      if ((angle > (360 - range)) || (angle < range)
          || ((angle <= (180 + range)) && (angle > (180 - range))))
      {
        QApplication::setOverrideCursor(
            QCursor(this->cursors[_corner->GetIndex() % 4]));
      }
      else if (((angle <= (360 - range)) && (angle > (270 + range)))
          || ((angle <= (180 - range)) && (angle > (90 + range))))
      {
        QApplication::setOverrideCursor(
            QCursor(this->cursors[(_corner->GetIndex() + 3) % 4]));
      }
      else if (((angle <= (270 + range)) && (angle > (270 - range)))
          || ((angle <= (90 + range)) && (angle > (90 - range))))
      {
        QApplication::setOverrideCursor(
            QCursor(this->cursors[(_corner->GetIndex() + 2) % 4]));
      }
      else
      {
        QApplication::setOverrideCursor(
            QCursor(this->cursors[(_corner->GetIndex() + 1) % 4]));
      }
      return true;
    }
    case QEvent::GraphicsSceneHoverLeave:
    {
      QApplication::restoreOverrideCursor();
      return true;
    }
    default:
      return false;
  }

  if (!mouseEvent)
    return false;


  if (_corner->GetMouseState() == QEvent::GraphicsSceneMouseMove)
  {
    double xPos = mouseEvent->pos().x();
    double yPos = mouseEvent->pos().y();

    // depending on which corner has been grabbed, we want to move the position
    // of the item as it grows/shrinks accordingly. so we need to either add
    // or subtract the offsets based on which corner this is.

    int xAxisSign = 0;
    int yAxisSign = 0;
    switch(_corner->GetIndex())
    {
      // corners
      case 0:
      {
        xAxisSign = 1;
        yAxisSign = 1;
        break;
      }
      case 2:
      {
        xAxisSign = -1;
        yAxisSign = 1;
        break;
      }
      case 4:
      {
        xAxisSign = -1;
        yAxisSign = -1;
        break;
      }
      case 6:
      {
        xAxisSign = +1;
        yAxisSign = -1;
        break;
      }
      //edges
      case 1:
      {
        xAxisSign = 0;
        yAxisSign = 1;
        break;
      }
      case 3:
      {
        xAxisSign = -1;
        yAxisSign = 0;
        break;
      }
      case 5:
      {
        xAxisSign = 0;
        yAxisSign = -1;
        break;
      }
      case 7:
      {
        xAxisSign = 1;
        yAxisSign = 0;
        break;
      }
      default:
        break;
    }

    // if the mouse is being dragged, calculate a new size and also position
    // for resizing the box

    double xMoved = _corner->GetMouseDownX() - xPos;
    double yMoved = _corner->GetMouseDownY() - yPos;

    double newWidth = this->width + (xAxisSign * xMoved);
    if (newWidth < 20)
      newWidth  = 20;

    double newHeight = this->height + (yAxisSign * yMoved);
    if (newHeight < 20)
      newHeight = 20;

    double deltaWidth = newWidth - this->width;
    double deltaHeight = newHeight - this->height;

    this->AdjustSize(deltaWidth, deltaHeight);

    deltaWidth *= (-1);
    deltaHeight *= (-1);

    double angle = rotationAngle / 360.0 * (2 * M_PI);
    double dx = 0;
    double dy = 0;
    switch(_corner->GetIndex())
    {
      // corners
      case 0:
      {
        dx = sin(-angle) * deltaHeight/2;
        dy = cos(-angle) * deltaHeight/2;
        dx += cos(angle) * deltaWidth/2;
        dy += sin(angle) * deltaWidth/2;
        this->SetPosition(this->pos() + QPointF(dx, dy));
        break;
      }
      case 2:
      {
        dx = sin(-angle) * deltaHeight/2;
        dy = cos(-angle) * deltaHeight/2;
        dx += -cos(angle) * deltaWidth/2;
        dy += -sin(angle) * deltaWidth/2;
        this->SetPosition(this->pos() + QPointF(dx, dy));
        break;
      }
      case 4:
      {
        dx = -sin(-angle) * deltaHeight/2;
        dy = -cos(-angle) * deltaHeight/2;
        dx += -cos(angle) * deltaWidth/2;
        dy += -sin(angle) * deltaWidth/2;
        this->SetPosition(this->pos() + QPointF(dx, dy));
        break;
      }
      case 6:
      {
        dx = -sin(-angle) * deltaHeight/2;
        dy = -cos(-angle) * deltaHeight/2;
        dx += cos(angle) * deltaWidth/2;
        dy += sin(angle) * deltaWidth/2;
        this->SetPosition(this->pos() + QPointF(dx, dy));
        break;
      }
      // edges
      case 1:
      {
        dx = sin(-angle) * deltaHeight/2;
        dy = cos(-angle) * deltaHeight/2;
        this->SetPosition(this->pos() + QPointF(dx, dy));
        break;
      }
      case 3:
      {
        dx = cos(-angle) * deltaWidth/2;
        dy = -sin(-angle) * deltaWidth/2;
        this->SetPosition(this->pos() - QPointF(dx, dy));
        break;
      }
      case 5:
      {
        dx = sin(-angle) * deltaHeight/2;
        dy = cos(-angle) * deltaHeight/2;
        this->SetPosition(this->pos() - QPointF(dx, dy));
        break;
      }
      case 7:
      {
        dx = cos(angle) * deltaWidth/2;
        dy = sin(angle) * deltaWidth/2;
        this->SetPosition(this->pos() + QPointF(dx, dy));
        break;
      }
      default:
        break;
    }
    this->UpdateCornerPositions();
    this->update();

    /*if (_corner->GetIndex() == 1 || _corner->GetIndex() == 5 ||
        (_corner->GetIndex() % 2 == 0))
      emit depthChanged(this->drawingHeight);

    if (_corner->GetIndex() == 3 || _corner->GetIndex() == 7 ||
        (_corner->GetIndex() % 2 == 0))
      emit widthChanged(this->drawingWidth);*/
  }
  return true;
}

/////////////////////////////////////////////////
void RectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *_event)
{
  _event->setAccepted(true);

  /// TODO: uncomment to enable snap to grid
/*  this->location.setX( (static_cast<int>(this->location.x())
      / this->gridSpace) * this->gridSpace);
  this->location.setY( (static_cast<int>(this->location.y())
      / this->gridSpace) * this->gridSpace);*/

//  this->SetPosition(this->location);
}

/////////////////////////////////////////////////
void RectItem::mousePressEvent(QGraphicsSceneMouseEvent *_event)
{
  if (!this->isSelected())
    this->scene()->clearSelection();

  this->setSelected(true);
  QApplication::setOverrideCursor(QCursor(Qt::SizeAllCursor));
//  this->location = this->pos();
  _event->setAccepted(true);
}

/////////////////////////////////////////////////
void RectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *_event)
{
  if (!this->isSelected())
    return;

  QPointF delta = _event->scenePos() - _event->lastScenePos();
  this->SetPosition(this->scenePos() + delta);
//  this->location += delta;
//  this->SetPosition(this->location);
}

/////////////////////////////////////////////////
void RectItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *_event)
{
  _event->setAccepted(true);
}

/////////////////////////////////////////////////
void RectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *_event)
{
  if (!this->isSelected())
  {
    _event->ignore();
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));

  for (int i = 0; i < 8; ++i)
  {
    if (this->corners[i]->isEnabled())
      this->corners[i]->removeSceneEventFilter(this);
  }
  this->rotateHandle->removeSceneEventFilter(this);
}

/////////////////////////////////////////////////
void RectItem::hoverMoveEvent(QGraphicsSceneHoverEvent *_event)
{
  if (!this->isSelected())
  {
    _event->ignore();
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::SizeAllCursor));
}

/////////////////////////////////////////////////
void RectItem::hoverEnterEvent(QGraphicsSceneHoverEvent *_event)
{
  if (!this->isSelected())
  {
    _event->ignore();
    return;
  }

  QApplication::setOverrideCursor(QCursor(Qt::SizeAllCursor));

//    this->borderColor = Qt::red;
  for (int i = 0; i < 8; ++i)
  {
    if (this->corners[i]->isEnabled())
      this->corners[i]->installSceneEventFilter(this);
  }
  this->rotateHandle->installSceneEventFilter(this);
}

/////////////////////////////////////////////////
void RectItem::UpdateCornerPositions()
{
  int cornerWidth = (this->corners[0]->boundingRect().width())/2;
  int cornerHeight = (this->corners[0]->boundingRect().height())/2;

  this->corners[0]->setPos(
      this->drawingOriginX - this->drawingWidth/2 - cornerWidth,
      this->drawingOriginY - this->drawingHeight/2 - cornerHeight);
  this->corners[2]->setPos(
      this->drawingOriginX + this->drawingWidth/2 - cornerWidth,
      this->drawingOriginY - this->drawingHeight/2 - cornerHeight);
  this->corners[4]->setPos(
      this->drawingOriginX + this->drawingWidth/2 - cornerWidth,
      this->drawingOriginY + this->drawingHeight/2 - cornerHeight);
  this->corners[6]->setPos(
      this->drawingOriginX - this->drawingWidth/2 - cornerWidth,
      this->drawingOriginY + this->drawingHeight/2 - cornerHeight);

  this->corners[1]->setPos(this->drawingOriginX - cornerWidth,
      this->drawingOriginY - this->drawingHeight/2 - cornerHeight);
  this->corners[3]->setPos(
      this->drawingOriginX + this->drawingWidth/2 - cornerWidth,
      this->drawingOriginY - cornerHeight);
  this->corners[5]->setPos(this->drawingOriginX - cornerWidth,
      this->drawingOriginY + this->drawingHeight/2 - cornerHeight);
  this->corners[7]->setPos(
      this->drawingOriginX - this->drawingWidth/2 - cornerWidth,
      this->drawingOriginY - cornerHeight);

  this->rotateHandle->setPos(this->drawingOriginX,
      this->drawingOriginY - this->drawingHeight/2);

  this->SizeChanged();

//  this->setPolygon(QPolygonF(this->boundingRect()));
}

/////////////////////////////////////////////////
void RectItem::SetWidth(int _width)
{
  this->width = _width;
  this->drawingWidth = this->width;
  this->UpdateCornerPositions();
  this->update();

  emit widthChanged(this->drawingWidth);
}

/////////////////////////////////////////////////
void RectItem::SetHeight(int _height)
{
  this->height = _height;
  this->drawingHeight = this->height;
  this->UpdateCornerPositions();
  this->update();

  emit depthChanged(this->drawingHeight);
}

/////////////////////////////////////////////////
void RectItem::SetSize(QSize _size)
{
  this->width = _size.width();
  this->drawingWidth = this->width;
  this->height = _size.height();
  this->drawingHeight = this->height;
  this->UpdateCornerPositions();
  this->update();

  emit widthChanged(this->drawingWidth);
  emit depthChanged(this->drawingHeight);
}

/////////////////////////////////////////////////
int RectItem::GetWidth()
{
  return this->drawingWidth;
}

/////////////////////////////////////////////////
int RectItem::GetHeight()
{
  return this->drawingHeight;
}

/////////////////////////////////////////////////
QRectF RectItem::boundingRect() const
{
  return QRectF(-this->width/2, -this->height/2, this->width, this->height);
}

/////////////////////////////////////////////////
void RectItem::DrawBoundingBox(QPainter *_painter)
{
  _painter->save();
  QPen boundingBoxPen;
  boundingBoxPen.setStyle(Qt::DashDotLine);
  boundingBoxPen.setColor(Qt::darkGray);
  boundingBoxPen.setCapStyle(Qt::RoundCap);
  boundingBoxPen.setJoinStyle(Qt::RoundJoin);
  _painter->setPen(boundingBoxPen);
  _painter->setOpacity(0.8);
  _painter->drawRect(this->boundingRect());
  _painter->restore();
}

/////////////////////////////////////////////////
QVector3D RectItem::GetSize()
{
  return QVector3D(this->width, this->height, 0);
}

/////////////////////////////////////////////////
QVector3D RectItem::GetScenePosition()
{
  return QVector3D(this->scenePos().x(), this->scenePos().y(), 0);
}

/////////////////////////////////////////////////
double RectItem::GetSceneRotation()
{
  return this->rotationAngle;
}

/////////////////////////////////////////////////
void RectItem::paint(QPainter *_painter, const QStyleOptionGraphicsItem *,
    QWidget *)
{
  _painter->save();

  QPointF topLeft(this->drawingOriginX - this->drawingWidth/2,
      this->drawingOriginY - this->drawingHeight/2);
  QPointF topRight(this->drawingOriginX + this->drawingWidth/2,
      this->drawingOriginY - this->drawingHeight/2);
  QPointF bottomLeft(this->drawingOriginX - this->drawingWidth/2,
      this->drawingOriginY + this->drawingHeight/2);
  QPointF bottomRight(this->drawingOriginX  + this->drawingWidth/2,
      this->drawingOriginY + this->drawingHeight/2);

  QPen rectPen;
  rectPen.setStyle(Qt::SolidLine);
  rectPen.setColor(borderColor);
  _painter->setPen(rectPen);

  _painter->drawLine(topLeft, topRight);
  _painter->drawLine(topRight, bottomRight);
  _painter->drawLine(bottomRight, bottomLeft);
  _painter->drawLine(bottomLeft, topLeft);
  _painter->restore();
}

/////////////////////////////////////////////////
void RectItem::mouseMoveEvent(QGraphicsSceneDragDropEvent *_event)
{
  _event->setAccepted(false);
}

/////////////////////////////////////////////////
void RectItem::mousePressEvent(QGraphicsSceneDragDropEvent *_event)
{
  _event->setAccepted(false);
}

/////////////////////////////////////////////////
void RectItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *_event)
{
  QMenu menu;
  menu.addAction(this->openInspectorAct);
  menu.exec(_event->screenPos());
  _event->accept();
}

/////////////////////////////////////////////////
void RectItem::OnOpenInspector()
{
}

/////////////////////////////////////////////////
void RectItem::SetPosition(QPointF _pos)
{
  this->SetPosition(_pos.x(), _pos.y());
}

/////////////////////////////////////////////////
void RectItem::SetPosition(double _x, double _y)
{
  this->setPos(_x, _y);
//  emit posXChanged(_x);
//  emit posYChanged(_y);
}

/////////////////////////////////////////////////
void RectItem::SetRotation(double _angle)
{
//  double halfX = this->drawingOriginX +
//      (this->drawingOriginX + this->drawingWidth)/2;
//  double halfY = this->drawingOriginY +
//      (this->drawingOriginY + this->drawingHeight)/2;

//  this->translate(halfX, halfY);
  this->rotate(_angle - this->rotationAngle);
//  this->translate(-halfX, -halfY);

  this->rotationAngle = _angle;
  emit yawChanged(this->rotationAngle);
}

/////////////////////////////////////////////////
double RectItem::GetRotation()
{
  return this->rotationAngle;
}

/////////////////////////////////////////////////
void RectItem::SizeChanged()
{
  emit depthChanged(this->drawingHeight);
  emit widthChanged(this->drawingWidth);
}

/////////////////////////////////////////////////
void RectItem::SetResizeFlag(unsigned int _flag)
{
  if (this->resizeFlag == _flag)
    return;

  this->resizeFlag = _flag;
  for (int i = 0; i < 8; ++i)
    this->corners[i]->setEnabled(false);


  if (resizeFlag & ITEM_WIDTH)
  {
    this->corners[3]->setEnabled(true);
    this->corners[7]->setEnabled(true);
  }
  if (resizeFlag & ITEM_HEIGHT)
  {
    this->corners[1]->setEnabled(true);
    this->corners[5]->setEnabled(true);
  }
  if ((resizeFlag & ITEM_WIDTH) && (resizeFlag & ITEM_HEIGHT))
  {
    this->corners[1]->setEnabled(true);
    this->corners[2]->setEnabled(true);
    this->corners[3]->setEnabled(true);
    this->corners[6]->setEnabled(true);
  }
}
