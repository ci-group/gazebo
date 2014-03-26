/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
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

#include <map>
#include <string>
#include <vector>
#include "gazebo/gui/qt.h"
#include "gazebo/common/Event.hh"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace gui
  {
    class EditorItem;
    class WindowItem;
    class StairsItem;
    class DoorItem;
    class WallItem;
    class FloorItem;
    class BuildingMaker;
    class LevelInspectorDialog;
    class GridLines;

    /// \class Level EditorView.hh
    /// \brief A convenient structure for storing level information
    class GAZEBO_VISIBLE Level
    {
      /// \brief Level number
      public: int level;

      /// \brief Level name
      public: std::string name;

      /// \brief Level height from ground
      public: double height;
    };

    /// \addtogroup gazebo_gui
    /// \{

    /// \class EditorView EditorView.hh
    /// \brief Control the editor view and manage contents in the editor scene.
    class GAZEBO_VISIBLE EditorView : public QGraphicsView
    {
      Q_OBJECT

      /// \enum DrawModes
      /// \brief Unique identifiers for all drawing modes within the editor.
      public: enum DrawModes {
                  /// \brief None mode
                  NONE,
                  /// \brief Wall mode
                  WALL,
                  /// \brief Window mode
                  WINDOW,
                  /// \brief Door mode
                  DOOR,
                  /// \brief Stairs mode
                  STAIRS
                };

      /// \brief Constructor
      /// \param[in] _parent Parent Widget.
      public: EditorView(QWidget *_parent = 0);

      /// \brief Destructor
      public: ~EditorView();

      /// \brief Create a 3D visual from a 2D editor item.
      public: void Create3DVisual(EditorItem *_item);

      /// \brief Delete an editor item.
      /// \param[in] _item Item to be deleted.
      public: void DeleteItem(EditorItem *_item);

      /// \brief Qt resize event received when the parent widget changes size.
      /// \param[in] _event Qt resize event
      private: void resizeEvent(QResizeEvent *_event);

      /// \brief Qt event received when the editor view is being scrolled.
      /// \param[in] _dx Change in X position of the scrollbar.
      /// \param[in] _dx Change in Y position of the scrollbar.
      private: void scrollContentsBy(int _dx, int _dy);

      /// \brief Qt context menu received on a mouse right click.
      /// \param[in] _event Qt context menu event.
      private: void contextMenuEvent(QContextMenuEvent *_event);

      /// \brief Qt wheel event received when the mouse wheel is being scrolled.
      /// \param[in] _event Qt wheel event.
      private: void wheelEvent(QWheelEvent *_event);

      /// \brief Qt mouse move event.
      /// \param[in] _event Qt mouse event.
      private: void mouseMoveEvent(QMouseEvent *_event);

      /// \brief Qt mouse press event.
      /// \param[in] _event Qt mouse event.
      private: void mousePressEvent(QMouseEvent *_event);

      /// \brief Qt mouse release event.
      /// \param[in] _event Qt mouse event.
      private: void mouseReleaseEvent(QMouseEvent *_event);

      /// \brief Qt mouse double click event.
      /// \param[in] _event Qt mouse event.
      private: void mouseDoubleClickEvent(QMouseEvent *_event);

      /// \brief Qt key press event.
      /// \param[in] _event Qt key event.
      private: void keyPressEvent(QKeyEvent *_event);

      /// \brief Draw a wall in the scene.
      /// \param[in] _pos Start position of the wall in pixel coordinates.
      private: void DrawWall(const QPoint &_pos);

      /// \brief Draw a window in the scene.
      /// \param[in] _pos Scene position in pixel coordinates to draw the
      /// window.
      private: void DrawWindow(const QPoint &_pos);

      /// \brief Draw a door in the scene
      /// \param[in] _pos Scene position in pixel coordinates to draw the
      /// door.
      private: void DrawDoor(const QPoint &_pos);

      /// \brief Draw a staircase in the scene
      /// \param[in] _pos Scene position in pixel coordinates to draw the
      /// staircase.
      private: void DrawStairs(const QPoint &_pos);

      /// \brief Callback triggered when the user chooses to draw an editor item
      /// in the scene
      /// \param[in] _type Type of editor item to be created.
      private: void OnCreateEditorItem(const std::string &_type);

      // private: void OnSaveModel(const std::string &_modelName,
      //     const std::string &_savePath);

      /// \brief Callback received when the model has been completed and
      /// uploaded onto the server.
      private: void OnFinishModel();

      /// \brief Callback received when the model has been discarded.
      private: void OnDiscardModel();

      /// \brief Qt callback when a level is to be added to the building model.
      private slots: void OnAddLevel();

      /// \brief Qt callback when a level is to be deleted from the building
      /// model.
      private slots: void OnDeleteLevel();

      /// \brief Qt callback when changes to a building level are to be applied.
      private slots: void OnLevelApply();

      /// \brief Qt Callback for opening the building level inspector.
      private slots: void OnOpenLevelInspector();

      /// \brief Callback received when a level on a building model is to
      /// be changed.
      private: void OnChangeLevel(int _level);

      /// \brief Delete a level from the building model
      /// \param[in] _level Level number to delete.
      private: void DeleteLevel(int _level);

      /// \brief Cancel the current drawing operation.
      private: void CancelDrawMode();

      /// \brief Current draw mode
      private: int drawMode;

      /// \brief Indicate whether or not a drawing operation is taking place.
      private: bool drawInProgress;

      /// \brief A list of wall items in the scene.
      private: std::vector<WallItem*> wallList;

      /// \brief A list of window items in the scene.
      private: std::vector<WindowItem*> windowList;

      /// \brief A list of door items in the scene.
      private: std::vector<DoorItem*> doorList;

      /// \brief A list of staircase items in the scene.
      private: std::vector<StairsItem*> stairsList;

      /// \brief A list of floor items in the scene.
      private: std::vector<FloorItem*> floorList;

      /// \brief Mapping between 2D editor items to 3D visuals.
      private: std::map<EditorItem *, std::string> itemToVisualMap;

      /// \brief A list of gui editor events connected to this view.
      private: std::vector<event::ConnectionPtr> connections;

      /// \brief Editor item currently attached to the mouse during a drawing
      /// operation.
      private: QGraphicsItem *currentMouseItem;

      /// \brief Currently selected editor item.
      private: QGraphicsItem *currentSelectedItem;

      /// \brief Building maker manages the creation of 3D visuals
      private: BuildingMaker *buildingMaker;

      // private: std::string lastWallSegmentName;

      /// \brief Current building level associated to the view.
      private: int currentLevel;

      /// \brief A list of building levels in the scene.
      private: std::vector<Level *> levels;

      /// \brief A counter that holds the total number of levels in the building
      /// model.
      private: int levelCounter;

      /// \brief Qt action for opening a building level inspector.
      private: QAction *openLevelInspectorAct;

      /// \brief Qt action for adding a level to the building model.
      private: QAction *addLevelAct;

      /// \brief Qt action for deleting a level from the building model.
      private: QAction *deleteLevelAct;

      /// \brief Rotation in degrees when a mouse is pressed and dragged for
      /// rotating an item.
      private: double mousePressRotation;

      /// \brief Inspector for a building level.
      private: LevelInspectorDialog *levelInspector;

      /// \brief Grid lines drawn on the background of the editor.
      private: GridLines *gridLines;

      /// \brief Scale (zoom level) of the editor view.
      private: double viewScale;

      /// \brief Indicate whether or not the wall can be closed during a draw
      /// wall operation.
      private: bool snapToCloseWall;
    };
    /// \}
  }
}

#endif
