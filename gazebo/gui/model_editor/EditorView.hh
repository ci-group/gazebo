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

#ifndef _EDITOR_VIEW_HH_
#define _EDITOR_VIEW_HH_

#include "gui/qt.h"
#include "common/Event.hh"

namespace gazebo
{
  namespace gui
  {
    class EditorItem;
    class WindowItem;
    class StairsItem;
    class DoorItem;
    class WallItem;
    class BuildingMaker;

    class EditorView : public QGraphicsView
    {
      Q_OBJECT

      public: EditorView(QWidget *_parent = 0);

      public: ~EditorView();

      public: enum modelTypes{NONE, WALL, WINDOW, DOOR, STAIRS};

      public: enum mouseActions{Select, Translate, Rotate};

      public: void CreateItem3D(EditorItem *item);

      private: void contextMenuEvent(QContextMenuEvent *event);

      private: void mousePressEvent(QMouseEvent *_event);

      private: void mouseReleaseEvent(QMouseEvent *_event);

      private: void mouseMoveEvent(QMouseEvent *_event);

      private: void mouseDoubleClickEvent(QMouseEvent *_event);

      private: void keyPressEvent(QKeyEvent *_event);

      private: void DrawWall(QPoint _pos);

      private: void DrawWindow(QPoint _pos);

      private: void DrawDoor(QPoint _pos);

      private: void DrawStairs(QPoint _pos);

      private: void OnCreateEditorItem(const std::string &_type);

      private: void OnFinishModel(const std::string &_modelName,
          const std::string &_savePath);

      private: void OnAddLevel(int _newLevel, std::string _levelName);

      private: void OnChangeLevel(int _level);

      private slots: void OnOpenLevelInspector();

      private: int drawMode;

      private: int mouseMode;

      private: bool drawInProgress;

      private: std::list<WallItem*> wallList;

      private: std::list<WindowItem*> windowList;

      private: std::list<DoorItem*> doorList;

      private: std::list<StairsItem*> stairsList;

      private: std::map<EditorItem *, std::string> itemToModelMap;

      private: QPoint lastLineCornerPos;

      private: std::vector<event::ConnectionPtr> connections;

      private: QGraphicsItem *currentMouseItem;

      private: QGraphicsItem *currentSelectedItem;

      private: BuildingMaker *buildingMaker;

      private: std::string lastWallSegmentName;

      private: int currentLevel;

      private: std::map<int, double> levelHeights;

      private: std::map<int, std::string> levelNames;

      private: QAction *openLevelInspectorAct;

      private: double grabberDragRotation;


    };
  }
}

#endif
