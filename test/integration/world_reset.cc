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
#include <string.h>

#include "gazebo/physics/physics.hh"
#include "gazebo/test/ServerFixture.hh"
#include "gazebo/test/helper_physics_generator.hh"

using namespace gazebo;

typedef std::tr1::tuple<const char *, const char *, int> string2_int;

class WorldResetTest : public ServerFixture,
                       public ::testing::WithParamInterface<string2_int>
{
  /// \brief Test to see if model pose is reset when the world is reset
  /// \param[in] _physicsEngine Physics engine type.
  /// \param[in] _world Name of world to load
  /// \param[in] _resets Number of resets to perform in the test
  public: void ModelPose(const std::string &_physicsEngine,
                         const std::string &_world, const int _resets);

  /// \brief Test to see if nested model pose is reset when the world is reset
  /// \param[in] _physicsEngine Physics engine type.
  /// \param[in] _world Name of world to load
  /// \param[in] _resets Number of resets to perform in the test
  public: void NestedModelPose(const std::string &_physicsEngine,
                         const std::string &_world, const int _resets);

  /// \brief Test resetting different worlds
  /// \param[in] _physicsEngine Physics engine type.
  /// \param[in] _world Name of world to load
  /// \param[in] _resets Number of resets to perform in the test
  public: void WorldName(const std::string &_physicsEngine,
                         const std::string &_world, const int _resets);
};

/////////////////////////////////////////////////
void WorldResetTest::ModelPose(const std::string &_physicsEngine,
                               const std::string &_world, const int _resets)
{
  if (_physicsEngine == "simbody" &&
      _world.find("pr2") != std::string::npos)
  {
    gzerr << "Simbody fails this test with the PR2 due to issue #1672"
          << std::endl;
    return;
  }
  if (_physicsEngine == "simbody" &&
      _world.find("nested_model") != std::string::npos)
  {
    gzerr << "Simbody fails this test with nested models due to issue #1718"
          << std::endl;
    return;
  }
  if (_physicsEngine == "dart" &&
      _world.find("pr2") != std::string::npos)
  {
    gzerr << "Abort test since dart does not support ray sensor in PR2, "
          << "Please see issue #911.\n";
    return;
  }

  Load(_world, true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);

  double dt = physics->GetMaxStepSize();
  unsigned int steps = 250;

  // Step forward, verify time increasing
  world->Step(steps);
  double simTime = world->GetSimTime().Double();
  EXPECT_NEAR(simTime, dt*steps, dt);

  ignition::math::Pose3d initialPose(1, 2, 0.5, 0, 0, 1.57);

  // spawn a box with known initial pose
  math::Vector3 size(1, 1, 1);
  SpawnBox("box", size, initialPose.Pos(), initialPose.Rot().Euler(), false);
  physics::ModelPtr model = world->GetModel("box");
  ASSERT_TRUE(model != NULL);

  // verify box pose
  EXPECT_EQ(model->GetWorldPose(), initialPose);

  // move box to new pose
  ignition::math::Pose3d newPose(4, 5, 0.5, 0, 0, 0);
  model->SetWorldPose(newPose);
  EXPECT_EQ(model->GetWorldPose(), newPose);

  // Reset world repeatedly
  for (int i = 0; i < _resets; ++i)
  {
    // Reset world, verify time == 0
    world->Reset();
    simTime = world->GetSimTime().Double();
    EXPECT_NEAR(simTime, 0.0, dt);

    // Step forward, verify time increasing
    world->Step(steps);
    simTime = world->GetSimTime().Double();
    EXPECT_NEAR(simTime, dt*steps, dt);
  }

  // verify box has moved back to initial pose
  EXPECT_EQ(model->GetWorldPose(), initialPose);
}

/////////////////////////////////////////////////
TEST_P(WorldResetTest, ModelPose)
{
  std::string physics = std::tr1::get<0>(GetParam());

  std::string worldName = std::tr1::get<1>(GetParam());
  int resets = std::tr1::get<2>(GetParam());
  gzdbg << "Physics engine [" << physics << "] "
        << "world name [" << worldName << "] "
        << "reset count [" << resets << "]"
        << std::endl;
  ModelPose(physics, worldName, resets);
}

