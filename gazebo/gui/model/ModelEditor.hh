/*
 * Copyright 2013 Open Source Robotics Foundation
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
#ifndef _MODEL_EDITOR_HH_
#define _MODEL_EDITOR_HH_

#include <string>

#include "gazebo/gui/qt.h"
#include "gazebo/gui/Editor.hh"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace gui
  {
    class ModelEditorPalette;

    /// \class ModelEditor ModelEditor.hh gui/gui.hh
    /// \brief Interface to the terrain editor.
    class GAZEBO_VISIBLE ModelEditor : public Editor
    {
      Q_OBJECT

      /// \brief Constuctor.
      /// \param[in] _mainWindow Pointer to the mainwindow.
      public: ModelEditor(MainWindow *_mainWindow);

      /// \brief Destuctor.
      public: virtual ~ModelEditor();

      /// \brief QT callback when entering building edit mode
      /// \param[in] _checked True if the menu item is checked
      private slots: void OnEdit(bool _checked);

      /// \brief QT callback when the joint button is clicked.
      private slots: void OnAddSelectedJoint();

      /// \brief QT callback when a joint menu is selected
      /// \param[in] _type Type of joint.
      private slots: void OnAddJoint(const QString &_type);

      /// \brief Qt callback when a joint is added.
      private slots: void OnJointAdded();

      /// \brief Callback when the model has been completed.
      private: void OnFinish();

      /// \brief Toggle main window's toolbar to display model editor icons.
      private: void ToggleToolbar();

      /// \brief Contains all the model editor tools.
      private: ModelEditorPalette *modelPalette;

      /// \brief True if model editor is active.
      private: bool active;

      /// \brief Qt action for selecting and adding a joint in the model editor.
      private: QAction *jointTypeAct;

      /// \brief Qt action for adding a previously selected joint in the
      /// model editor.
      private: QAction *jointAct;

      /// \brief A separator for the joint icon.
      private: QAction *jointSeparatorAct;

      /// \brief Qt tool button associated with the joint action.
      private: QToolButton *jointButton;

      /// \brief Qt signal mapper for mapping add jointsignals.
      private: QSignalMapper *signalMapper;

      /// \brief Previously selected joint type.
      private: std::string selectedJointType;
    };
  }
}
#endif
