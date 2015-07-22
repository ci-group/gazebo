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
#include <string.h>

#include "gazebo/msgs/msgs.hh"
#include "gazebo/physics/physics.hh"
#include "gazebo/physics/ode/ODESurfaceParams.hh"
#include "gazebo/transport/transport.hh"
#include "gazebo/test/ServerFixture.hh"
#include "gazebo/test/helper_physics_generator.hh"

using namespace gazebo;

const double g_friction_tolerance = 1e-3;

class PhysicsTorsionalFrictionTest : public ServerFixture,
    public testing::WithParamInterface<const char*>
{
  protected: PhysicsTorsionalFrictionTest() : ServerFixture()
  {
  }

  /// \brief Data structure to hold model pointer and friction parameter
  /// for each test model in friction demo world.
  class SphereData
  {
    /// \brief Constructor.
    /// \param[in] _world World pointer.
    /// \param[in] _name Model name.
    public: SphereData(physics::WorldPtr _world,
        const std::string &_name)
    {
      // Get the model pointer
      this->model = _world->GetModel(_name);

      // Get the surface params
      physics::LinkPtr link = this->model->GetLink();
      physics::Collision_V collisions = link->GetCollisions();
      auto iter = collisions.begin();
      EXPECT_TRUE(iter != collisions.end());

      auto surf = (*iter)->GetSurface();
      EXPECT_TRUE(surf != NULL);

      auto surfODE =
          boost::static_pointer_cast<physics::ODESurfaceParams>(surf);

      this->mu3 = surf->GetFrictionPyramid()->GetMuTorsion();
      this->patch = surf->GetFrictionPyramid()->GetPatchRadius();
      this->radius = surf->GetFrictionPyramid()->GetCurvatureRadius();
      this->kp = surfODE->kp;

      auto inertial = link->GetInertial();
      EXPECT_TRUE(inertial != NULL);

      this->mass = inertial->GetMass();
    }

    /// \brief Destructor
    public: ~SphereData() = default;

    /// \brief Pointer to the model
    public: physics::ModelPtr model;

    /// \brief Torsional friction
    public: double mu3;

    /// \brief Spring constant equivalents of a contact.
    public: double kp;

    /// \brief Mass
    public: double mass;

    /// \brief Patch
    public: double patch;

    /// \brief Radius
    public: double radius;
  };

  /// \brief Use the torsional_friction_test world to test spheres with
  /// different coefficients of torsional friction.
  /// \param[in] _physicsEngine Physics engine to use.
  public: void CoefficientTest(const std::string &_physicsEngine);

  /// \brief Use the torsional_friction_test world to test spheres with
  /// different curvatures.
  /// \param[in] _physicsEngine Physics engine to use.
  public: void CurvatureTest(const std::string &_physicsEngine);

  /// \brief Use the torsional_friction_test world to test spheres with
  /// different contact depths. It checks if the contact depth is correct for
  /// those spheres.
  /// \param[in] _physicsEngine Physics engine to use.
  public: void DepthTest(const std::string &_physicsEngine);

  /// \brief Use the torsional_friction_test world to test spheres with
  /// different patch radii.
  /// \param[in] _physicsEngine Physics engine to use.
  public: void PatchTest(const std::string &_physicsEngine);

  /// \brief Callback for contact subscribers in depth test.
  /// \param[in] _msg Contact message
  private: void Callback(const ConstContactsPtr &_msg);

  /// \brief Message to be filled with the latest contacts message.
  private: msgs::Contacts contactsMsg;
};

/////////////////////////////////////////////////
void PhysicsTorsionalFrictionTest::Callback(const ConstContactsPtr &_msg)
{
  this->contactsMsg = *_msg;
}