/////////////////////////////////////////////////
void WorldResetTest::NestedModelPose(const std::string &_physicsEngine,
                                     const std::string &_world,
                                     const int _resets)
{
  if (_physicsEngine == "simbody")
  {
    gzerr  << "Nested models are not working in simbody yet, issue #1718"
        << std::endl;
    return;
  }

  if (_physicsEngine == "dart" &&
      _world.find("pr2") != std::string::npos)
  {
    gzerr << "Abort test since dart does not support ray sensor in PR2, "
          << "Please see issue #911.\n";
    return;
  }

  if (_world != "worlds/nested_model.world")
    return;

  Load(_world, true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);

  double dt = physics->GetMaxStepSize();
  unsigned int steps = 250;

  // Step forward, verify time increasing
  world->Step(steps);
  double simTime = world->GetSimTime().Double();
  EXPECT_NEAR(simTime, dt*steps, dt);

  physics::ModelPtr model = world->GetModel("model_00");
  ASSERT_TRUE(model != NULL);

  // store all initial pose
  std::list<ignition::math::Pose3d> modelPoses;
  std::list<physics::ModelPtr> models;
  models.push_back(model);
  while (!models.empty())
  {
    physics::ModelPtr m = models.front();
    modelPoses.push_back(m->GetWorldPose().Ign());
    models.pop_front();
    for (const auto &nested : m->NestedModels())
      models.push_back(nested);
  }

  // move model to new pose
  ignition::math::Pose3d newPose(9, 5, 2.5, 0, 0, 0);
  model->SetWorldPose(newPose);
  EXPECT_EQ(model->GetWorldPose(), newPose);

  // Reset world repeatedly
  for (int i = 0; i < _resets; ++i)
  {
    // Reset world, verify time == 0
    world->Reset();
    simTime = world->GetSimTime().Double();
    EXPECT_NEAR(simTime, 0.0, dt);

    // Step forward, verify time increasing
    world->Step(steps);
    simTime = world->GetSimTime().Double();
    EXPECT_NEAR(simTime, dt*steps, dt);
  }

  // verify all nested models have moved back to initial pose
  models.clear();
  models.push_back(model);
  while (!models.empty())
  {
    physics::ModelPtr m = models.front();
    EXPECT_EQ(m->GetWorldPose(), modelPoses.front());
    models.pop_front();
    modelPoses.pop_front();
    for (const auto &nested : m->NestedModels())
      models.push_back(nested);
  }
}

/////////////////////////////////////////////////
TEST_P(WorldResetTest, NestedModelPose)
{
  std::string physics = std::tr1::get<0>(GetParam());

  std::string worldName = std::tr1::get<1>(GetParam());
  int resets = std::tr1::get<2>(GetParam());
  gzdbg << "Physics engine [" << physics << "] "
        << "world name [" << worldName << "] "
        << "reset count [" << resets << "]"
        << std::endl;
  NestedModelPose(physics, worldName, resets);
}

/////////////////////////////////////////////////
void WorldResetTest::WorldName(const std::string &_physicsEngine,
                               const std::string &_world, const int _resets)
{
  if (_physicsEngine == "simbody" &&
      _world.find("pr2") != std::string::npos)
  {
    gzerr << "Simbody fails this test with the PR2 due to issue #1672"
          << std::endl;
    return;
  }
  if (_physicsEngine == "simbody" &&
      _world.find("nested_model") != std::string::npos)
  {
    gzerr << "Simbody fails this test with nested models due to issue #1718"
          << std::endl;
    return;
  }
  if (_physicsEngine == "dart" &&
      _world.find("pr2") != std::string::npos)
  {
    gzerr << "Abort test since dart does not support ray sensor in PR2, "
          << "Please see issue #911.\n";
    return;
  }

  Load(_world, true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);

  double dt = physics->GetMaxStepSize();
  unsigned int steps = 250;

  // Step forward, verify time increasing
  world->Step(steps);
  double simTime = world->GetSimTime().Double();
  EXPECT_NEAR(simTime, dt*steps, dt);

  // Reset world repeatedly
  for (int i = 0; i < _resets; ++i)
  {
    // Reset world, verify time == 0
    world->Reset();
    simTime = world->GetSimTime().Double();
    EXPECT_NEAR(simTime, 0.0, dt);

    // Step forward, verify time increasing
    world->Step(steps);
    simTime = world->GetSimTime().Double();
    EXPECT_NEAR(simTime, dt*steps, dt);
  }
}

/////////////////////////////////////////////////
TEST_P(WorldResetTest, WorldName)
{
  std::string physics = std::tr1::get<0>(GetParam());
  std::string worldName = std::tr1::get<1>(GetParam());
  int resets = std::tr1::get<2>(GetParam());
  gzdbg << "Physics engine [" << physics << "] "
        << "world name [" << worldName << "] "
        << "reset count [" << resets << "]"
        << std::endl;
  WorldName(physics, worldName, resets);
}

INSTANTIATE_TEST_CASE_P(PhysicsEngines, WorldResetTest,
  ::testing::Combine(PHYSICS_ENGINE_VALUES,
  ::testing::Values("worlds/empty.world",
                    "worlds/pr2.world",
                    "worlds/nested_model.world"),
  ::testing::Range(1, 3)));

/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
