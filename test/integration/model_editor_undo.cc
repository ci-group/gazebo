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

#include "gazebo/common/SystemPaths.hh"

#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/GLWidget.hh"
#include "gazebo/gui/GuiIface.hh"
#include "gazebo/gui/GuiEvents.hh"
#include "gazebo/gui/InsertModelWidget.hh"
#include "gazebo/gui/MainWindow.hh"
#include "gazebo/gui/model/ModelCreator.hh"
#include "gazebo/gui/model/ModelEditorEvents.hh"

#include "model_editor_undo.hh"

#include "test_config.h"

/////////////////////////////////////////////////
void ModelEditorUndoTest::LinkInsertionByMouse()
{
  this->resMaxPercentChange = 5.0;
  this->shareMaxPercentChange = 2.0;

  this->Load("worlds/empty.world", false, false, false);

  // Create the main window.
  auto mainWindow = new gazebo::gui::MainWindow();
  QVERIFY(mainWindow != NULL);
  mainWindow->Load();
  mainWindow->Init();
  mainWindow->show();

  this->ProcessEventsAndDraw(mainWindow);

  // Get the user camera, scene and GLWidget
  auto cam = gazebo::gui::get_active_camera();
  QVERIFY(cam != NULL);
  auto scene = cam->GetScene();
  QVERIFY(scene != NULL);
  auto glWidget = mainWindow->findChild<gazebo::gui::GLWidget *>("GLWidget");
  QVERIFY(glWidget != NULL);

  // Get the cylinder button on the model editor palette
  auto cylinderButton = mainWindow->findChild<QToolButton *>(
      "modelEditorPaletteCylinderButton");
  QVERIFY(cylinderButton != NULL);
  QVERIFY(!cylinderButton->isChecked());
  QVERIFY(!cylinderButton->isVisible());

  // Enter the model editor
  QVERIFY(gazebo::gui::g_editModelAct != NULL);
  gazebo::gui::g_editModelAct->trigger();
  QVERIFY(cylinderButton->isVisible());

  // Check undo/redo are disabled
  QVERIFY(gazebo::gui::g_undoAct != NULL);
  QVERIFY(gazebo::gui::g_undoHistoryAct != NULL);
  QVERIFY(gazebo::gui::g_redoAct != NULL);
  QVERIFY(gazebo::gui::g_redoHistoryAct != NULL);
  QVERIFY(!gazebo::gui::g_undoAct->isEnabled());
  QVERIFY(!gazebo::gui::g_undoHistoryAct->isEnabled());
  QVERIFY(!gazebo::gui::g_redoAct->isEnabled());
  QVERIFY(!gazebo::gui::g_redoHistoryAct->isEnabled());

  // Press the cylinder button to start inserting a link
  cylinderButton->click();
  QVERIFY(cylinderButton->isChecked());

  // Press the mouse in the scene to finish inserting a link
  QTest::mouseRelease(glWidget, Qt::LeftButton, 0,
      QPoint(-mainWindow->width()*0.5, -mainWindow->height()*0.5));

  this->ProcessEventsAndDraw(mainWindow);

  // Check the cylinder button is not pressed anymore
  QVERIFY(!cylinderButton->isChecked());

  // Undo -> Redo a few times
  std::string insertedLinkDefaultName = "ModelPreview_0::link_0";
  for (unsigned int j = 0; j < 3; ++j)
  {
    // Check undo is enabled
    QVERIFY(gazebo::gui::g_undoAct->isEnabled());
    QVERIFY(gazebo::gui::g_undoHistoryAct->isEnabled());
    QVERIFY(!gazebo::gui::g_redoAct->isEnabled());
    QVERIFY(!gazebo::gui::g_redoHistoryAct->isEnabled());

    // Check the link has been inserted
    auto link = scene->GetVisual(insertedLinkDefaultName);
    QVERIFY(link != NULL);

    // Undo
    gzmsg << "Undo inserting [" << insertedLinkDefaultName << "]" << std::endl;
    gazebo::gui::g_undoAct->trigger();

    // Check redo is enabled
    QVERIFY(!gazebo::gui::g_undoAct->isEnabled());
    QVERIFY(!gazebo::gui::g_undoHistoryAct->isEnabled());
    QVERIFY(gazebo::gui::g_redoAct->isEnabled());
    QVERIFY(gazebo::gui::g_redoHistoryAct->isEnabled());

    // Check the link has been removed
    link = scene->GetVisual(insertedLinkDefaultName);
    QVERIFY(link == NULL);

    // Redo
    gzmsg << "Redo inserting [" << insertedLinkDefaultName << "]" << std::endl;
    gazebo::gui::g_redoAct->trigger();
  }

  mainWindow->close();
  delete mainWindow;
  mainWindow = NULL;
}

