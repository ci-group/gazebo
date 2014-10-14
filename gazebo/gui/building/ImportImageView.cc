/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#include "gazebo/gui/building/ImportImageDialog.hh"
#include "gazebo/gui/building/GridLines.hh"
#include "gazebo/gui/building/EditorItem.hh"
#include "gazebo/gui/building/LineSegmentItem.hh"
#include "gazebo/gui/building/MeasureItem.hh"
#include "gazebo/gui/building/ImportImageView.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
ImportImageView::ImportImageView(ImportImageDialog *_parent)
  : QGraphicsView(_parent), currentMouseItem(0)
{
  this->setObjectName("importImageView");

  this->parent = _parent;

  this->drawInProgress = false;

  this->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
  this->gridLines = NULL;
  this->imageItem = NULL;
  this->imagePixmap = NULL;

  this->noImageText = NULL;

  this->measureItem = NULL;
  this->drawDistanceEnabled = false;
}

/////////////////////////////////////////////////
ImportImageView::~ImportImageView()
{
}

/////////////////////////////////////////////////
void ImportImageView::SetImage(const std::string &_filename)
{
  if (this->imageItem)
    this->scene()->removeItem(this->imageItem);
  if (this->gridLines)
  {
    this->scene()->removeItem(this->gridLines);
    this->gridLines = NULL;
  }
  if (this->noImageText)
  {
    this->scene()->removeItem(this->noImageText);
    this->noImageText = NULL;
  }
  if (this->measureItem)
    this->scene()->removeItem(this->measureItem);

  this->imagePixmap = new QPixmap(QString(_filename.c_str()));
  this->imageWidthPx = this->imagePixmap->width();
  this->imageItem = new QGraphicsPixmapItem(this->imagePixmap->scaled(
      this->scene()->sceneRect().width(),
      this->scene()->sceneRect().height(), Qt::KeepAspectRatio));

  this->pixmapWidthPx = this->imageItem->pixmap().width();
  this->pixmapHeightPx = this->imageItem->pixmap().height();

  if (this->imageItem)
  {
    this->scene()->addItem(this->imageItem);
    this->parent->resolutionSpin->setButtonSymbols(
        QAbstractSpinBox::UpDownArrows);
    this->parent->resolutionSpin->setReadOnly(false);
  }
  else
  {
    this->scene()->addItem(this->gridLines);
  }
}

/////////////////////////////////////////////////
void ImportImageView::resizeEvent(QResizeEvent *_event)
{
  if (this->scene())
  {
    this->scene()->setSceneRect(0, 0, _event->size().width(),
                                      _event->size().height());

    if (!this->imageItem)
    {
      if (!this->gridLines)
      {
        this->gridLines = new GridLines(_event->size().width(),
            _event->size().height());
        this->scene()->addItem(this->gridLines);

        this->noImageText = new QGraphicsTextItem;
        this->noImageText->setPlainText("No image selected");
        this->noImageText->setDefaultTextColor(Qt::gray);
        this->scene()->addItem(this->noImageText);
      }
      else
      {
        this->gridLines->SetSize(_event->size().width(),
              _event->size().height());
      }
    }
    else
    {
      this->scene()->removeItem(this->imageItem);
      this->imageItem = new QGraphicsPixmapItem(this->imagePixmap->scaled(
          this->scene()->sceneRect().width(),
          this->scene()->sceneRect().height(), Qt::KeepAspectRatio));
      this->scene()->addItem(this->imageItem);

      if (this->measureItem)
      {
        double scaleWidth = this->imageItem->pixmap().width() /
            static_cast<double>(this->pixmapWidthPx);
        double scaleHeight = this->imageItem->pixmap().height() /
            static_cast<double>(this->pixmapHeightPx);

        LineSegmentItem *segment = this->measureItem->GetSegment(0);
        QPointF p1 = segment->mapToScene(segment->line().p1());
        QPointF p2 = segment->mapToScene(segment->line().p2());

        p1.setX(p1.x() * scaleWidth);
        p2.setX(p2.x() * scaleWidth);
        p1.setY(p1.y() * scaleHeight);
        p2.setY(p2.y() * scaleHeight);

        this->measureItem->SetVertexPosition(0, p1);
        this->measureItem->SetVertexPosition(1, p2);
      }
    }

    if (this->imageItem)
    {
      this->pixmapWidthPx = this->imageItem->pixmap().width();
      this->pixmapHeightPx = this->imageItem->pixmap().height();
    }
  }
}

