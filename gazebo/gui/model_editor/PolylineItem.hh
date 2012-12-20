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

#ifndef _POLYLINE_ITEM_HH_
#define _POLYLINE_ITEM_HH_

#include <gui/qt.h>
#include <gui/model_editor/EditorItem.hh>

namespace gazebo
{
  namespace gui
  {
    class CornerGrabber;

    class LineSegmentItem;

    class PolylineItem : public EditorItem, public QGraphicsPathItem
    {
      public: PolylineItem(QPointF _start, QPointF _end);

      public: ~PolylineItem();

      public: void AddPoint(QPointF _point);

      public: void PopEndPoint();

      public: unsigned int GetVertexCount();

      public: unsigned int GetSegmentCount();

      public: void SetVertexPosition(unsigned int _index, QPointF _pos);

      public: void TranslateVertex(unsigned int _index, QPointF _trans);

      public: LineSegmentItem *GetSegment(unsigned int _index);

      public: void ShowCorners(bool _show);

      public: void SetThickness(double _thickness);

      public: void SetPosition(QPointF _pos);

      private: void UpdatePath();

      private: void UpdatePathAtIndex(unsigned int _index, QPointF _pos);

      private: void UpdatePathAt(unsigned int _index, QPointF _pos);

      private: void AppendToPath(QPointF _point);

      private: bool sceneEventFilter(QGraphicsItem * watched,
        QEvent *_event);

      private: virtual bool cornerEventFilter(CornerGrabber *_corner,
          QEvent *_event);

      private: virtual bool segmentEventFilter(LineSegmentItem *_item,
          QEvent *_event);

      private: void hoverEnterEvent(QGraphicsSceneHoverEvent *_event);

      private: void hoverLeaveEvent(QGraphicsSceneHoverEvent *_event);

      private: void mouseReleaseEvent(QGraphicsSceneMouseEvent *_event);

      private: void mouseMoveEvent(QGraphicsSceneMouseEvent *_event);

      private: void mousePressEvent(QGraphicsSceneMouseEvent *_event);

      private: QVariant itemChange(GraphicsItemChange _change,
        const QVariant &_value);

      private: void DrawBoundingBox(QPainter *_painter);

      private: void paint(QPainter *_painter,
          const QStyleOptionGraphicsItem *_option, QWidget *_widget);

      private: QPointF origin;

      private: QPointF location;

      private: int gridSpace;

      protected: std::vector<CornerGrabber*> corners;

      protected: std::vector<LineSegmentItem*> segments;

      private: int cornerWidth;

      private: int cornerHeight;

      protected: QPointF segmentMouseMove;

      private: QColor borderColor;

      private: double lineThickness;
    };
  }
}

#endif