/////////////////////////////////////////////////
void ModelEditorUndoTest::LinkDeletionByContextMenu()
{
  this->resMaxPercentChange = 5.0;
  this->shareMaxPercentChange = 2.0;

  // Load a world with simple shape models
  this->Load("worlds/shapes.world", false, false, false);

  // Create the main window.
  auto mainWindow = new gazebo::gui::MainWindow();
  QVERIFY(mainWindow != NULL);
  mainWindow->Load();
  mainWindow->Init();
  mainWindow->show();

  this->ProcessEventsAndDraw(mainWindow);

  // Get the user camera, scene and GLWidget
  auto cam = gazebo::gui::get_active_camera();
  QVERIFY(cam != NULL);
  auto scene = cam->GetScene();
  QVERIFY(scene != NULL);

  // Enter the model editor to edit the box model
  QVERIFY(gazebo::gui::g_editModelAct != NULL);
  gazebo::gui::g_editModelAct->trigger();
  gazebo::gui::Events::editModel("box");

  // Check undo/redo are disabled
  QVERIFY(gazebo::gui::g_undoAct != NULL);
  QVERIFY(gazebo::gui::g_undoHistoryAct != NULL);
  QVERIFY(gazebo::gui::g_redoAct != NULL);
  QVERIFY(gazebo::gui::g_redoHistoryAct != NULL);
  QVERIFY(!gazebo::gui::g_undoAct->isEnabled());
  QVERIFY(!gazebo::gui::g_undoHistoryAct->isEnabled());
  QVERIFY(!gazebo::gui::g_redoAct->isEnabled());
  QVERIFY(!gazebo::gui::g_redoHistoryAct->isEnabled());

  // Check the visuals have been created inside the editor
  std::string linkVisName = "ModelPreview_1::link";
  auto link = scene->GetVisual(linkVisName);
  QVERIFY(link != NULL);

  // Open the context menu and trigger the delete action
  QTimer::singleShot(500, this, SLOT(TriggerDelete()));
  gazebo::gui::model::Events::showLinkContextMenu(linkVisName);

  // Undo -> Redo a few times
  for (unsigned int j = 0; j < 3; ++j)
  {
    this->ProcessEventsAndDraw(mainWindow);

    // Check undo is enabled
    QVERIFY(gazebo::gui::g_undoAct->isEnabled());
    QVERIFY(gazebo::gui::g_undoHistoryAct->isEnabled());
    QVERIFY(!gazebo::gui::g_redoAct->isEnabled());
    QVERIFY(!gazebo::gui::g_redoHistoryAct->isEnabled());

    // Check the link has been deleted
    link = scene->GetVisual(linkVisName);
    QVERIFY(link == NULL);

    // Undo
    gzmsg << "Undo deleting [" << linkVisName << "]" << std::endl;
    gazebo::gui::g_undoAct->trigger();

    this->ProcessEventsAndDraw(mainWindow);

    // Check redo is enabled
    QVERIFY(!gazebo::gui::g_undoAct->isEnabled());
    QVERIFY(!gazebo::gui::g_undoHistoryAct->isEnabled());
    QVERIFY(gazebo::gui::g_redoAct->isEnabled());
    QVERIFY(gazebo::gui::g_redoHistoryAct->isEnabled());

    // Check the link has been inserted
    link = scene->GetVisual(linkVisName);
    QVERIFY(link != NULL);

    // Redo
    gzmsg << "Redo deleting [" << linkVisName << "]" << std::endl;
    gazebo::gui::g_redoAct->trigger();
  }

  mainWindow->close();
  delete mainWindow;
  mainWindow = NULL;
}

