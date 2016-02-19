/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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

#ifndef _GAZEBO_TEST_INTEGRATION_MODELEDITORUNDO_HH_
#define _GAZEBO_TEST_INTEGRATION_MODELEDITORUNDO_HH_

#include "gazebo/gui/QTestFixture.hh"

// FIXME: Tests only pass if ran individually, issue #1861

/// \brief A test class for undo / redo link insertion in the model editor.
class ModelEditorUndoTest : public QTestFixture
{
  Q_OBJECT

  /// \brief Test undo/redo link insertion using the mouse.
  private slots: void LinkInsertionByMouse();

  /// \brief Test undo/redo link deletion via the right-click context menu.
  private slots: void LinkDeletionByContextMenu();

  /// \brief Helper callback to trigger the delete action on the context
  /// menu after the menu, which is modal, has been opened.
  private slots: void TriggerDelete();
};

#endif

