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
#include "gazebo/gui/model/JointInspector.hh"
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

  delete jointMaker;
  mainWindow->close();
  delete mainWindow;
}

/////////////////////////////////////////////////
void JointMaker_TEST::JointDefaultProperties()
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

  // Add a revolute2 joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_HINGE2);
  gui::JointData *revoluteJointData =
      jointMaker->CreateJoint(boxLink, sphereLink);
  jointMaker->CreateHotSpot(revoluteJointData);
  QCOMPARE(jointMaker->GetJointCount(), 1u);

  // verify connected joints
  std::vector<gui::JointData *> boxJointData =
      jointMaker->GetJointDataByLink("box::link");
  QCOMPARE(static_cast<unsigned int>(boxJointData.size()), 1u);

  gui::JointData *rev2joint = boxJointData[0];
  QVERIFY(rev2joint != NULL);
  QVERIFY(rev2joint->inspector != NULL);

  // verify default values
  QVERIFY(msgs::ConvertJointType(rev2joint->jointMsg->type()) == "revolute2");
  QCOMPARE(msgs::ConvertIgn(rev2joint->jointMsg->pose()),
      ignition::math::Pose3d::Zero);
  qFuzzyCompare(rev2joint->jointMsg->cfm(), 0.0);
  qFuzzyCompare(rev2joint->jointMsg->bounce(), 0.0);
  qFuzzyCompare(rev2joint->jointMsg->fudge_factor(), 0.0);
  qFuzzyCompare(rev2joint->jointMsg->limit_cfm(), 0.0);
  qFuzzyCompare(rev2joint->jointMsg->limit_erp(), 0.2);
  qFuzzyCompare(rev2joint->jointMsg->suspension_cfm(), 0.0);
  qFuzzyCompare(rev2joint->jointMsg->suspension_erp(), 0.2);

  msgs::Axis rev2Axis1Msg = rev2joint->jointMsg->axis1();
  QCOMPARE(msgs::ConvertIgn(rev2Axis1Msg.xyz()),
      ignition::math::Vector3d(1, 0, 0));
  qFuzzyCompare(rev2Axis1Msg.limit_lower(), -GZ_DBL_MAX);
  qFuzzyCompare(rev2Axis1Msg.limit_upper(), GZ_DBL_MAX);
  qFuzzyCompare(rev2Axis1Msg.limit_effort(), -1);
  qFuzzyCompare(rev2Axis1Msg.limit_velocity(), -1);
  qFuzzyCompare(rev2Axis1Msg.damping(), 0.0);
  qFuzzyCompare(rev2Axis1Msg.friction(), 0.0);
  QCOMPARE(rev2Axis1Msg.use_parent_model_frame(), false);

  msgs::Axis rev2Axis2Msg = rev2joint->jointMsg->axis2();
  QCOMPARE(msgs::ConvertIgn(rev2Axis2Msg.xyz()),
      ignition::math::Vector3d(0, 1, 0));
  qFuzzyCompare(rev2Axis2Msg.limit_lower(), -GZ_DBL_MAX);
  qFuzzyCompare(rev2Axis2Msg.limit_upper(), GZ_DBL_MAX);
  qFuzzyCompare(rev2Axis2Msg.limit_effort(), -1);
  qFuzzyCompare(rev2Axis2Msg.limit_velocity(), -1);
  qFuzzyCompare(rev2Axis2Msg.damping(), 0.0);
  qFuzzyCompare(rev2Axis2Msg.friction(), 0.0);
  QCOMPARE(rev2Axis2Msg.use_parent_model_frame(), false);

  // Add a prismatic joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_SLIDER);
  gui::JointData *prismaticJointData =
      jointMaker->CreateJoint(sphereLink, cylinderLink);
  jointMaker->CreateHotSpot(prismaticJointData);
  QCOMPARE(jointMaker->GetJointCount(), 2u);

  // verify connected joints
  std::vector<gui::JointData *> sphereJointData =
      jointMaker->GetJointDataByLink("sphere::link");
  QCOMPARE(static_cast<unsigned int>(sphereJointData.size()), 2u);

  std::vector<gui::JointData *> cylinderJointData =
      jointMaker->GetJointDataByLink("cylinder::link");
  QCOMPARE(static_cast<unsigned int>(cylinderJointData.size()), 1u);

  gui::JointData *prisJoint = cylinderJointData[0];
  QVERIFY(prisJoint != NULL);
  QVERIFY(prisJoint->inspector != NULL);

  // verify default values
  QVERIFY(msgs::ConvertJointType(prisJoint->jointMsg->type()) == "prismatic");
  QCOMPARE(msgs::ConvertIgn(prisJoint->jointMsg->pose()),
      ignition::math::Pose3d::Zero);
  qFuzzyCompare(prisJoint->jointMsg->cfm(), 0.0);
  qFuzzyCompare(prisJoint->jointMsg->bounce(), 0.0);
  qFuzzyCompare(prisJoint->jointMsg->fudge_factor(), 0.0);
  qFuzzyCompare(prisJoint->jointMsg->limit_cfm(), 0.0);
  qFuzzyCompare(prisJoint->jointMsg->limit_erp(), 0.2);
  qFuzzyCompare(prisJoint->jointMsg->suspension_cfm(), 0.0);
  qFuzzyCompare(prisJoint->jointMsg->suspension_erp(), 0.2);

  msgs::Axis prisAxis1Msg = prisJoint->jointMsg->axis1();
  QCOMPARE(msgs::ConvertIgn(prisAxis1Msg.xyz()),
      ignition::math::Vector3d(1, 0, 0));
  qFuzzyCompare(prisAxis1Msg.limit_lower(), -GZ_DBL_MAX);
  qFuzzyCompare(prisAxis1Msg.limit_upper(), GZ_DBL_MAX);
  qFuzzyCompare(prisAxis1Msg.limit_effort(), -1);
  qFuzzyCompare(prisAxis1Msg.limit_velocity(), -1);
  qFuzzyCompare(prisAxis1Msg.damping(), 0.0);
  qFuzzyCompare(prisAxis1Msg.friction(), 0.0);
  QCOMPARE(prisAxis1Msg.use_parent_model_frame(), false);

  // Add a gearbox joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_GEARBOX);
  gui::JointData *gearboxJointData =
      jointMaker->CreateJoint(boxLink, cylinderLink);
  jointMaker->CreateHotSpot(gearboxJointData);
  QCOMPARE(jointMaker->GetJointCount(), 3u);

  // verify connected joints
  boxJointData =
      jointMaker->GetJointDataByLink("box::link");
  QCOMPARE(static_cast<unsigned int>(boxJointData.size()), 2u);

  cylinderJointData =
      jointMaker->GetJointDataByLink("cylinder::link");
  QCOMPARE(static_cast<unsigned int>(cylinderJointData.size()), 2u);

  gui::JointData *gearboxJoint = cylinderJointData[0];
  QVERIFY(gearboxJoint != NULL);
  QVERIFY(gearboxJoint->inspector != NULL);

  // verify default values
  QVERIFY(msgs::ConvertJointType(gearboxJoint->jointMsg->type()) == "gearbox");
  QCOMPARE(msgs::Convert(gearboxJoint->jointMsg->pose()), math::Pose::Zero);
  qFuzzyCompare(gearboxJoint->jointMsg->cfm(), 0.0);
  qFuzzyCompare(gearboxJoint->jointMsg->bounce(), 0.0);
  qFuzzyCompare(gearboxJoint->jointMsg->fudge_factor(), 0.0);
  qFuzzyCompare(gearboxJoint->jointMsg->limit_cfm(), 0.0);
  qFuzzyCompare(gearboxJoint->jointMsg->limit_erp(), 0.2);
  qFuzzyCompare(gearboxJoint->jointMsg->suspension_cfm(), 0.0);
  qFuzzyCompare(gearboxJoint->jointMsg->suspension_erp(), 0.2);

  msgs::Axis gearboxAxis1Msg = gearboxJoint->jointMsg->axis1();
  QCOMPARE(msgs::Convert(gearboxAxis1Msg.xyz()), math::Vector3(1, 0, 0));
  qFuzzyCompare(gearboxAxis1Msg.limit_lower(), -GZ_DBL_MAX);
  qFuzzyCompare(gearboxAxis1Msg.limit_upper(), GZ_DBL_MAX);
  qFuzzyCompare(gearboxAxis1Msg.limit_effort(), -1);
  qFuzzyCompare(gearboxAxis1Msg.limit_velocity(), -1);
  qFuzzyCompare(gearboxAxis1Msg.damping(), 0.0);
  qFuzzyCompare(gearboxAxis1Msg.friction(), 0.0);
  QCOMPARE(gearboxAxis1Msg.use_parent_model_frame(), false);

  msgs::Axis gearboxAxis2Msg = gearboxJoint->jointMsg->axis2();
  QCOMPARE(msgs::Convert(gearboxAxis2Msg.xyz()), math::Vector3(0, 1, 0));
  qFuzzyCompare(gearboxAxis2Msg.limit_lower(), -GZ_DBL_MAX);
  qFuzzyCompare(gearboxAxis2Msg.limit_upper(), GZ_DBL_MAX);
  qFuzzyCompare(gearboxAxis2Msg.limit_effort(), -1);
  qFuzzyCompare(gearboxAxis2Msg.limit_velocity(), -1);
  qFuzzyCompare(gearboxAxis2Msg.damping(), 0.0);
  qFuzzyCompare(gearboxAxis2Msg.friction(), 0.0);
  QCOMPARE(gearboxAxis2Msg.use_parent_model_frame(), false);

  // Add a fixed joint
  jointMaker->AddJoint(gui::JointMaker::JOINT_FIXED);
  gui::JointData *fixedJointData =
      jointMaker->CreateJoint(boxLink, cylinderLink);
  jointMaker->CreateHotSpot(fixedJointData);
  QCOMPARE(jointMaker->GetJointCount(), 4u);

  // verify connected joints
  boxJointData =
      jointMaker->GetJointDataByLink("box::link");
  QCOMPARE(static_cast<unsigned int>(boxJointData.size()), 3u);

  cylinderJointData =
      jointMaker->GetJointDataByLink("cylinder::link");
  QCOMPARE(static_cast<unsigned int>(cylinderJointData.size()), 3u);

  gui::JointData *fixedJoint = cylinderJointData[1];
  QVERIFY(fixedJoint != NULL);
  QVERIFY(fixedJoint->inspector != NULL);

  // verify default values
  QVERIFY(msgs::ConvertJointType(fixedJoint->jointMsg->type()) == "fixed");
  QCOMPARE(msgs::Convert(fixedJoint->jointMsg->pose()), math::Pose::Zero);
  qFuzzyCompare(fixedJoint->jointMsg->cfm(), 0.0);
  qFuzzyCompare(fixedJoint->jointMsg->bounce(), 0.0);
  qFuzzyCompare(fixedJoint->jointMsg->fudge_factor(), 0.0);
  qFuzzyCompare(fixedJoint->jointMsg->limit_cfm(), 0.0);
  qFuzzyCompare(fixedJoint->jointMsg->limit_erp(), 0.2);
  qFuzzyCompare(fixedJoint->jointMsg->suspension_cfm(), 0.0);
  qFuzzyCompare(fixedJoint->jointMsg->suspension_erp(), 0.2);

  // fixed joint created using revolute joint with zero limits so it
  // has one axis
  msgs::Axis fixedAxis1Msg = fixedJoint->jointMsg->axis1();
  QCOMPARE(msgs::Convert(fixedAxis1Msg.xyz()), math::Vector3(1, 0, 0));
  qFuzzyCompare(fixedAxis1Msg.limit_lower(), 0);
  qFuzzyCompare(fixedAxis1Msg.limit_upper(), 0);
  qFuzzyCompare(fixedAxis1Msg.limit_effort(), -1);
  qFuzzyCompare(fixedAxis1Msg.limit_velocity(), -1);
  qFuzzyCompare(fixedAxis1Msg.damping(), 0.0);
  qFuzzyCompare(fixedAxis1Msg.friction(), 0.0);
  QCOMPARE(fixedAxis1Msg.use_parent_model_frame(), false);

  delete jointMaker;
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

/////////////////////////////////////////////////
void JointMaker_TEST::JointMaterial()
{
  this->Load("worlds/empty.world");

  gui::JointMaker *jointMaker = new gui::JointMaker();

  // all currently supported joint types.
  std::vector<std::string> jointTypes;
  jointTypes.push_back("revolute");
  jointTypes.push_back("revolute2");
  jointTypes.push_back("prismatic");
  jointTypes.push_back("ball");
  jointTypes.push_back("universal");
  jointTypes.push_back("screw");
  jointTypes.push_back("gearbox");

  // verify joint materials are not empty and they are all unique
  std::set<std::string> jointMaterials;
  for (auto &j : jointTypes)
  {
    std::string mat = jointMaker->GetJointMaterial(j);
    QVERIFY(mat != "");
    QVERIFY(jointMaterials.find(mat) == jointMaterials.end());
    jointMaterials.insert(mat);
  }
  delete jointMaker;
}

// Generate a main function for the test
QTEST_MAIN(JointMaker_TEST)