/////////////////////////////////////////////////
void PhysicsTorsionalFrictionTest::DepthTest(
    const std::string &_physicsEngine)
{
  if (_physicsEngine != "ode")
  {
    gzerr << "Torsional friction only works with ODE (#ISSUE)"
          << std::endl;
    return;
  }

  Load("test/worlds/torsional_friction_test.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // Step so contacts begin
  world->Step(1);

  // check the gravity vector
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);
  math::Vector3 g = physics->GetGravity();
  EXPECT_DOUBLE_EQ(g.x, 0);
  EXPECT_DOUBLE_EQ(g.y, 0);
  EXPECT_DOUBLE_EQ(g.z, -9.8);

  // Sleep to ensure transport topics are all advertised
  common::Time::MSleep(100);
  std::list<std::string> topics =
    transport::getAdvertisedTopics("gazebo.msgs.Contacts");
  topics.sort();
  EXPECT_FALSE(topics.empty());
  EXPECT_EQ(topics.size(), 1u);

  auto topic = topics.front();

  gzdbg << "Listening to " << topic << std::endl;
  transport::SubscriberPtr sub = this->node->Subscribe(topic,
      &PhysicsTorsionalFrictionTest::Callback, this);

  // Step for spheres to sink and rest at the final depth
  world->Step(5000);

  EXPECT_GT(this->contactsMsg.contact().size(), 0);

  // Load the spheres
  std::vector<PhysicsTorsionalFrictionTest::SphereData>
      spheres;
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_mass_1"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_mass_2"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_mass_3"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_mass_4"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_mass_5"));

  // Verify sphere data structure
  for (auto sphere : spheres)
  {
    ASSERT_TRUE(sphere.model != NULL);
  }

  // Check relevant contacts from message
  for (auto contact : this->contactsMsg.contact())
  {
    if (!(contact.has_collision1() &&
        contact.collision1().find("sphere_mass") == 0))
      continue;

    // Get corresponding sphere
    int number = std::stoi(contact.collision1().substr(12, 1));
    ASSERT_GT(number, 0);
    auto sphere = spheres[number-1];

    // Check that contact normal is in the positive Z direction
    math::Vector3 normal = msgs::Convert(contact.normal(0));
    EXPECT_EQ(normal, math::Vector3::UnitZ);

    // Check that contact depth is:
    // normal force = kp * depth
    double expectedDepth = sphere.mass * -g.z / sphere.kp;
    double relativeError =
      std::abs(contact.depth(0) - expectedDepth)/expectedDepth;

    // Less than 1% error
    EXPECT_LT(relativeError, 0.01);
  }
}

/////////////////////////////////////////////////
void PhysicsTorsionalFrictionTest::CoefficientTest(
    const std::string &_physicsEngine)
{
  if (_physicsEngine != "ode")
  {
    gzerr << "Torsional friction only works with ODE (#ISSUE)"
          << std::endl;
    return;
  }

  Load("worlds/torsional_friction_test.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // check the gravity vector
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);
  math::Vector3 g = physics->GetGravity();
  EXPECT_DOUBLE_EQ(g.x, 0);
  EXPECT_DOUBLE_EQ(g.y, 0);
  EXPECT_DOUBLE_EQ(g.z, -9.8);

  // Load the spheres
  std::vector<PhysicsTorsionalFrictionTest::SphereData>
      spheres;
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_mu3_1"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_mu3_2"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_mu3_3"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_mu3_4"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_mu3_5"));

  // Verify sphere data structure
  for (auto sphere : spheres)
  {
    ASSERT_TRUE(sphere.model != NULL);
  }

  common::Time t = world->GetSimTime();
  while (t.sec < 1)
  {
    world->Step(500);
    t = world->GetSimTime();

    for (auto sphere : spheres)
    {
      math::Vector3 vel = sphere.model->GetWorldAngularVel();
      EXPECT_NEAR(vel.x, 0, g_friction_tolerance);
      EXPECT_NEAR(vel.y, 0, g_friction_tolerance);

      double rhs = 3 * M_PI * sphere.patch * sphere.mu3 / 16;
      double normalMag = -sphere.mass * g.z;

std::cout << sphere.model->GetName()
          << "  t[" << t.Double() << "]"
          << "  vel.z[" << vel.z << "]"
          << "  rhs[" << rhs << "]"
          << "  normal[" << normalMag << "]" << std::endl;

      // Does this make sense for mu3?
      if (sphere.mu3 >= 1.0)
      {
        // Friction is large enough to prevent motion
        EXPECT_NEAR(vel.z, 0, g_friction_tolerance);
      }
      else
      {
        // Friction is small enough to allow motion
        // Expect velocity = acceleration * time
        // EXPECT_NEAR(vel.z, (g.z + sphere.mu3) * t.Double(),
        //            g_friction_tolerance);
      }
    }
  }
}

