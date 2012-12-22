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

#ifndef _LINE_SEGMENT_ITEM_HH_
#define _LINE_SEGMENT_ITEM_HH_

#include <gui/qt.h>
#include <gui/model_editor/EditorItem.hh>

namespace gazebo
{
  namespace gui
  {

    class EditorItem;

    class LineSegmentItem : public EditorItem, public QGraphicsLineItem
    {
      public: LineSegmentItem(QGraphicsItem *_parent = 0, int _index = 0);

      public: ~LineSegmentItem();

      public: void SetLine(QPointF _start, QPointF _end);

      public: void SetStartPoint(QPointF _start);

      public: void SetEndPoint(QPointF _end);

      public: int GetIndex();

      public: QVector3D GetSize();

      public: QVector3D GetScenePosition();

      public: double GetSceneRotation();

      /// \brief Set the current mouse state
      public: void SetMouseState(int _state);

      /// \brief Retrieve the current mouse state
      public: int  GetMouseState();

      public: void SetMouseDownX(double _x);

      public: void SetMouseDownY(double _y);

      public: double GetMouseDownX();

      public: double GetMouseDownY();

      public: void Update();

      private: void hoverEnterEvent(QGraphicsSceneHoverEvent *_event);

      private: void hoverLeaveEvent(QGraphicsSceneHoverEvent *_event);

      private: void mouseReleaseEvent(QGraphicsSceneMouseEvent *_event);

      private: void mouseMoveEvent(QGraphicsSceneMouseEvent *_event);

      private: void mousePressEvent(QGraphicsSceneMouseEvent *_event);

      private: void LineChanged();

      private: int index;

      private: QPointF start;

      private: QPointF end;

      private: int mouseButtonState;

      private: double mouseDownX;

      private: double mouseDownY;
    };
  }
}

#endif