/////////////////////////////////////////////////
void ImportImageView::mouseMoveEvent(QMouseEvent *_event)
{
  if (this->drawInProgress && this->measureItem)
  {
    QPointF p2 = this->mapToScene(_event->pos());

    if (!(QApplication::keyboardModifiers() & Qt::ShiftModifier))
    {
      // snap to 0/90/180 degrees
      LineSegmentItem *segment = this->measureItem->GetSegment(
        this->measureItem->GetSegmentCount()-1);
      QPointF p1 = segment->mapToScene(segment->line().p1());
      QLineF line(p1, p2);
      double angle = line.angle();
      double range = 10;
      if ((angle < range) || (angle > (360 - range)) ||
          ((angle > (180 - range)) && (angle < (180 + range))))
      {
        p2 = QPointF(p2.x(), p1.y());
      }
      else if (((angle > (90 - range)) && (angle < (90 + range))) ||
          ((angle > (270 - range)) && (angle < (270 + range))))
      {
        p2 = QPointF(p1.x(), p2.y());
      }
    }

    this->measureItem->SetVertexPosition(
        this->measureItem->GetVertexCount()-1, p2);
  }

  if (!drawInProgress)
  {
    QGraphicsView::mouseMoveEvent(_event);
  }
}

/////////////////////////////////////////////////
void ImportImageView::mouseReleaseEvent(QMouseEvent *_event)
{
  if (this->imageItem)
  {
    this->DrawMeasure(_event->pos());
  }

  if (!this->drawInProgress)
  {
    this->currentMouseItem = NULL;
  }

  QGraphicsView::mouseReleaseEvent(_event);
}

/////////////////////////////////////////////////
void ImportImageView::keyPressEvent(QKeyEvent *_event)
{
  if (_event->key() == Qt::Key_Escape)
  {
    this->drawInProgress = false;
    if (this->measureItem)
    {
      this->scene()->removeItem(this->measureItem);
    }
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
    return;
  }
  _event->ignore();
}

/////////////////////////////////////////////////
void ImportImageView::DrawMeasure(const QPoint &_pos)
{
  if (!this->drawDistanceEnabled)
    return;

  if (!this->drawInProgress)
  {
    if (this->measureItem)
    {
      this->scene()->removeItem(this->measureItem);
    }

    QApplication::setOverrideCursor(QCursor(Qt::CrossCursor));

    QPointF pointStart = mapToScene(_pos);
    QPointF pointEnd = pointStart + QPointF(1, 0);

    this->measureItem = new MeasureItem(pointStart, pointEnd);
    this->measureItem->SetValue(this->parent->distanceSpin->value());
    this->scene()->addItem(this->measureItem);
    this->currentMouseItem = this->measureItem;
    this->drawInProgress = true;
  }
  else
  {
    this->measureItem = dynamic_cast<MeasureItem *>(this->currentMouseItem);
    if (this->measureItem)
    {
      LineSegmentItem *segment = this->measureItem->GetSegment(
          this->measureItem->GetSegmentCount()-1);
      this->measureItem->AddPoint(segment->mapToScene(segment->line().p2())
          + QPointF(1, 0));

      this->measureItem->PopEndPoint();

      this->measureScenePx = this->measureItem->GetDistance();

      this->parent->distanceSpin->setButtonSymbols(
          QAbstractSpinBox::UpDownArrows);
      this->parent->distanceSpin->setReadOnly(false);
    }

    // Calculate distance
    double distanceImage = this->measureScenePx * this->imageWidthPx
        / this->pixmapWidthPx;
    this->parent->resolutionSpin->setValue(
        distanceImage / this->parent->distanceSpin->value());

    this->currentMouseItem = NULL;
    this->drawInProgress = false;
    this->releaseKeyboard();
    this->parent->distanceSpin->setFocus();
    this->parent->distanceSpin->selectAll();
    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
  }
}

/////////////////////////////////////////////////
void ImportImageView::RefreshDistance(double _distance)
{
  this->measureItem->SetValue(_distance);
  this->scene()->update();
}

/////////////////////////////////////////////////
void ImportImageView::EnableDrawDistance(bool _enable)
{
  this->drawDistanceEnabled = _enable;
}
