/*
 * Copyright (C) 2014-2015 Open Source Robotics Foundation
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

#include "gazebo/gui/GLWidget.hh"
#include "gazebo/gui/MainWindow.hh"
#include "gazebo/gui/MouseEventHandler.hh"
#include "gazebo/gui/GuiIface.hh"
#include "gazebo/gui/model/JointMaker.hh"
#include "gazebo/gui/model/JointMaker_TEST.hh"

using namespace gazebo;

/////////////////////////////////////////////////
void JointMaker_TEST::JointState()
{
  this->Load("worlds/empty.world");

  gui::JointMaker *jointMaker = new gui::JointMaker();
  QCOMPARE(jointMaker->GetState(), gui::JointMaker::JOINT_NONE);

  jointMaker->AddJoint(gui::JointMaker::JOINT_HINGE);
  QCOMPARE(jointMaker->GetState(), gui::JointMaker::JOINT_HINGE);

  jointMaker->Reset();
  QCOMPARE(jointMaker->GetState(), gui::JointMaker::JOINT_NONE);

  jointMaker->AddJoint(gui::JointMaker::JOINT_SLIDER);
  QCOMPARE(jointMaker->GetState(), gui::JointMaker::JOINT_SLIDER);

  jointMaker->Stop();
  QCOMPARE(jointMaker->GetState(), gui::JointMaker::JOINT_NONE);

  delete jointMaker;
}

/////////////////////////////////////////////////
void JointMaker_TEST::CreateRemoveJoint()
{
  // FIXME Test passes but segfaults when QTestFixture clean up
  // Problem: JointMaker's destructor resets visual shared_ptrs
  // but this later causes a segfault in Visual's destructor when exiting the
  // program.

  this->resMaxPercentChange = 5.0;
  this->shareMaxPercentChange = 2.0;

  this->Load("worlds/shapes.world", false, false, true);

  gui::JointMaker *jointMaker = new gui::JointMaker();
  QCOMPARE(jointMaker->GetState(), gui::JointMaker::JOINT_NONE);
  QCOMPARE(jointMaker->GetJointCount(), 0u);

  gui::MainWindow *mainWindow = new gui::MainWindow();
  QVERIFY(mainWindow != NULL);
  mainWindow->Load();
  mainWindow->Init();
  mainWindow->show();

  // Process some events, and draw the screen
  for (unsigned int i = 0; i < 10; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  rendering::UserCameraPtr cam = gui::get_active_camera();
  Q_ASSERT(cam);
  rendering::ScenePtr scene = cam->GetScene();
  Q_ASSERT(scene);

  rendering::VisualPtr boxLink = scene->GetVisual("box::link");
  rendering::VisualPtr sphereLink = scene->GetVisual("sphere::link");
  rendering::VisualPtr cylinderLink = scene->GetVisual("cylinder::link");

  Q_ASSERT(boxLink.get());
  Q_ASSERT(sphereLink.get());
  Q_ASSERT(cylinderLink.get());

  // Add a revolute joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_HINGE);
  gui::JointData *revoluteJointData =
      jointMaker->CreateJoint(boxLink, sphereLink);
  jointMaker->CreateHotSpot(revoluteJointData);
  QCOMPARE(jointMaker->GetJointCount(), 1u);

  // Add a prismatic joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_SLIDER);
  gui::JointData *prismaticJointData =
      jointMaker->CreateJoint(sphereLink, cylinderLink);
  jointMaker->CreateHotSpot(prismaticJointData);
  QCOMPARE(jointMaker->GetJointCount(), 2u);

  // Add a screw joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_SCREW);
  gui::JointData *screwJointData =
      jointMaker->CreateJoint(cylinderLink, boxLink);
  jointMaker->CreateHotSpot(screwJointData);
  QCOMPARE(jointMaker->GetJointCount(), 3u);

  // Remove the screw joint
  jointMaker->RemoveJoint(screwJointData->hotspot->GetName());
  QCOMPARE(jointMaker->GetJointCount(), 2u);

  // Add a ball joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_BALL);
  gui::JointData *ballJointData =
      jointMaker->CreateJoint(cylinderLink, boxLink);
  jointMaker->CreateHotSpot(ballJointData);
  QCOMPARE(jointMaker->GetJointCount(), 3u);

  // Remove the two joints connected to the sphere
  jointMaker->RemoveJointsByLink(sphereLink->GetName());
  QCOMPARE(jointMaker->GetJointCount(), 1u);

  // Remove the last joint
  jointMaker->RemoveJoint(ballJointData->hotspot->GetName());
  QCOMPARE(jointMaker->GetJointCount(), 0u);

  // delete jointMaker;
  mainWindow->close();
  delete mainWindow;
}


/////////////////////////////////////////////////
void JointMaker_TEST::ShowJoints()
{
  this->resMaxPercentChange = 5.0;
  this->shareMaxPercentChange = 2.0;

  this->Load("worlds/shapes.world", false, false, false);

  gui::JointMaker *jointMaker = new gui::JointMaker();

  gui::MainWindow *mainWindow = new gui::MainWindow();
  QVERIFY(mainWindow != NULL);
  mainWindow->Load();
  mainWindow->Init();
  mainWindow->show();

  // Process some events, and draw the screen
  for (unsigned int i = 0; i < 10; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  rendering::UserCameraPtr cam = gui::get_active_camera();
  Q_ASSERT(cam);
  rendering::ScenePtr scene = cam->GetScene();
  Q_ASSERT(scene);

  rendering::VisualPtr boxLink = scene->GetVisual("box::link");
  rendering::VisualPtr sphereLink = scene->GetVisual("sphere::link");
  rendering::VisualPtr cylinderLink = scene->GetVisual("cylinder::link");

  Q_ASSERT(boxLink.get());
  Q_ASSERT(sphereLink.get());
  Q_ASSERT(cylinderLink.get());

  // Add a revolute joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_HINGE);
  gui::JointData *revoluteJointData =
      jointMaker->CreateJoint(boxLink, sphereLink);
  jointMaker->CreateHotSpot(revoluteJointData);
  QCOMPARE(jointMaker->GetJointCount(), 1u);

  // Add a prismatic joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_SLIDER);
  gui::JointData *prismaticJointData =
      jointMaker->CreateJoint(sphereLink, cylinderLink);
  jointMaker->CreateHotSpot(prismaticJointData);
  QCOMPARE(jointMaker->GetJointCount(), 2u);

  // Process some events, and draw the screen
  for (unsigned int i = 0; i < 10; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  QVERIFY(revoluteJointData->hotspot != NULL);
  QVERIFY(prismaticJointData->hotspot != NULL);
  QVERIFY(revoluteJointData->jointVisual != NULL);
  QVERIFY(prismaticJointData->jointVisual != NULL);

  // toggle joint visualization and verify
  jointMaker->ShowJoints(false);
  QVERIFY(!revoluteJointData->hotspot->GetVisible());
  QVERIFY(!prismaticJointData->hotspot->GetVisible());
  QVERIFY(!revoluteJointData->jointVisual->GetVisible());
  QVERIFY(!prismaticJointData->jointVisual->GetVisible());

  jointMaker->ShowJoints(true);
  QVERIFY(revoluteJointData->hotspot->GetVisible());
  QVERIFY(prismaticJointData->hotspot->GetVisible());
  QVERIFY(revoluteJointData->jointVisual->GetVisible());
  QVERIFY(prismaticJointData->jointVisual->GetVisible());

  jointMaker->ShowJoints(false);
  QVERIFY(!revoluteJointData->hotspot->GetVisible());
  QVERIFY(!prismaticJointData->hotspot->GetVisible());
  QVERIFY(!revoluteJointData->jointVisual->GetVisible());
  QVERIFY(!prismaticJointData->jointVisual->GetVisible());

  delete jointMaker;
  mainWindow->close();
  delete mainWindow;
}

/////////////////////////////////////////////////
void JointMaker_TEST::Selection()
{
  this->resMaxPercentChange = 5.0;
  this->shareMaxPercentChange = 2.0;

  this->Load("worlds/shapes.world", false, false, false);

  gui::JointMaker *jointMaker = new gui::JointMaker();

  QCOMPARE(jointMaker->GetState(), gui::JointMaker::JOINT_NONE);
  QCOMPARE(jointMaker->GetJointCount(), 0u);

  gui::MainWindow *mainWindow = new gui::MainWindow();
  QVERIFY(mainWindow != NULL);
  mainWindow->Load();
  mainWindow->Init();
  mainWindow->show();

  // Process some events, and draw the screen
  for (unsigned int i = 0; i < 10; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  rendering::UserCameraPtr cam = gui::get_active_camera();
  Q_ASSERT(cam);
  rendering::ScenePtr scene = cam->GetScene();
  Q_ASSERT(scene);

  rendering::VisualPtr boxLink = scene->GetVisual("box::link");
  rendering::VisualPtr sphereLink = scene->GetVisual("sphere::link");
  rendering::VisualPtr cylinderLink = scene->GetVisual("cylinder::link");

  Q_ASSERT(boxLink.get());
  Q_ASSERT(sphereLink.get());
  Q_ASSERT(cylinderLink.get());

  // Add a revolute joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_HINGE);
  gui::JointData *revoluteJointData =
      jointMaker->CreateJoint(boxLink, sphereLink);
  jointMaker->CreateHotSpot(revoluteJointData);
  QCOMPARE(jointMaker->GetJointCount(), 1u);

  // Add a prismatic joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_SLIDER);
  gui::JointData *prismaticJointData =
      jointMaker->CreateJoint(sphereLink, cylinderLink);
  jointMaker->CreateHotSpot(prismaticJointData);
  QCOMPARE(jointMaker->GetJointCount(), 2u);

  // Add a screw joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_SCREW);
  gui::JointData *screwJointData =
      jointMaker->CreateJoint(cylinderLink, boxLink);
  jointMaker->CreateHotSpot(screwJointData);
  QCOMPARE(jointMaker->GetJointCount(), 3u);

  // verify initial selected state
  QVERIFY(!revoluteJointData->hotspot->GetHighlighted());
  QVERIFY(!prismaticJointData->hotspot->GetHighlighted());
  QVERIFY(!screwJointData->hotspot->GetHighlighted());

  // select the joints and verify that they are selected
  jointMaker->SetSelected(revoluteJointData->hotspot, true);
  QVERIFY(revoluteJointData->hotspot->GetHighlighted());

  jointMaker->SetSelected(prismaticJointData->hotspot, true);
  QVERIFY(prismaticJointData->hotspot->GetHighlighted());

  jointMaker->SetSelected(screwJointData->hotspot, true);
  QVERIFY(screwJointData->hotspot->GetHighlighted());

  // deselect and verify
  jointMaker->SetSelected(revoluteJointData->hotspot, false);
  QVERIFY(!revoluteJointData->hotspot->GetHighlighted());

  jointMaker->SetSelected(prismaticJointData->hotspot, false);
  QVERIFY(!prismaticJointData->hotspot->GetHighlighted());

  jointMaker->SetSelected(screwJointData->hotspot, false);
  QVERIFY(!screwJointData->hotspot->GetHighlighted());

  // select one and verify all
  jointMaker->SetSelected(prismaticJointData->hotspot, true);
  QVERIFY(prismaticJointData->hotspot->GetHighlighted());
  QVERIFY(!revoluteJointData->hotspot->GetHighlighted());
  QVERIFY(!screwJointData->hotspot->GetHighlighted());

  delete jointMaker;
  mainWindow->close();
  delete mainWindow;
}

// Generate a main function for the test
QTEST_MAIN(JointMaker_TEST)