/////////////////////////////////////////////////
void PhysicsTorsionalFrictionTest::PatchTest(
    const std::string &_physicsEngine)
{
  if (_physicsEngine != "ode")
  {
    gzerr << "Torsional friction only works with ODE (#ISSUE)"
          << std::endl;
    return;
  }

  Load("worlds/torsional_friction_test.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // check the gravity vector
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);
  math::Vector3 g = physics->GetGravity();
  EXPECT_DOUBLE_EQ(g.x, 0);
  EXPECT_DOUBLE_EQ(g.y, 0);
  EXPECT_DOUBLE_EQ(g.z, -9.8);

  // Load the spheres
  std::vector<PhysicsTorsionalFrictionTest::SphereData>
      spheres;
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_patch_1"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_patch_2"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_patch_3"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_patch_4"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_patch_5"));

  // Verify sphere data structure
  for (auto sphere : spheres)
  {
    ASSERT_TRUE(sphere.model != NULL);
  }

  common::Time t = world->GetSimTime();
  while (t.sec < 1)
  {
    world->Step(500);
    t = world->GetSimTime();

    for (auto sphere : spheres)
    {
      math::Vector3 vel = sphere.model->GetWorldAngularVel();
      EXPECT_NEAR(vel.x, 0, g_friction_tolerance);
      EXPECT_NEAR(vel.y, 0, g_friction_tolerance);

      double rhs = 3 * M_PI * sphere.patch * sphere.mu3 / 16;
      double normalMag = -sphere.mass * g.z;

std::cout << sphere.model->GetName()
          << "  t[" << t.Double() << "]"
          << "  vel.z[" << vel.z << "]"
          << "  rhs[" << rhs << "]"
          << "  normal[" << normalMag << "]" << std::endl;

      // Expect something in function of patch...?
      // EXPECT_NEAR(vel.z, (g.z + sphere.patch) * t.Double(),
      //            g_friction_tolerance);
    }
  }
}

/////////////////////////////////////////////////
void PhysicsTorsionalFrictionTest::CurvatureTest(
    const std::string &_physicsEngine)
{
  if (_physicsEngine != "ode")
  {
    gzerr << "Torsional friction only works with ODE (#ISSUE)"
          << std::endl;
    return;
  }

  Load("worlds/torsional_friction_test.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // check the gravity vector
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);
  math::Vector3 g = physics->GetGravity();
  EXPECT_DOUBLE_EQ(g.x, 0);
  EXPECT_DOUBLE_EQ(g.y, 0);
  EXPECT_DOUBLE_EQ(g.z, -9.8);

  // Load the spheres
  std::vector<PhysicsTorsionalFrictionTest::SphereData>
      spheres;
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_radius_1"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_radius_2"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_radius_3"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_radius_4"));
  spheres.push_back(
      PhysicsTorsionalFrictionTest::SphereData(world, "sphere_radius_5"));

  // Verify sphere data structure
  for (auto sphere : spheres)
  {
    ASSERT_TRUE(sphere.model != NULL);
  }

  common::Time t = world->GetSimTime();
  while (t.sec < 1)
  {
    world->Step(500);
    t = world->GetSimTime();

    for (auto sphere : spheres)
    {
      math::Vector3 vel = sphere.model->GetWorldAngularVel();
      EXPECT_NEAR(vel.x, 0, g_friction_tolerance);
      EXPECT_NEAR(vel.y, 0, g_friction_tolerance);

      double depthAtEquilibrium = sphere.mass * -g.z / sphere.kp;
      double patch = sqrt(sphere.radius * depthAtEquilibrium);
      double rhs = 3 * M_PI * patch * sphere.mu3 / 16;
      double normalMag = -sphere.mass * g.z;

std::cout << sphere.model->GetName()
          << "  t[" << t.Double() << "]"
          << "  vel.z[" << vel.z << "]"
          << "  depth[" << depthAtEquilibrium << "]"
          << "  patch[" << patch << "]"
          << "  rhs[" << rhs << "]"
          << "  normal[" << normalMag << "]" << std::endl;

      // Expect something in function of radius...?
      // EXPECT_NEAR(vel.z, (g.z + sphere.radius) * t.Double(),
      //            g_friction_tolerance);
    }
  }
}

/////////////////////////////////////////////////
TEST_P(PhysicsTorsionalFrictionTest, CoefficientTest)
{
  CoefficientTest(GetParam());
}

/////////////////////////////////////////////////
TEST_P(PhysicsTorsionalFrictionTest, CurvatureTest)
{
  CurvatureTest(GetParam());
}

/////////////////////////////////////////////////
TEST_P(PhysicsTorsionalFrictionTest, DepthTest)
{
  DepthTest(GetParam());
}

/////////////////////////////////////////////////
TEST_P(PhysicsTorsionalFrictionTest, PatchTest)
{
  PatchTest(GetParam());
}

INSTANTIATE_TEST_CASE_P(PhysicsEngines, PhysicsTorsionalFrictionTest,
                        PHYSICS_ENGINE_VALUES);

/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
