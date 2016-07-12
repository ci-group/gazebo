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

#include <gtest/gtest.h>
#include "gazebo/rendering/RenderingIface.hh"
#include "gazebo/rendering/RenderTypes.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/Light.hh"
#include "gazebo/test/ServerFixture.hh"

using namespace gazebo;
class Light_TEST : public RenderingFixture
{
};

/////////////////////////////////////////////////
TEST_F(Light_TEST, LightVisualTest)
{
  Load("worlds/empty.world");

  gazebo::rendering::ScenePtr scene = gazebo::rendering::get_scene("default");

  if (!scene)
    scene = gazebo::rendering::create_scene("default", false);

  EXPECT_TRUE(scene != nullptr);

  // create a light
  gazebo::rendering::LightPtr light(new gazebo::rendering::Light(scene));
  light->Load();

  std::string lightName = light->Name();
  gazebo::rendering::VisualPtr lightVisual = scene->GetVisual(lightName);
  EXPECT_TRUE(lightVisual != nullptr);

  scene->RemoveLight(light);

  // reset the pointer so that the destructor gets called
  light.reset();

  lightVisual = scene->GetVisual(lightName);
  EXPECT_TRUE(lightVisual == nullptr);
}

/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
