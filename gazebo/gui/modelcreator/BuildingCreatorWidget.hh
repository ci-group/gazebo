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

#ifndef _BUILDING_CREATOR_WIDGET_HH_
#define _BUILDING_CREATOR_WIDGET_HH_

#include <string>
#include "gui/qt.h"

#include "common/MouseEvent.hh"
#include "common/Event.hh"

class SelectableLineSegment;

namespace gazebo
{
  namespace gui
  {
    class BuildingCreatorWidget : public QWidget
    {
      Q_OBJECT

      public: BuildingCreatorWidget(QWidget *_parent = 0);

      public: ~BuildingCreatorWidget();

      public: enum modelTypes {None, Wall, Window, Door, Stairs};

      protected: void paintEvent(QPaintEvent *event);

      /*protected: void mousePressEvent(QMouseEvent *_event);

      protected: void mouseReleaseEvent(QGraphicsSceneMouseEvent *_event);

      protected: void mouseMoveEvent(QGraphicsSceneMouseEvent *_event);

      protected: void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *_event);

      private: void DrawLines(QPointF _pos);

      private: int drawMode;

      private: bool drawInProgress;

      private: std::vector<SelectableLineSegment*> lineList;

      private: qreal lastLinePosX;

      private: qreal lastLinePosY;*/

//      private: QGraphicsScene *scene;

//      private: CreatorScene *view;


    };
  }
}

#endif
