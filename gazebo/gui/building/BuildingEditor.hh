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
#ifndef _BUILDING_EDITOR_HH_
#define _BUILDING_EDITOR_HH_

#include "gazebo/gui/qt.h"
#include "gazebo/gui/Editor.hh"

namespace gazebo
{
  namespace gui
  {
    class BuildingEditorPalette;

    /// \class TerrainEditor TerrainEditor.hh gui/gui.hh
    /// \brief Interface to the terrain editor.
    class BuildingEditor : public Editor
    {
      Q_OBJECT

      /// \brief Constuctor.
      /// \param[in] _mainWindow Pointer to the mainwindow.
      public: BuildingEditor(MainWindow *_mainWindow);

      /// \brief Destuctor.
      public: virtual ~BuildingEditor();

      /// \brief Qt callback when the building editor's save action is
      /// triggered.
      private slots: void Save();

      /// \brief Qt callback when the building editor's discard action is
      /// triggered.
      private slots: void Discard();

      /// \brief Qt callback when the building editor's done action is
      /// triggered.
      private slots: void Done();

      /// \brief Qt callback when the building editor's exit action is
      /// triggered.
      private slots: void Exit();

      /// \brief Callback from the building editor when the building model
      /// has been completed.
      private: void OnFinish();

      /// \brief QT callback when entering building edit mode
      private slots: void OnEdit();

      /// \brief Contains all the building editor tools.
      private: BuildingEditorPalette *buildingPalette;

      /// \brief Our custom menubar
      private: QMenuBar *menuBar;
    };
  }
}
#endif