/////////////////////////////////////////////////
void ModelEditorUndoTest::NestedModelInsertionByMouse()
{
  this->resMaxPercentChange = 5.0;
  this->shareMaxPercentChange = 2.0;

  this->Load("worlds/empty.world", false, false, false);

  // Create the main window.
  auto mainWindow = new gazebo::gui::MainWindow();
  QVERIFY(mainWindow != NULL);
  mainWindow->Load();
  mainWindow->Init();
  mainWindow->show();

  this->ProcessEventsAndDraw(mainWindow);

  // Get the user camera, scene and GLWidget
  auto cam = gazebo::gui::get_active_camera();
  QVERIFY(cam != NULL);
  auto scene = cam->GetScene();
  QVERIFY(scene != NULL);
  auto glWidget = mainWindow->findChild<gazebo::gui::GLWidget *>("GLWidget");
  QVERIFY(glWidget != NULL);

  // Add the test model database to the insert tab
  gazebo::common::SystemPaths::Instance()->AddModelPathsUpdate(
      CMAKE_SOURCE_DIR "/test/models/testdb");

  // Get the insert model widget
  auto insertModelWidget = mainWindow->findChild<
      gazebo::gui::InsertModelWidget *>("insertModel");
  QVERIFY(insertModelWidget != NULL);

  // Get the items in the list
  auto tree = insertModelWidget->findChildren<QTreeWidget *>();
  QVERIFY(tree.size() == 1u);
  auto cococanItem = tree[0]->findItems(QString("cococan"),
      Qt::MatchContains | Qt::MatchRecursive);
  QVERIFY(cococanItem.size() > 0);

  // Enter the model editor
  QVERIFY(gazebo::gui::g_editModelAct != NULL);
  gazebo::gui::g_editModelAct->trigger();

  // Check undo/redo are disabled
  QVERIFY(gazebo::gui::g_undoAct != NULL);
  QVERIFY(gazebo::gui::g_undoHistoryAct != NULL);
  QVERIFY(gazebo::gui::g_redoAct != NULL);
  QVERIFY(gazebo::gui::g_redoHistoryAct != NULL);
  QVERIFY(!gazebo::gui::g_undoAct->isEnabled());
  QVERIFY(!gazebo::gui::g_undoHistoryAct->isEnabled());
  QVERIFY(!gazebo::gui::g_redoAct->isEnabled());
  QVERIFY(!gazebo::gui::g_redoHistoryAct->isEnabled());

  // Trigger signal as if item was clicked
  QMetaObject::invokeMethod(tree[0], "itemClicked", Q_ARG(QTreeWidgetItem *,
      cococanItem[0]), Q_ARG(int, 0));

  // Press the mouse in the scene to finish inserting a model
  QTest::mouseRelease(glWidget, Qt::LeftButton, 0,
      QPoint(-mainWindow->width()*0.5, -mainWindow->height()*0.5));

  this->ProcessEventsAndDraw(mainWindow);

  // Undo -> Redo a few times
  std::string insertedModelName = "ModelPreview_0::cococan";
  for (unsigned int j = 0; j < 3; ++j)
  {
    // Check undo is enabled
    QVERIFY(gazebo::gui::g_undoAct->isEnabled());
    QVERIFY(gazebo::gui::g_undoHistoryAct->isEnabled());
    QVERIFY(!gazebo::gui::g_redoAct->isEnabled());
    QVERIFY(!gazebo::gui::g_redoHistoryAct->isEnabled());

    // Check the model has been inserted
    auto link = scene->GetVisual(insertedModelName);
    QVERIFY(link != NULL);

    // Undo
    gzmsg << "Undo inserting [" << insertedModelName << "]" << std::endl;
    gazebo::gui::g_undoAct->trigger();

    // Check redo is enabled
    QVERIFY(!gazebo::gui::g_undoAct->isEnabled());
    QVERIFY(!gazebo::gui::g_undoHistoryAct->isEnabled());
    QVERIFY(gazebo::gui::g_redoAct->isEnabled());
    QVERIFY(gazebo::gui::g_redoHistoryAct->isEnabled());

    // Check the model has been removed
    link = scene->GetVisual(insertedModelName);
    QVERIFY(link == NULL);

    // Redo
    gzmsg << "Redo inserting [" << insertedModelName << "]" << std::endl;
    gazebo::gui::g_redoAct->trigger();
  }

  mainWindow->close();
  delete mainWindow;
  mainWindow = NULL;
}

