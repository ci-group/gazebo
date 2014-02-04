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

#include <gtest/gtest.h>
#include "gazebo/math/Rand.hh"
#include "gazebo/rendering/RenderingIface.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/GpuLaser.hh"
#include "test/ServerFixture.hh"

class GpuLaser_TEST : public ServerFixture
{
};

/////////////////////////////////////////////////
//                                             //
//  bring up a GpuLaser and exercise API       //
//                                             //
/////////////////////////////////////////////////
TEST_F(GpuLaser_TEST, BasicGpuLaserAPITest)
{
  Load("worlds/empty.world");

  gazebo::rendering::ScenePtr scene = gazebo::rendering::get_scene("default");

  if (!scene)
      scene = gazebo::rendering::create_scene("default", false);

  EXPECT_TRUE(scene != NULL);

  gazebo::rendering::GpuLaserPtr laserCam =
    scene->CreateGpuLaser("test_laser", false);

  EXPECT_TRUE(laserCam != NULL);

  // The following tests all the getters and setters
  {
    laserCam->SetNearClip(0.1);
    EXPECT_NEAR(laserCam->GetNearClip(), 0.1, 1e-6);

    laserCam->SetFarClip(100.0);
    EXPECT_NEAR(laserCam->GetFarClip(), 100, 1e-6);

    laserCam->SetHorzHalfAngle(1.2);
    EXPECT_NEAR(laserCam->GetHorzHalfAngle(), 1.2, 1e-6);

    laserCam->SetVertHalfAngle(0.5);
    EXPECT_NEAR(laserCam->GetVertHalfAngle(), 0.5, 1e-6);

    laserCam->SetIsHorizontal(false);
    EXPECT_FALSE(laserCam->IsHorizontal());

    laserCam->SetHorzFOV(2.4);
    EXPECT_NEAR(laserCam->GetHorzFOV(), 2.4, 1e-6);

    laserCam->SetVertFOV(1.0);
    EXPECT_NEAR(laserCam->GetVertFOV(), 1.0, 1e-6);

    laserCam->SetCosHorzFOV(0.2);
    EXPECT_NEAR(laserCam->GetCosHorzFOV(), 0.2, 1e-6);

    laserCam->SetCosVertFOV(0.1);
    EXPECT_NEAR(laserCam->GetCosVertFOV(), 0.1, 1e-6);

    laserCam->SetRayCountRatio(0.344);
    EXPECT_NEAR(laserCam->GetRayCountRatio(), 0.344, 1e-6);

    laserCam->SetCameraCount(4);
    EXPECT_EQ(static_cast<int>(laserCam->GetCameraCount()), 4);
  }
}

/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
