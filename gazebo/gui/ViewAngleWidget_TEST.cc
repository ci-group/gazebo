/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#include "gazebo/math/Helpers.hh"

#include "gazebo/gui/MainWindow.hh"
#include "gazebo/gui/GuiIface.hh"

#include "gazebo/gui/ViewAngleWidget.hh"
#include "gazebo/gui/ViewAngleWidget_TEST.hh"

/////////////////////////////////////////////////
void ViewAngleWidget_TEST::EmptyWorld()
{
  this->resMaxPercentChange = 5.0;
  this->shareMaxPercentChange = 2.0;

  this->Load("worlds/empty.world");

  // Create the main window.
  gazebo::gui::MainWindow *mainWindow = new gazebo::gui::MainWindow();
  QVERIFY(mainWindow != NULL);
  mainWindow->Load();
  mainWindow->Init();
  mainWindow->show();

  // Process some events and draw the screen
  for (size_t i = 0; i < 10; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Get camera
  gazebo::rendering::UserCameraPtr cam = gazebo::gui::get_active_camera();
  QVERIFY(cam != NULL);

  // Get camera default pose and check that's the current pose
  gazebo::math::Pose defaultPose = cam->GetDefaultPose();
  QVERIFY(defaultPose == cam->GetWorldPose());

  // Get the camera distance from the world origin (zoom level)
  double dist = defaultPose.pos.GetLength();

  // Get the view angle widget
  gazebo::gui::ViewAngleWidget *viewAngleWidget =
      mainWindow->findChild<gazebo::gui::ViewAngleWidget *>("viewAngleWidget");

  QApplication::postEvent( viewAngleWidget, new QShowEvent() );

  // Process some events and draw the screen
  for (size_t i = 0; i < 10; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Get buttons
  QList<QToolButton *> buttons =
      viewAngleWidget->findChildren<QToolButton *>();
  QVERIFY(buttons.size() == 7u);

  // Get slider
  QList<QSlider *> sliders =
      viewAngleWidget->findChildren<QSlider *>();
  QVERIFY(sliders.size() == 1u);

  // Check that the slider matches the distance
  QVERIFY(gazebo::math::equal(sliders[0]->sliderPosition(),
      static_cast<int>(dist)));

  // Trigger the top view button
  buttons[0]->click();

  // Process some events and draw the screen
  for (size_t i = 0; i < 100; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Check the camera position
  double tol = 1;
  gazebo::math::Pose pose = cam->GetWorldPose();
  QVERIFY((pose.pos - gazebo::math::Vector3(0, 0, dist)).GetLength() < tol);

  // Trigger the bottom view button
  buttons[1]->click();

  // Process some events and draw the screen
  for (size_t i = 0; i < 100; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Check the camera position
  pose = cam->GetWorldPose();
  QVERIFY((pose.pos - gazebo::math::Vector3(0, 0, -dist)).GetLength() < tol);

  // Trigger the front view button
  buttons[2]->click();

  // Process some events and draw the screen
  for (size_t i = 0; i < 100; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Check the camera position
  pose = cam->GetWorldPose();
  QVERIFY((pose.pos - gazebo::math::Vector3(dist, 0, 0)).GetLength() < tol);

  // Trigger the back view button
  buttons[3]->click();

  // Process some events and draw the screen
  for (size_t i = 0; i < 100; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Check the camera position
  pose = cam->GetWorldPose();
  QVERIFY((pose.pos - gazebo::math::Vector3(-dist, 0, 0)).GetLength() < tol);

  // Trigger the right view button
  buttons[4]->click();

  // Process some events and draw the screen
  for (size_t i = 0; i < 100; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Check the camera position
  pose = cam->GetWorldPose();
  QVERIFY((pose.pos - gazebo::math::Vector3(0, dist, 0)).GetLength() < tol);

  // Trigger the left view button
  buttons[5]->click();

  // Process some events and draw the screen
  for (size_t i = 0; i < 100; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Check the camera position
  pose = cam->GetWorldPose();
  QVERIFY((pose.pos - gazebo::math::Vector3(0, -dist, 0)).GetLength() < tol);

  // Trigger the reset view button
  buttons[6]->click();

  // Process some events and draw the screen
  for (size_t i = 0; i < 100; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Check the camera position
  pose = cam->GetWorldPose();
  QVERIFY((pose.pos - defaultPose.pos).GetLength() < tol);

  // Clean up
  cam->Fini();
  mainWindow->close();
  delete mainWindow;
}

// Generate a main function for the test
QTEST_MAIN(ViewAngleWidget_TEST)