/////////////////////////////////////////////////
void ModelEditorUndoTest::NestedModelDeletionByContextMenu()
{
  this->resMaxPercentChange = 5.0;
  this->shareMaxPercentChange = 2.0;

  // Load a world with a deeply nested model
  this->Load("test/worlds/deeply_nested_models.world", false, false, false);

  // Create the main window.
  auto mainWindow = new gazebo::gui::MainWindow();
  QVERIFY(mainWindow != NULL);
  mainWindow->Load();
  mainWindow->Init();
  mainWindow->show();

  this->ProcessEventsAndDraw(mainWindow);

  // Get the user camera, scene and GLWidget
  auto cam = gazebo::gui::get_active_camera();
  QVERIFY(cam != NULL);
  auto scene = cam->GetScene();
  QVERIFY(scene != NULL);

  // Enter the model editor to edit the nested model
  QVERIFY(gazebo::gui::g_editModelAct != NULL);
  gazebo::gui::g_editModelAct->trigger();
  gazebo::gui::Events::editModel("model_00");

  // Check undo/redo are disabled
  QVERIFY(gazebo::gui::g_undoAct != NULL);
  QVERIFY(gazebo::gui::g_undoHistoryAct != NULL);
  QVERIFY(gazebo::gui::g_redoAct != NULL);
  QVERIFY(gazebo::gui::g_redoHistoryAct != NULL);
  QVERIFY(!gazebo::gui::g_undoAct->isEnabled());
  QVERIFY(!gazebo::gui::g_undoHistoryAct->isEnabled());
  QVERIFY(!gazebo::gui::g_redoAct->isEnabled());
  QVERIFY(!gazebo::gui::g_redoHistoryAct->isEnabled());

  this->ProcessEventsAndDraw(mainWindow);

  // Check the visuals have been created inside the editor
  std::vector<std::string> visNames;
  visNames.push_back("ModelPreview_1::model_01");
  visNames.push_back("ModelPreview_1::model_01::model_02");
  visNames.push_back("ModelPreview_1::model_01::model_02::model_03");
  for (auto name : visNames)
    QVERIFY(scene->GetVisual(name) != NULL);

  // Open the context menu and trigger the delete action
  QTimer::singleShot(500, this, SLOT(TriggerDelete()));
  gazebo::gui::model::Events::showLinkContextMenu(visNames[0]);

  // Undo -> Redo a few times
  for (unsigned int j = 0; j < 3; ++j)
  {
    this->ProcessEventsAndDraw(mainWindow);

    // Check undo is enabled
    QVERIFY(gazebo::gui::g_undoAct->isEnabled());
    QVERIFY(gazebo::gui::g_undoHistoryAct->isEnabled());
    QVERIFY(!gazebo::gui::g_redoAct->isEnabled());
    QVERIFY(!gazebo::gui::g_redoHistoryAct->isEnabled());

    // Check the model has been deleted
    for (auto name : visNames)
      QVERIFY(scene->GetVisual(name) == NULL);

    // Undo
    gzmsg << "Undo deleting [" << visNames[0] << "]" << std::endl;
    gazebo::gui::g_undoAct->trigger();

    this->ProcessEventsAndDraw(mainWindow);

    // Check redo is enabled
    QVERIFY(!gazebo::gui::g_undoAct->isEnabled());
    QVERIFY(!gazebo::gui::g_undoHistoryAct->isEnabled());
    QVERIFY(gazebo::gui::g_redoAct->isEnabled());
    QVERIFY(gazebo::gui::g_redoHistoryAct->isEnabled());

    // Check the model has been inserted
    for (auto name : visNames)
      QVERIFY(scene->GetVisual(name) != NULL);

    // Redo
    gzmsg << "Redo deleting [" << visNames[0] << "]" << std::endl;
    gazebo::gui::g_redoAct->trigger();
  }

  mainWindow->close();
  delete mainWindow;
  mainWindow = NULL;
}

/////////////////////////////////////////////////
void ModelEditorUndoTest::TriggerDelete()
{
  auto allToplevelWidgets = QApplication::topLevelWidgets();
  for (auto widget : allToplevelWidgets)
  {
    if (widget->inherits("QMenu") &&
        widget->objectName() == "ModelEditorContextMenu")
    {
      auto context = qobject_cast<QMenu *>(widget);
      QVERIFY(context != NULL);

      // This only works as long as the Delete action is the last one in the
      // menu
      QTest::mouseClick(context, Qt::LeftButton, 0, QPoint(10,
          context->height() - 10));

      return;
    }
  }
}

// Generate a main function for the test
QTEST_MAIN(ModelEditorUndoTest)

