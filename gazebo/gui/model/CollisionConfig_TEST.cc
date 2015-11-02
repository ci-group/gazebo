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

#include "gazebo/gui/model/CollisionConfig.hh"
#include "gazebo/gui/ConfigWidget.hh"
#include "gazebo/gui/model/CollisionConfig_TEST.hh"

#include "test_config.h"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
void CollisionConfig_TEST::Initialization()
{
  gazebo::gui::CollisionConfig cc;

  QCOMPARE(cc.GetCollisionCount(), 0u);
  QVERIFY(cc.GetData("NotFound") == NULL);
}

/////////////////////////////////////////////////
void CollisionConfig_TEST::CollisionUpdates()
{
  gazebo::gui::CollisionConfig cc;

  msgs::Collision c1, c2, c3;

  cc.AddCollision("c1", &c1);
  cc.AddCollision("c2", &c2);
  cc.AddCollision("c3", &c3);

  QCOMPARE(cc.GetCollisionCount(), 3u);

  QVERIFY(cc.GetData("c1") != NULL);
  QVERIFY(cc.GetData("c2") != NULL);
  QVERIFY(cc.GetData("c3") != NULL);
  QVERIFY(cc.GetData("NotFound") == NULL);

  msgs::CollisionPtr collisionMsgPtr(new msgs::Collision);
  collisionMsgPtr->set_laser_retro(0.0000789);

  cc.UpdateCollision("c1", collisionMsgPtr);
  bool foundConfig = false;

  for (const auto &it : cc.ConfigData())
  {
    if (it.second->name == "c1")
    {
      const CollisionConfigData *configData = it.second;
      QCOMPARE(configData->configWidget->GetDoubleWidgetValue("laser_retro"),
          0.0000789);
      foundConfig = true;
      break;
    }
  }
  QVERIFY(foundConfig);

  cc.Reset();

  QCOMPARE(cc.GetCollisionCount(), 0u);

  QVERIFY(cc.GetData("c1") == NULL);
  QVERIFY(cc.GetData("c2") == NULL);
  QVERIFY(cc.GetData("c3") == NULL);
}

/////////////////////////////////////////////////
void CollisionConfig_TEST::GeometryUpdates()
{
  gazebo::gui::CollisionConfig cc;
  msgs::Collision c1;

  cc.AddCollision("c1", &c1);

  const ignition::math::Vector3d size1(5, 10, 15);

  cc.SetGeometry("c1", size1, "unit_box");

  ignition::math::Vector3d size2;
  std::string uri;

  cc.Geometry("c1", size2, uri);

  QCOMPARE(5.0, size2.X());
  QCOMPARE(10.0, size2.Y());
  QCOMPARE(15.0, size2.Z());

  ignition::math::Vector3d size3(0, 0, 0);

  cc.Geometry("NotFound", size3, uri);

  QCOMPARE(0.0, size3.X());
  QCOMPARE(0.0, size3.Y());
  QCOMPARE(0.0, size3.Z());
}

// Generate a main function for the test
QTEST_MAIN(CollisionConfig_TEST)
