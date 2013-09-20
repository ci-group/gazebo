/*
 * Copyright 2012 Open Source Robotics Foundation
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

#include "ServerFixture.hh"
#include "gazebo/physics/physics.hh"
#include "SimplePendulumIntegrator.hh"
#include "gazebo/msgs/msgs.hh"
#include "helper_physics_generator.hh"

#define PHYSICS_TOL 1e-2
using namespace gazebo;

class PhysicsTest : public ServerFixture,
                    public testing::WithParamInterface<const char*>
{
  public: void InelasticCollision(const std::string &_physicsEngine);
  public: void EmptyWorld(const std::string &_physicsEngine);
  public: void SpawnDrop(const std::string &_physicsEngine);
  public: void SpawnDropCoGOffset(const std::string &_physicsEngine);
  public: void RevoluteJoint(const std::string &_physicsEngine);
  public: void SimplePendulum(const std::string &_physicsEngine);
  public: void CollisionFiltering(const std::string &_physicsEngine);
  public: void JointDampingTest(const std::string &_physicsEngine);
  public: void DropStuff(const std::string &_physicsEngine);
};

////////////////////////////////////////////////////////////////////////
// EmptyWorld:
// Load a world, take a few steps, and verify that time is increasing.
// This is the most basic physics engine test.
////////////////////////////////////////////////////////////////////////
void PhysicsTest::EmptyWorld(const std::string &_physicsEngine)
{
  // Load an empty world
  Load("worlds/empty.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // Verify physics engine type
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);

  // simulate 1 step
  world->StepWorld(1);
  double t = world->GetSimTime().Double();
  // verify that time moves forward
  EXPECT_GT(t, 0);

  // simulate a few steps
  int steps = 20;
  world->StepWorld(steps);
  double dt = world->GetPhysicsEngine()->GetMaxStepSize();
  EXPECT_GT(dt, 0);
  t = world->GetSimTime().Double();
  EXPECT_GT(t, 0.99*dt*static_cast<double>(steps+1));
}

TEST_P(PhysicsTest, EmptyWorld)
{
  EmptyWorld(GetParam());
}

////////////////////////////////////////////////////////////////////////
// SpawnDrop:
// Load a world, check that gravity points along z axis, spawn simple
// shapes (box, sphere, cylinder), verify that they fall and hit the
// ground plane. The test currently assumes inelastic collisions.
////////////////////////////////////////////////////////////////////////
void PhysicsTest::SpawnDrop(const std::string &_physicsEngine)
{
  // load an empty world
  Load("worlds/empty.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // check the gravity vector
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);
  math::Vector3 g = physics->GetGravity();
  // Assume gravity vector points down z axis only.
  EXPECT_EQ(g.x, 0);
  EXPECT_EQ(g.y, 0);
  EXPECT_LE(g.z, -9.8);

  // get physics time step
  double dt = physics->GetMaxStepSize();
  EXPECT_GT(dt, 0);

  // spawn some simple shapes and check to see that they start falling
  double z0 = 3;
  std::map<std::string, math::Vector3> modelPos;
  modelPos["test_box"] = math::Vector3(0, 0, z0);
  modelPos["test_sphere"] = math::Vector3(4, 0, z0);
  modelPos["test_cylinder"] = math::Vector3(8, 0, z0);
  modelPos["test_empty"] = math::Vector3(12, 0, z0);
  modelPos["link_offset_box"] = math::Vector3(0, 0, z0);

  // FIXME Trimesh drop test passes in bullet but fails in ode because
  // the mesh bounces to the side when it hits the ground.
  // See issue #513. Uncomment test when issue is resolved.
  // modelPos["test_trimesh"] = math::Vector3(16, 0, z0);

  SpawnBox("test_box", math::Vector3(1, 1, 1), modelPos["test_box"],
      math::Vector3::Zero);
  SpawnSphere("test_sphere", modelPos["test_sphere"], math::Vector3::Zero);
  SpawnCylinder("test_cylinder", modelPos["test_cylinder"],
      math::Vector3::Zero);
  SpawnEmptyLink("test_empty", modelPos["test_empty"], math::Vector3::Zero);

  std::ostringstream linkOffsetStream;
  math::Pose linkOffsetPose1(0, 0, z0, 0, 0, 0);
  math::Pose linkOffsetPose2(1000, 1000, 0, 0, 0, 0);
  math::Vector3 linkOffsetSize(1, 1, 1);
  linkOffsetStream << "<sdf version='" << SDF_VERSION << "'>"
    << "<model name ='link_offset_box'>"
    << "<pose>" << linkOffsetPose1 << "</pose>"
    << "<allow_auto_disable>false</allow_auto_disable>"
    << "<link name ='body'>"
    << "  <pose>" << linkOffsetPose2 << "</pose>"
    << "  <inertial>"
    << "    <mass>4.0</mass>"
    << "    <inertia>"
    << "      <ixx>0.1667</ixx> <ixy>0.0</ixy> <ixz>0.0</ixz>"
    << "      <iyy>0.1667</iyy> <iyz>0.0</iyz>"
    << "      <izz>0.1667</izz>"
    << "    </inertia>"
    << "  </inertial>"
    << "  <collision name ='geom'>"
    << "    <geometry>"
    << "      <box><size>" << linkOffsetSize << "</size></box>"
    << "    </geometry>"
    << "  </collision>"
    << "  <visual name ='visual'>"
    << "    <geometry>"
    << "      <box><size>" << linkOffsetSize << "</size></box>"
    << "    </geometry>"
    << "  </visual>"
    << "</link>"
    << "</model>"
    << "</sdf>";
  SpawnSDF(linkOffsetStream.str());

  /// \TODO: bullet needs this to pass
  if (physics->GetType()  == "bullet")
    physics->SetSORPGSIters(300);

  // std::string trimeshPath =
  //    "file://media/models/cube_20k/meshes/cube_20k.stl";
  // SpawnTrimesh("test_trimesh", trimeshPath, math::Vector3(0.5, 0.5, 0.5),
  //    modelPos["test_trimesh"], math::Vector3::Zero);

  int steps = 2;
  physics::ModelPtr model;
  math::Pose pose1, pose2;
  math::Vector3 vel1, vel2;

  double t, x0 = 0;
  // This loop steps the world forward and makes sure that each model falls,
  // expecting downward z velocity and decreasing z position.
  for (std::map<std::string, math::Vector3>::iterator iter = modelPos.begin();
    iter != modelPos.end(); ++iter)
  {
    std::string name = iter->first;
    // Make sure the model is loaded
    model = world->GetModel(name);
    if (model != NULL)
    {
      gzdbg << "Check freefall of model " << name << '\n';
      // Step once and check downward z velocity
      world->StepWorld(1);
      vel1 = model->GetWorldLinearVel();
      t = world->GetSimTime().Double();
      EXPECT_EQ(vel1.x, 0);
      EXPECT_EQ(vel1.y, 0);
      EXPECT_NEAR(vel1.z, g.z*t, -g.z*t*PHYSICS_TOL);
      // Need to step at least twice to check decreasing z position
      world->StepWorld(steps - 1);
      pose1 = model->GetWorldPose();
      x0 = modelPos[name].x;
      EXPECT_EQ(pose1.pos.x, x0);
      EXPECT_EQ(pose1.pos.y, 0);
      EXPECT_NEAR(pose1.pos.z, z0 + g.z/2*t*t, (z0+g.z/2*t*t)*PHYSICS_TOL);
      // Check once more and just make sure they keep falling
      world->StepWorld(steps);
      vel2 = model->GetWorldLinearVel();
      pose2 = model->GetWorldPose();
      EXPECT_LT(vel2.z, vel1.z);
      EXPECT_LT(pose2.pos.z, pose1.pos.z);

      // if (physics->GetType()  == "bullet")
      // {
      //   gzerr << "m[" << model->GetName()
      //         << "] p[" << model->GetWorldPose()
      //         << "] v[" << model->GetWorldLinearVel()
      //         << "]\n";

      //   gzerr << "wait: ";
      //   getchar();
      // }
    }
    else
    {
      gzerr << "Error loading model " << name << '\n';
      EXPECT_TRUE(model != NULL);
    }
  }

  // Predict time of contact with ground plane.
  double tHit = sqrt(2*(z0-0.5) / (-g.z));
  // Time to advance, allow 0.5 s settling time.
  // This assumes inelastic collisions with the ground.
  double dtHit = tHit+0.5 - world->GetSimTime().Double();
  steps = ceil(dtHit / dt);
  EXPECT_GT(steps, 0);

  world->StepWorld(steps);

  // debug
  // for (int i = 0; i < steps; ++i)
  // {
  //   world->StepWorld(1);
  //   if (physics->GetType()  == "bullet")
  //   {
  //     model = world->GetModel("link_offset_box");
  //     gzerr << "m[" << model->GetName()
  //           << "] i[" << i << "/" << steps
  //           << "] pm[" << model->GetWorldPose()
  //           << "] pb[" << model->GetLink("body")->GetWorldPose()
  //           << "] v[" << model->GetWorldLinearVel()
  //           << "]\n";

  //     if (model->GetWorldPose().pos.z < 0.6)
  //     {
  //       gzerr << "wait: ";
  //       getchar();
  //     }
  //   }
  // }

  // This loop checks the velocity and pose of each model 0.5 seconds
  // after the time of predicted ground contact. The velocity is expected
  // to be small, and the pose is expected to be underneath the initial pose.
  for (std::map<std::string, math::Vector3>::iterator iter = modelPos.begin();
    iter != modelPos.end(); ++iter)
  {
    std::string name = iter->first;
    // Make sure the model is loaded
    model = world->GetModel(name);
    if (model != NULL)
    {
      gzdbg << "Check ground contact of model " << name << '\n';
      // Check that velocity is small
      vel1 = model->GetWorldLinearVel();
      t = world->GetSimTime().Double();
      EXPECT_NEAR(vel1.x, 0, PHYSICS_TOL);
      EXPECT_NEAR(vel1.y, 0, PHYSICS_TOL);
      if (name == "test_empty")
        EXPECT_NEAR(vel1.z, g.z*t, -g.z*t*PHYSICS_TOL);
      else
        EXPECT_NEAR(vel1.z, 0, PHYSICS_TOL);

      // Check that model is resting on ground
      pose1 = model->GetWorldPose();
      x0 = modelPos[name].x;
      EXPECT_NEAR(pose1.pos.x, x0, PHYSICS_TOL);
      EXPECT_NEAR(pose1.pos.y, 0, PHYSICS_TOL);

      // debug
      // if (physics->GetType()  == "bullet")
      // {
      //   gzerr << "m[" << model->GetName()
      //         << "] p[" << model->GetWorldPose()
      //         << "] v[" << model->GetWorldLinearVel()
      //         << "]\n";

      //   gzerr << "wait: ";
      //   getchar();
      // }

      if (name == "test_empty")
      {
        EXPECT_NEAR(pose1.pos.z, z0+g.z/2*t*t,
            fabs((z0+g.z/2*t*t)*PHYSICS_TOL));
      }
      else
        EXPECT_NEAR(pose1.pos.z, 0.5, PHYSICS_TOL);
    }
    else
    {
      gzerr << "Error loading model " << name << '\n';
      EXPECT_TRUE(model != NULL);
    }
  }

  // Compute and check link pose of link_offset_box
  gzdbg << "Check link pose of link_offset_box\n";
  model = world->GetModel("link_offset_box");
  ASSERT_TRUE(model);
  physics::LinkPtr link = model->GetLink();
  ASSERT_TRUE(link);
  // relative pose of link in linkOffsetPose2
  for (int i = 0; i < 20; ++i)
  {
    pose1 = model->GetWorldPose();
    pose2 = linkOffsetPose2 + pose1;
    EXPECT_NEAR(pose2.pos.x, linkOffsetPose2.pos.x, PHYSICS_TOL);
    EXPECT_NEAR(pose2.pos.y, linkOffsetPose2.pos.y, PHYSICS_TOL);
    EXPECT_NEAR(pose2.pos.z, 0.5, PHYSICS_TOL);
    world->StepWorld(1);
  }
}

TEST_P(PhysicsTest, SpawnDrop)
{
  SpawnDrop(GetParam());
}

////////////////////////////////////////////////////////////////////////
// SpawnDropCoGOffset:
// Load a world, check that gravity points along z axis, spawn several
// spheres of varying radii and center of gravity (cg) location.
//  sphere1: smaller radius, centered cg
//  sphere2: larger radius, centered cg
//  sphere3: larger radius, lowered cg
//  sphere4: larger radius, raised cg
//  sphere5: larger radius, y offset cg
//  sphere6: larger radius, x offset cg
//  sphere7: larger radius, 45 deg offset cg
//  sphere8: larger radius, -30 deg offset cg
// The bottom of each sphere is at the same height, and it is verified
// that they hit the ground at the same time. Also, sphere5 should start
// rolling to the side when it hits the ground.
////////////////////////////////////////////////////////////////////////
void PhysicsTest::SpawnDropCoGOffset(const std::string &_physicsEngine)
{
  // load an empty world
  Load("worlds/empty.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // check the gravity vector
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);
  math::Vector3 g = physics->GetGravity();
  // Assume gravity vector points down z axis only.
  EXPECT_EQ(g.x, 0);
  EXPECT_EQ(g.y, 0);
  EXPECT_LT(g.z, 0);

  // get physics time step
  double dt = physics->GetMaxStepSize();
  EXPECT_GT(dt, 0);

  // spawn some spheres and check to see that they start falling
  double z0 = 3;
  double r1 = 0.5, r2 = 1.5;
  math::Vector3 v30 = math::Vector3::Zero;
  math::Vector3 cog;
  math::Angle angle;

  std::vector<std::string> modelNames;
  std::vector<double> x0s;
  std::vector<double> y0s;
  std::vector<double> radii;
  std::vector<math::Vector3> cogs;

  // sphere1 and sphere2 have c.g. at center of sphere, different sizes
  modelNames.push_back("small_centered_sphere");
  x0s.push_back(0);
  y0s.push_back(0);
  radii.push_back(r1);
  cogs.push_back(v30);

  modelNames.push_back("large_centered_sphere");
  x0s.push_back(4);
  y0s.push_back(0);
  radii.push_back(r2);
  cogs.push_back(v30);

  // sphere3 has c.g. below the center
  modelNames.push_back("lowered_cog_sphere");
  x0s.push_back(8);
  y0s.push_back(0);
  radii.push_back(r2);
  cogs.push_back(math::Vector3(0, 0, -r1));

  // sphere4 has c.g. above the center
  modelNames.push_back("raised_cog_sphere");
  x0s.push_back(-4);
  y0s.push_back(0);
  radii.push_back(r2);
  cogs.push_back(math::Vector3(0, 0, r1));

  // sphere5 has c.g. to the side along y axis; it will roll
  modelNames.push_back("cog_y_offset_sphere");
  x0s.push_back(-8);
  y0s.push_back(0);
  radii.push_back(r2);
  cogs.push_back(math::Vector3(0, r1, 0));

  // sphere6 has c.g. to the side along x axis; it will roll
  modelNames.push_back("cog_x_offset_sphere");
  x0s.push_back(15);
  y0s.push_back(0);
  radii.push_back(r2);
  cogs.push_back(math::Vector3(r1, 0, 0));

  // sphere7 has c.g. to the side diagonally; it will roll
  modelNames.push_back("cog_xy_45deg_offset_sphere");
  x0s.push_back(0);
  y0s.push_back(8);
  radii.push_back(r2);
  angle.SetFromDegree(45);
  cogs.push_back(math::Vector3(r1*cos(angle.Radian()),
                               r1*sin(angle.Radian()), 0));

  // sphere8 has c.g. to the side diagonally; it will roll
  modelNames.push_back("cog_xy_-30deg_offset_sphere");
  x0s.push_back(0);
  y0s.push_back(-8);
  radii.push_back(r2);
  angle.SetFromDegree(-30);
  cogs.push_back(math::Vector3(r1*cos(angle.Radian()),
                               r1*sin(angle.Radian()), 0));

  unsigned int i;
  for (i = 0; i < modelNames.size(); ++i)
  {
    SpawnSphere(modelNames[i], math::Vector3(x0s[i], y0s[i], z0+radii[i]),
                v30, cogs[i], radii[i]);
  }

  int steps = 2;
  physics::ModelPtr model;
  math::Pose pose1, pose2;
  math::Vector3 vel1, vel2;

  double t, x0 = 0, y0 = 0, radius;
  // This loop steps the world forward and makes sure that each model falls,
  // expecting downward z velocity and decreasing z position.
  for (i = 0; i < modelNames.size(); ++i)
  {
    // Make sure the model is loaded
    model = world->GetModel(modelNames[i]);
    x0 = x0s[i];
    y0 = y0s[i];
    radius = radii[i];
    if (model != NULL)
    {
      gzdbg << "Check freefall of model " << modelNames[i] << '\n';
      // Step once and check downward z velocity
      world->StepWorld(1);
      vel1 = model->GetWorldLinearVel();
      t = world->GetSimTime().Double();
      EXPECT_EQ(vel1.x, 0);
      EXPECT_EQ(vel1.y, 0);
      EXPECT_NEAR(vel1.z, g.z*t, -g.z*t*PHYSICS_TOL);
      // Need to step at least twice to check decreasing z position
      world->StepWorld(steps - 1);
      pose1 = model->GetWorldPose();
      EXPECT_NEAR(pose1.pos.x, x0, PHYSICS_TOL*PHYSICS_TOL);
      EXPECT_NEAR(pose1.pos.y, y0, PHYSICS_TOL*PHYSICS_TOL);
      EXPECT_NEAR(pose1.pos.z, z0+radius + g.z/2*t*t,
                  (z0+radius+g.z/2*t*t)*PHYSICS_TOL);

      // Check once more and just make sure they keep falling
      world->StepWorld(steps);
      vel2 = model->GetWorldLinearVel();
      pose2 = model->GetWorldPose();
      EXPECT_LT(vel2.z, vel1.z);
      EXPECT_LT(pose2.pos.z, pose1.pos.z);
    }
    else
    {
      gzerr << "Error loading model " << modelNames[i] << '\n';
      EXPECT_TRUE(model != NULL);
    }
  }

  // Predict time of contact with ground plane.
  double tHit = sqrt(2*(z0-0.5) / (-g.z));
  // Time to advance, allow 0.5 s settling time.
  // This assumes inelastic collisions with the ground.
  double dtHit = tHit+0.5 - world->GetSimTime().Double();
  steps = ceil(dtHit / dt);
  EXPECT_GT(steps, 0);
  world->StepWorld(steps);

  // This loop checks the velocity and pose of each model 0.5 seconds
  // after the time of predicted ground contact. Except for sphere5,
  // the velocity is expected to be small, and the pose is expected
  // to be underneath the initial pose. sphere5 is expected to be rolling.
  for (i = 0; i < modelNames.size(); ++i)
  {
    // Make sure the model is loaded
    model = world->GetModel(modelNames[i]);
    x0 = x0s[i];
    y0 = y0s[i];
    radius = radii[i];
    cog = cogs[i];
    if (model != NULL)
    {
      gzdbg << "Check ground contact and roll without slip of model "
            << modelNames[i] << '\n';

      // Check that velocity is small
      vel1 = model->GetWorldLinearVel();
      vel2 = model->GetWorldAngularVel();

      // vertical component of linear and angular velocity should be small
      EXPECT_NEAR(vel1.z, 0, PHYSICS_TOL);
      EXPECT_NEAR(vel2.z, 0, PHYSICS_TOL);

      // expect small values for directions with no offset
      if (cog.x == 0)
      {
        EXPECT_NEAR(vel1.x, 0, PHYSICS_TOL);
        EXPECT_NEAR(vel2.y, 0, PHYSICS_TOL);
      }
      // expect rolling in direction of cog offset
      else
      {
        EXPECT_GT(vel1.x*cog.x, 0.2*cog.x*cog.x);
        EXPECT_GT(vel2.y*cog.x, 0.2*cog.x*cog.x);
      }

      if (cog.y == 0)
      {
        EXPECT_NEAR(vel1.y, 0, PHYSICS_TOL);
        EXPECT_NEAR(vel2.x, 0, PHYSICS_TOL);
      }
      else
      {
        EXPECT_GT(vel1.y*cog.y,  0.2*cog.y*cog.y);
        EXPECT_LT(vel2.x*cog.y, -0.2*cog.y*cog.y);
      }

      // Expect roll without slip
      EXPECT_NEAR(vel1.x,  vel2.y*radius, PHYSICS_TOL);
      EXPECT_NEAR(vel1.y, -vel2.x*radius, PHYSICS_TOL);

      // Use GetWorldLinearVel with global offset to check roll without slip
      // Expect small linear velocity at contact point
      math::Vector3 vel3 = model->GetLink()->GetWorldLinearVel(
          math::Vector3(0, 0, -radius), math::Quaternion(0, 0, 0));
      EXPECT_NEAR(vel3.x, 0, PHYSICS_TOL);
      EXPECT_NEAR(vel3.y, 0, PHYSICS_TOL);
      EXPECT_NEAR(vel3.z, 0, PHYSICS_TOL);
      // Expect speed at top of sphere to be double the speed at center
      math::Vector3 vel4 = model->GetLink()->GetWorldLinearVel(
          math::Vector3(0, 0, radius), math::Quaternion(0, 0, 0));
      EXPECT_NEAR(vel4.y, 2*vel1.y, PHYSICS_TOL);
      EXPECT_NEAR(vel4.x, 2*vel1.x, PHYSICS_TOL);
      EXPECT_NEAR(vel4.z, 0, PHYSICS_TOL);

      // Check that model is resting on ground
      pose1 = model->GetWorldPose();
      EXPECT_NEAR(pose1.pos.z, radius, PHYSICS_TOL);

      // expect no pose change for directions with no offset
      if (cog.x == 0)
      {
        EXPECT_NEAR(pose1.pos.x, x0, PHYSICS_TOL);
      }
      // expect rolling in direction of cog offset
      else
      {
        EXPECT_GT((pose1.pos.x-x0) * cog.x, cog.x * cog.x);
      }

      // expect no pose change for directions with no offset
      if (cog.y == 0)
      {
        EXPECT_NEAR(pose1.pos.y, y0, PHYSICS_TOL);
      }
      // expect rolling in direction of cog offset
      else
      {
        EXPECT_GT((pose1.pos.y-y0) * cog.y, cog.y * cog.y);
      }
    }
    else
    {
      gzerr << "Error loading model " << modelNames[i] << '\n';
      EXPECT_TRUE(model != NULL);
    }
  }
}

TEST_P(PhysicsTest, SpawnDropCoGOffset)
{
  SpawnDropCoGOffset(GetParam());
}

////////////////////////////////////////////////////////////////////////
// RevoluteJoint:
// Load 8 double pendulums arranged in a circle.
// Measure angular velocity of links, and verify proper axis orientation.
// Then set joint limits and verify that links remain within limits.
////////////////////////////////////////////////////////////////////////
void PhysicsTest::RevoluteJoint(const std::string &_physicsEngine)
{
  math::Rand::SetSeed(0);
  // Load world
  Load("worlds/revolute_joint_test.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // Verify physics engine type
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);

  // Model names
  std::vector<std::string> modelNames;
  modelNames.push_back("pendulum_0deg");
  modelNames.push_back("pendulum_45deg");
  modelNames.push_back("pendulum_90deg");
  modelNames.push_back("pendulum_135deg");
  modelNames.push_back("pendulum_180deg");
  modelNames.push_back("pendulum_225deg");
  modelNames.push_back("pendulum_270deg");
  modelNames.push_back("pendulum_315deg");

  // Global axis
  double sqrt1_2 = sqrt(2.0) / 2.0;
  std::vector<math::Vector3> globalAxes;
  globalAxes.push_back(math::Vector3(1, 0, 0));
  globalAxes.push_back(math::Vector3(sqrt1_2, sqrt1_2, 0));
  globalAxes.push_back(math::Vector3(0, 1, 0));
  globalAxes.push_back(math::Vector3(-sqrt1_2, sqrt1_2, 0));
  globalAxes.push_back(math::Vector3(-1, 0, 0));
  globalAxes.push_back(math::Vector3(-sqrt1_2, -sqrt1_2, 0));
  globalAxes.push_back(math::Vector3(0, -1, 0));
  globalAxes.push_back(math::Vector3(sqrt1_2, -sqrt1_2, 0));

  // Link names
  std::vector<std::string> linkNames;
  linkNames.push_back("lower_link");
  linkNames.push_back("upper_link");

  // Link names
  std::vector<std::string> jointNames;
  jointNames.push_back("lower_joint");
  jointNames.push_back("upper_joint");

  physics::ModelPtr model;
  physics::LinkPtr link;
  std::vector<std::string>::iterator modelIter;
  physics::JointPtr joint;

  // Check global axes before simulation starts
  std::vector<math::Vector3>::iterator axisIter;
  axisIter = globalAxes.begin();
  for (modelIter  = modelNames.begin();
       modelIter != modelNames.end(); ++modelIter)
  {
    model = world->GetModel(*modelIter);
    if (model)
    {
      gzdbg << "Check global axes of model " << *modelIter << '\n';
      std::vector<std::string>::iterator jointIter;
      for (jointIter  = jointNames.begin();
           jointIter != jointNames.end(); ++jointIter)
      {
        joint = model->GetJoint(*jointIter);
        if (joint)
        {
          math::Vector3 axis = joint->GetGlobalAxis(0);
          EXPECT_NEAR(axis.x, (*axisIter).x, PHYSICS_TOL);
          EXPECT_NEAR(axis.y, (*axisIter).y, PHYSICS_TOL);
          EXPECT_NEAR(axis.z, (*axisIter).z, PHYSICS_TOL);
        }
        else
        {
          gzerr << "Error loading joint " << *jointIter
                << " of model " << *modelIter << '\n';
          EXPECT_TRUE(joint != NULL);
        }
      }
    }
    else
    {
      gzerr << "Error loading model " << *modelIter << '\n';
      EXPECT_TRUE(model != NULL);
    }
    ++axisIter;
  }

  // Step forward 0.75 seconds
  double dt = physics->GetMaxStepSize();
  EXPECT_GT(dt, 0);
  int steps = ceil(0.75 / dt);
  world->StepWorld(steps);

  // Get global angular velocity of each link
  math::Vector3 angVel;
  for (modelIter  = modelNames.begin();
       modelIter != modelNames.end(); ++modelIter)
  {
    model = world->GetModel(*modelIter);
    if (model)
    {
      gzdbg << "Check angular velocity of model " << *modelIter << '\n';
      link = model->GetLink("base");
      if (link)
      {
        // Expect stationary base
        angVel = link->GetWorldAngularVel();
        EXPECT_NEAR(angVel.x, 0, PHYSICS_TOL*10);
        EXPECT_NEAR(angVel.y, 0, PHYSICS_TOL*10);
        EXPECT_NEAR(angVel.z, 0, PHYSICS_TOL*10);
      }
      else
      {
        gzerr << "Error loading base link of model " << *modelIter << '\n';
        EXPECT_TRUE(link != NULL);
      }

      std::vector<std::string>::iterator linkIter;
      for (linkIter  = linkNames.begin();
           linkIter != linkNames.end(); ++linkIter)
      {
        link = model->GetLink(*linkIter);
        if (link)
        {
          // Expect relative angular velocity of pendulum links to be negative
          // and along x axis.
          angVel = link->GetRelativeAngularVel().Normalize();
          EXPECT_NEAR(angVel.x, -1, PHYSICS_TOL);
          EXPECT_NEAR(angVel.y,  0, 2*PHYSICS_TOL);
          EXPECT_NEAR(angVel.z,  0, 2*PHYSICS_TOL);
        }
        else
        {
          gzerr << "Error loading link " << *linkIter
                << " of model " << *modelIter << '\n';
          EXPECT_TRUE(link != NULL);
        }
      }
    }
    else
    {
      gzerr << "Error loading model " << *modelIter << '\n';
      EXPECT_TRUE(model != NULL);
    }
  }

  // Keep stepping forward, verifying that joint angles move in the direction
  // implied by the joint velocity
  for (modelIter  = modelNames.begin();
       modelIter != modelNames.end(); ++modelIter)
  {
    model = world->GetModel(*modelIter);
    if (model)
    {
      double jointVel1, jointVel2;
      double angle1, angle2, angle3;

      gzdbg << "Check angle measurement for " << *modelIter << '\n';
      std::vector<std::string>::iterator jointIter;
      for (jointIter  = jointNames.begin();
           jointIter != jointNames.end(); ++jointIter)
      {
        joint = model->GetJoint(*jointIter);
        if (joint)
        {
          // Get first joint angle
          angle1 = joint->GetAngle(0).Radian();

          // Get joint velocity and assume it is not too small
          jointVel1 = joint->GetVelocity(0);
          EXPECT_GT(fabs(jointVel1), 1e-1);

          // Take 1 step and measure again
          world->StepWorld(1);

          // Expect angle change in direction of joint velocity
          angle2 = joint->GetAngle(0).Radian();
          EXPECT_GT((angle2 - angle1) * math::clamp(jointVel1*1e4, -1.0, 1.0)
                    , 0);

          jointVel2 = joint->GetVelocity(0);
          EXPECT_GT(fabs(jointVel2), 1e-1);

          // Take 1 step and measure the last angle, expect decrease
          world->StepWorld(1);
          angle3 = joint->GetAngle(0).Radian();
          EXPECT_GT((angle3 - angle2) * math::clamp(jointVel2*1e4, -1.0, 1.0)
                    , 0);
        }
        else
        {
          gzerr << "Error loading joint " << *jointIter
                << " of model " << *modelIter << '\n';
          EXPECT_TRUE(joint != NULL);
        }
      }
    }
    else
    {
      gzerr << "Error loading model " << *modelIter << '\n';
      EXPECT_TRUE(model != NULL);
    }
  }


  // Reset the world, and impose joint limits
  world->Reset();

  for (modelIter  = modelNames.begin();
       modelIter != modelNames.end(); ++modelIter)
  {
    model = world->GetModel(*modelIter);
    if (model)
    {
      std::vector<std::string>::iterator jointIter;
      for (jointIter  = jointNames.begin();
           jointIter != jointNames.end(); ++jointIter)
      {
        joint = model->GetJoint(*jointIter);
        if (joint)
        {
          joint->SetLowStop(0, math::Angle(-0.1));
          joint->SetHighStop(0, math::Angle(0.1));
        }
        else
        {
          gzerr << "Error loading joint " << *jointIter
                << " of model " << *modelIter << '\n';
          EXPECT_TRUE(joint != NULL);
        }
      }
    }
    else
    {
      gzerr << "Error loading model " << *modelIter << '\n';
      EXPECT_TRUE(model != NULL);
    }
  }

  // Step forward again for 0.75 seconds and check that joint angles
  // are within limits
  world->StepWorld(steps);
  for (modelIter  = modelNames.begin();
       modelIter != modelNames.end(); ++modelIter)
  {
    model = world->GetModel(*modelIter);
    if (model)
    {
      gzdbg << "Check angle limits and velocity of joints of model "
            << *modelIter << '\n';
      std::vector<std::string>::iterator jointIter;
      for (jointIter  = jointNames.begin();
           jointIter != jointNames.end(); ++jointIter)
      {
        joint = model->GetJoint(*jointIter);
        if (joint)
        {
          EXPECT_NEAR(joint->GetAngle(0).Radian(), 0, 0.11);
        }
        else
        {
          gzerr << "Error loading joint " << *jointIter
                << " of model " << *modelIter << '\n';
          EXPECT_TRUE(joint != NULL);
        }
      }
    }
    else
    {
      gzerr << "Error loading model " << *modelIter << '\n';
      EXPECT_TRUE(model != NULL);
    }
  }

  // Reset world again, disable gravity, detach upper_joint
  // Then apply torque at lower_joint and verify motion
  world->Reset();
  for (modelIter  = modelNames.begin();
       modelIter != modelNames.end(); ++modelIter)
  {
    model = world->GetModel(*modelIter);
    if (model)
    {
      gzdbg << "Check SetForce for model " << *modelIter << '\n';
      std::vector<std::string>::iterator linkIter;
      for (linkIter  = linkNames.begin();
           linkIter != linkNames.end(); ++linkIter)
      {
        link = model->GetLink(*linkIter);
        if (link)
        {
          // Disable gravity for links.
          link->SetGravityMode(false);
        }
        else
        {
          gzerr << "Error loading link " << *linkIter
                << " of model " << *modelIter << '\n';
          EXPECT_TRUE(link != NULL);
        }
      }

      joint = model->GetJoint("upper_joint");
      if (joint)
      {
        // Detach upper_joint.
        // joint->Detach();
        // Simbody and DART will not easily detach joints,
        // freeze joint limit instead
        math::Angle curAngle = joint->GetAngle(0u);
        joint->SetLowStop(0, curAngle);
        joint->SetHighStop(0, curAngle);
      }
      else
      {
        gzerr << "Error loading upper_joint "
              << " of model " << *modelIter << '\n';
        EXPECT_TRUE(joint != NULL);
      }

      // Step forward and let things settle a bit.
      world->StepWorld(100);

      joint = model->GetJoint("lower_joint");
      if (joint)
      {
        double oldVel, newVel, force;
        oldVel = joint->GetVelocity(0);
        // Apply positive torque to the lower_joint and step forward.
        force = 1;

        for (int i = 0; i < 10; ++i)
        {
          joint->SetForce(0, force);
          world->StepWorld(1);
          joint->SetForce(0, force);
          world->StepWorld(1);
          joint->SetForce(0, force);
          world->StepWorld(1);
          newVel = joint->GetVelocity(0);

          gzdbg << "model " << *modelIter
                << "  i " << i
                << "  oldVel " << oldVel
                << "  newVel " << newVel
                << "  upper joint v "
                << model->GetJoint("upper_joint")->GetVelocity(0)
                << " joint " << joint->GetName() <<  "\n";

          // Expect increasing velocities
          EXPECT_GT(newVel, oldVel);
          oldVel = newVel;

          // Check that GetForce returns what we set
          EXPECT_NEAR(joint->GetForce(0u), force, PHYSICS_TOL);

          // Expect joint velocity to be near angular velocity difference
          // of child and parent, along global axis
          // jointVel == DOT(angVelChild - angVelParent, axis)
          double jointVel = joint->GetVelocity(0);
          math::Vector3 axis = joint->GetGlobalAxis(0);
          angVel  = joint->GetChild()->GetWorldAngularVel();
          angVel -= joint->GetParent()->GetWorldAngularVel();
          EXPECT_NEAR(jointVel, axis.Dot(angVel), PHYSICS_TOL);
        }
        // Apply negative torque to lower_joint
        force = -3;
        for (int i = 0; i < 10; ++i)
        {
          joint->SetForce(0, force);
          world->StepWorld(1);
          joint->SetForce(0, force);
          world->StepWorld(1);
          joint->SetForce(0, force);
          world->StepWorld(1);
          newVel = joint->GetVelocity(0);

          gzdbg << "model " << *modelIter
                << "  i " << i
                << "  oldVel " << oldVel
                << "  newVel " << newVel
                << "  upper joint v "
                << model->GetJoint("upper_joint")->GetVelocity(0)
                << " joint " << joint->GetName() <<  "\n";

          // Expect decreasing velocities
          EXPECT_LT(newVel, oldVel);

          // Check that GetForce returns what we set
          EXPECT_NEAR(joint->GetForce(0u), force, PHYSICS_TOL);

          // Expect joint velocity to be near angular velocity difference
          // of child and parent, along global axis
          // jointVel == DOT(angVelChild - angVelParent, axis)
          double jointVel = joint->GetVelocity(0);
          math::Vector3 axis = joint->GetGlobalAxis(0);
          angVel  = joint->GetChild()->GetWorldAngularVel();
          angVel -= joint->GetParent()->GetWorldAngularVel();
          EXPECT_NEAR(jointVel, axis.Dot(angVel), PHYSICS_TOL);
        }
      }
      else
      {
        gzerr << "Error loading lower_joint "
              << " of model " << *modelIter << '\n';
        EXPECT_TRUE(joint != NULL);
      }
    }
    else
    {
      gzerr << "Error loading model " << *modelIter << '\n';
      EXPECT_TRUE(model != NULL);
    }
  }
}

TEST_P(PhysicsTest, RevoluteJoint)
{
  RevoluteJoint(GetParam());
}

/// \TODO: Redo state test
// TEST_F(PhysicsTest, State)
// {
  /*
  Load("worlds/empty.world");
  physics::WorldPtr world = physics::get_world("default");
  EXPECT_TRUE(world != NULL);

  physics::WorldState worldState = world->GetState();
  physics::ModelState modelState = worldState.GetModelState(0);
  physics::LinkState linkState = modelState.GetLinkState(0);
  physics::CollisionState collisionState = linkState.GetCollisionState(0);

  math::Pose pose;
  EXPECT_EQ(1u, worldState.GetModelStateCount());
  EXPECT_EQ(1u, modelState.GetLinkStateCount());
  EXPECT_EQ(1u, linkState.GetCollisionStateCount());
  EXPECT_EQ(pose, modelState.GetPose());
  EXPECT_EQ(pose, linkState.GetPose());
  EXPECT_EQ(pose, collisionState.GetPose());

  Unload();
  Load("worlds/shapes.world");
  world = physics::get_world("default");
  EXPECT_TRUE(world != NULL);
  worldState = world->GetState();

  for (unsigned int i = 0; i < worldState.GetModelStateCount(); ++i)
  {
    modelState = worldState.GetModelState(i);
    if (modelState.GetName() == "plane")
      pose.Set(math::Vector3(0, 0, 0), math::Quaternion(0, 0, 0));
    else if (modelState.GetName() == "box")
      pose.Set(math::Vector3(0, 0, 0.5), math::Quaternion(0, 0, 0));
    else if (modelState.GetName() == "sphere")
      pose.Set(math::Vector3(0, 1.5, 0.5), math::Quaternion(0, 0, 0));
    else if (modelState.GetName() == "cylinder")
      pose.Set(math::Vector3(0, -1.5, 0.5), math::Quaternion(0, 0, 0));

    EXPECT_TRUE(pose == modelState.GetPose());
  }

  // Move the box
  world->GetModel("box")->SetWorldPose(
      math::Pose(math::Vector3(1, 2, 0.5), math::Quaternion(0, 0, 0)));

  gazebo::common::Time::MSleep(10);

  // Make sure the box has been moved
  physics::ModelState modelState2 = world->GetState().GetModelState("box");
  pose.Set(math::Vector3(1, 2, 0.5), math::Quaternion(0, 0, 0));
  EXPECT_TRUE(pose == modelState2.GetPose());

  // Reset world state, and check for correctness
  world->SetState(worldState);
  modelState2 = world->GetState().GetModelState("box");
  pose.Set(math::Vector3(0, 0, 0.5), math::Quaternion(0, 0, 0));
  EXPECT_TRUE(pose == modelState2.GetPose());
  Unload();
  */
// }

void PhysicsTest::JointDampingTest(const std::string &_physicsEngine)
{
  // Random seed is set to prevent brittle failures (gazebo issue #479)
  math::Rand::SetSeed(18420503);
  Load("worlds/damp_test.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  int i = 0;
  while (!this->HasEntity("model_4_mass_1_ixx_1_damping_10") && i < 20)
  {
    common::Time::MSleep(100);
    ++i;
  }

  if (i > 20)
    gzthrow("Unable to get model_4_mass_1_ixx_1_damping_10");

  physics::ModelPtr model = world->GetModel("model_4_mass_1_ixx_1_damping_10");
  EXPECT_TRUE(model != NULL);

  {
    // compare against recorded data only
    double test_duration = 1.5;
    double dt = world->GetPhysicsEngine()->GetMaxStepSize();
    int steps = test_duration/dt;

    for (int i = 0; i < steps; ++i)
    {
      world->StepWorld(1);  // theoretical contact, but
      // gzdbg << "box time [" << world->GetSimTime().Double()
      //       << "] vel [" << model->GetWorldLinearVel()
      //       << "] pose [" << model->GetWorldPose()
      //       << "]\n";
    }

    EXPECT_EQ(world->GetSimTime().Double(), 1.5);

    // This test expects a linear velocity at the CoG
    math::Vector3 vel = model->GetLink()->GetWorldCoGLinearVel();
    math::Pose pose = model->GetWorldPose();

    EXPECT_EQ(vel.x, 0.0);

    EXPECT_NEAR(vel.y, -10.2009, PHYSICS_TOL);
    EXPECT_NEAR(vel.z, -6.51755, PHYSICS_TOL);

    EXPECT_DOUBLE_EQ(pose.pos.x, 3.0);
    EXPECT_NEAR(pose.pos.y, 0.0, PHYSICS_TOL);
    EXPECT_NEAR(pose.pos.z, 10.099, PHYSICS_TOL);
    EXPECT_NEAR(pose.rot.GetAsEuler().x, 0.567334, PHYSICS_TOL);
    EXPECT_DOUBLE_EQ(pose.rot.GetAsEuler().y, 0.0);
    EXPECT_DOUBLE_EQ(pose.rot.GetAsEuler().z, 0.0);
  }
}

TEST_P(PhysicsTest, JointDampingTest)
{
  JointDampingTest(GetParam());
}

void PhysicsTest::DropStuff(const std::string &_physicsEngine)
{
  Load("worlds/drop_test.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  EXPECT_TRUE(world != NULL);

  int i = 0;
  while (!this->HasEntity("cylinder") && i < 20)
  {
    common::Time::MSleep(100);
    ++i;
  }

  if (i > 20)
    gzthrow("Unable to get cylinder");

  {
    // todo: get parameters from drop_test.world
    double test_duration = 1.5;
    double z = 10.5;
    double v = 0.0;
    double g = -10.0;
    double dt = world->GetPhysicsEngine()->GetMaxStepSize();

    // world->StepWorld(1428);  // theoretical contact, but
    // world->StepWorld(100);  // integration error requires few more steps

    int steps = test_duration/dt;
    bool post_contact_correction = false;

    for (int i = 0; i < steps; ++i)
    {
      // integrate here to see when the collision should happen
      v = v + dt * g;
      z = z + dt * v;

      world->StepWorld(1);  // theoretical contact, but
      {
        physics::ModelPtr box_model = world->GetModel("box");
        if (box_model)
        {
          math::Vector3 vel = box_model->GetWorldLinearVel();
          math::Pose pose = box_model->GetWorldPose();
          // gzdbg << "box time [" << world->GetSimTime().Double()
          //      << "] sim z [" << pose.pos.z
          //      << "] exact z [" << z
          //      << "] sim vz [" << vel.z
          //      << "] exact vz [" << v << "]\n";
          if (z > 0.5 || !post_contact_correction)
          {
            EXPECT_LT(fabs(vel.z - v) , 0.0001);
            EXPECT_LT(fabs(pose.pos.z - z) , 0.0001);
          }
          else
          {
            EXPECT_LT(fabs(vel.z), 0.0101);  // sometimes -0.01, why?
            EXPECT_LT(fabs(pose.pos.z - 0.5), 0.00001);
          }
        }

        physics::ModelPtr sphere_model = world->GetModel("sphere");
        if (sphere_model)
        {
          math::Vector3 vel = sphere_model->GetWorldLinearVel();
          math::Pose pose = sphere_model->GetWorldPose();
          // gzdbg << "sphere time [" << world->GetSimTime().Double()
          //       << "] sim z [" << pose.pos.z
          //       << "] exact z [" << z
          //       << "] sim vz [" << vel.z
          //       << "] exact vz [" << v << "]\n";
          if (z > 0.5 || !post_contact_correction)
          {
            EXPECT_LT(fabs(vel.z - v), 0.0001);
            EXPECT_LT(fabs(pose.pos.z - z), 0.0001);
          }
          else
          {
            EXPECT_LT(fabs(vel.z), 3e-5);
            EXPECT_LT(fabs(pose.pos.z - 0.5), 0.00001);
          }
        }

        physics::ModelPtr cylinder_model = world->GetModel("cylinder");
        if (cylinder_model)
        {
          math::Vector3 vel = cylinder_model->GetWorldLinearVel();
          math::Pose pose = cylinder_model->GetWorldPose();
          // gzdbg << "cylinder time [" << world->GetSimTime().Double()
          //       << "] sim z [" << pose.pos.z
          //       << "] exact z [" << z
          //       << "] sim vz [" << vel.z
          //       << "] exact vz [" << v << "]\n";
          if (z > 0.5 || !post_contact_correction)
          {
            EXPECT_LT(fabs(vel.z - v), 0.0001);
            EXPECT_LT(fabs(pose.pos.z - z), 0.0001);
          }
          else
          {
            EXPECT_LT(fabs(vel.z), 0.011);
            EXPECT_LT(fabs(pose.pos.z - 0.5), 0.0001);
          }
        }
      }
      if (z < 0.5) post_contact_correction = true;
    }
  }
}

// This test doesn't pass yet in Bullet
TEST_P(PhysicsTest, DropStuff)
{
  DropStuff(GetParam());
}

void PhysicsTest::InelasticCollision(const std::string &_physicsEngine)
{
  // check conservation of mementum for linear inelastic collision
  Load("worlds/collision_test.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  EXPECT_TRUE(world != NULL);

  int i = 0;
  while (!this->HasEntity("sphere") && i < 20)
  {
    common::Time::MSleep(100);
    ++i;
  }

  if (i > 20)
    gzthrow("Unable to get sphere");

  {
    // todo: get parameters from drop_test.world
    double test_duration = 1.1;
    double dt = world->GetPhysicsEngine()->GetMaxStepSize();

    physics::ModelPtr box_model = world->GetModel("box");
    physics::LinkPtr box_link = box_model->GetLink("link");
    double f = 1000.0;
    double v = 0;
    double x = 0;
    double m = box_link->GetInertial()->GetMass();

    int steps = test_duration/dt;

    for (int i = 0; i < steps; ++i)
    {
      double t = world->GetSimTime().Double();

      world->StepWorld(1);  // theoretical contact, but
      {
        if (box_model)
        {
          math::Vector3 vel = box_model->GetWorldLinearVel();
          math::Pose pose = box_model->GetWorldPose();

          gzdbg << "box time [" << t
               << "] sim x [" << pose.pos.x
               << "] ideal x [" << x
               << "] sim vx [" << vel.x
               << "] ideal vx [" << v
               << "]\n";

          if (i == 0)
            box_model->GetLink("link")->SetForce(math::Vector3(f, 0, 0));

          if (t > 1.000 && t < 1.01)
          {
            // collision transition, do nothing
          }
          else
          {
            // collision happened
            EXPECT_NEAR(pose.pos.x, x, PHYSICS_TOL);
            EXPECT_NEAR(vel.x, v, PHYSICS_TOL);
          }
        }

        physics::ModelPtr sphere_model = world->GetModel("sphere");
        if (sphere_model)
        {
          math::Vector3 vel = sphere_model->GetWorldLinearVel();
          math::Pose pose = sphere_model->GetWorldPose();
          gzdbg << "sphere time [" << world->GetSimTime().Double()
               << "] sim x [" << pose.pos.x
               << "] ideal x [" << x
               << "] sim vx [" << vel.x
               << "] ideal vx [" << v
               << "]\n";
          if (t > 1.000 && t < 1.01)
          {
            // collision transition, do nothing
          }
          else if (t <= 1.00)
          {
            // no collision
            EXPECT_EQ(pose.pos.x, 2);
            EXPECT_EQ(vel.x, 0);
          }
          else
          {
            // collision happened
            EXPECT_NEAR(pose.pos.x, x + 1.0, PHYSICS_TOL);
            EXPECT_NEAR(vel.x, v, PHYSICS_TOL);
          }
        }
      }

/*
      // integrate here to see when the collision should happen
      double impulse = dt*f;
      if (i == 0) v = v + impulse;
      else if (t >= 1.0) v = dt*f/ 2.0;  // inelastic col. w/ eqal mass.
      x = x + dt * v;
*/

      // integrate here to see when the collision should happen
      double vold = v;
      if (i == 0) v = vold + dt* (f / m);
      else if (t >= 1.0) v = dt*f/ 2.0;  // inelastic col. w/ eqal mass.
      x = x + dt * (v + vold) / 2.0;
    }
  }
}

TEST_P(PhysicsTest, InelasticCollision)
{
  InelasticCollision(GetParam());
}

void PhysicsTest::SimplePendulum(const std::string &_physicsEngine)
{
  Load("worlds/simple_pendulums.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);

  int i = 0;
  while (!this->HasEntity("model_1") && i < 20)
  {
    common::Time::MSleep(100);
    ++i;
  }

  if (i > 20)
    gzthrow("Unable to get model_1");

  physics::PhysicsEnginePtr physicsEngine = world->GetPhysicsEngine();
  EXPECT_TRUE(physicsEngine);
  physics::ModelPtr model = world->GetModel("model_1");
  EXPECT_TRUE(model);
  physics::LinkPtr link = model->GetLink("link_2");  // sphere link at end
  EXPECT_TRUE(link);

  double g = 9.81;
  double l = 10.0;
  double m = 10.0;

  double e_start;

  {
    // check velocity / energy
    math::Vector3 vel = link->GetWorldLinearVel();
    math::Pose pos = link->GetWorldPose();
    double pe = 9.81 * m * pos.pos.z;
    double ke = 0.5 * m * (vel.x*vel.x + vel.y*vel.y + vel.z*vel.z);
    e_start = pe + ke;
    // gzdbg << "total energy [" << e_start
    //       << "] pe[" << pe
    //       << "] ke[" << ke
    //       << "] p[" << pos.pos.z
    //       << "] v[" << vel
    //       << "]\n";
  }
  physicsEngine->SetMaxStepSize(0.0001);
  physicsEngine->SetSORPGSIters(1000);

  {
    // test with global contact_max_correcting_vel at 0 as set by world file
    //   here we expect significant energy loss as the velocity correction
    //   is set to 0
    int steps = 10;  // @todo: make this more general
    for (int i = 0; i < steps; i ++)
    {
      world->StepWorld(2000);
      {
        // check velocity / energy
        math::Vector3 vel = link->GetWorldLinearVel();
        math::Pose pos = link->GetWorldPose();
        double pe = 9.81 * m * pos.pos.z;
        double ke = 0.5 * m * (vel.x*vel.x + vel.y*vel.y + vel.z*vel.z);
        double e = pe + ke;
        double e_tol = 3.0*static_cast<double>(i+1)
          / static_cast<double>(steps);
        // gzdbg << "total energy [" << e
        //       << "] pe[" << pe
        //       << "] ke[" << ke
        //       << "] p[" << pos.pos.z
        //       << "] v[" << vel
        //       << "] error[" << e - e_start
        //       << "] tol[" << e_tol
        //       << "]\n";

        EXPECT_LT(fabs(e - e_start), e_tol);
      }

      physics::JointPtr joint = model->GetJoint("joint_0");
      if (joint)
      {
        double integ_theta = (
          PendulumAngle(g, l, 1.57079633, 0.0, world->GetSimTime().Double(),
          0.000001) - 1.5707963);
        double actual_theta = joint->GetAngle(0).Radian();
        // gzdbg << "time [" << world->GetSimTime().Double()
        //       << "] exact [" << integ_theta
        //       << "] actual [" << actual_theta
        //       << "] pose [" << model->GetWorldPose()
        //       << "]\n";
         EXPECT_LT(fabs(integ_theta - actual_theta) , 0.01);
      }
    }
  }



  {
    // test with global contact_max_correcting_vel at 100
    // here we expect much lower energy loss
    world->Reset();
    physicsEngine->SetContactMaxCorrectingVel(100);

    int steps = 10;  // @todo: make this more general
    for (int i = 0; i < steps; i ++)
    {
      world->StepWorld(2000);
      {
        // check velocity / energy
        math::Vector3 vel = link->GetWorldLinearVel();
        math::Pose pos = link->GetWorldPose();
        double pe = 9.81 * m * pos.pos.z;
        double ke = 0.5 * m * (vel.x*vel.x + vel.y*vel.y + vel.z*vel.z);
        double e = pe + ke;
        double e_tol = 3.0*static_cast<double>(i+1)
          / static_cast<double>(steps);
        // gzdbg << "total energy [" << e
        //       << "] pe[" << pe
        //       << "] ke[" << ke
        //       << "] p[" << pos.pos.z
        //       << "] v[" << vel
        //       << "] error[" << e - e_start
        //       << "] tol[" << e_tol
        //       << "]\n";

        EXPECT_LT(fabs(e - e_start), e_tol);
      }

      physics::JointPtr joint = model->GetJoint("joint_0");
      if (joint)
      {
        double integ_theta = (
          PendulumAngle(g, l, 1.57079633, 0.0, world->GetSimTime().Double(),
          0.000001) - 1.5707963);
        double actual_theta = joint->GetAngle(0).Radian();
        // gzdbg << "time [" << world->GetSimTime().Double()
        //       << "] exact [" << integ_theta
        //       << "] actual [" << actual_theta
        //       << "] pose [" << model->GetWorldPose()
        //       << "]\n";
         EXPECT_LT(fabs(integ_theta - actual_theta) , 0.01);
      }
    }
  }
}

TEST_P(PhysicsTest, SimplePendulum)
{
  SimplePendulum(GetParam());
}

////////////////////////////////////////////////////////////////////////
// CollisionFiltering:
// Load a world, spawn a model with two overlapping links. By default,
// the links should not collide with each other as they have the same
// parent model. Check the x and y velocities to see if they are 0
////////////////////////////////////////////////////////////////////////
void PhysicsTest::CollisionFiltering(const std::string &_physicsEngine)
{
  // load an empty world
  Load("worlds/empty.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  std::stringstream newModelStr;

  std::string modelName = "multiLinkModel";
  math::Pose modelPose(0, 0, 2, 0, 0, 0);
  math::Pose link01Pose(0, 0.1, 0, 0, 0, 0);
  math::Pose link02Pose(0, -0.1, 0, 0, 0, 0);

  // A model composed of two overlapping links at fixed y offset from origin
  newModelStr << "<sdf version='" << SDF_VERSION << "'>"
              << "<model name ='" << modelName << "'>"
              << "<pose>" << modelPose.pos.x << " "
                         << modelPose.pos.y << " "
                         << modelPose.pos.z << " "
                         << modelPose.rot.GetAsEuler().x << " "
                         << modelPose.rot.GetAsEuler().y << " "
                         << modelPose.rot.GetAsEuler().z << "</pose>"
              << "<link name ='link01'>"
              << "  <pose>" << link01Pose.pos.x << " "
                         << link01Pose.pos.y << " "
                         << link01Pose.pos.z << " "
                         << link01Pose.rot.GetAsEuler().x << " "
                         << link01Pose.rot.GetAsEuler().y << " "
                         << link01Pose.rot.GetAsEuler().z << "</pose>"
              << "  <collision name ='geom'>"
              << "    <geometry>"
              << "      <box><size>1 1 1</size></box>"
              << "    </geometry>"
              << "  </collision>"
              << "  <visual name ='visual'>"
              << "    <geometry>"
              << "      <box><size>1 1 1</size></box>"
              << "    </geometry>"
              << "  </visual>"
              << "</link>"
              << "<link name ='link02'>"
              << "  <pose>" << link02Pose.pos.x << " "
                         << link02Pose.pos.y << " "
                         << link02Pose.pos.z << " "
                         << link02Pose.rot.GetAsEuler().x << " "
                         << link02Pose.rot.GetAsEuler().y << " "
                         << link02Pose.rot.GetAsEuler().z << "</pose>"
              << "  <collision name ='geom'>"
              << "    <geometry>"
              << "      <box><size>1 1 1</size></box>"
              << "    </geometry>"
              << "  </collision>"
              << "  <visual name ='visual'>"
              << "    <geometry>"
              << "      <box><size>1 1 1</size></box>"
              << "    </geometry>"
              << "  </visual>"
              << "</link>"
              << "</model>"
              << "</sdf>";

  SpawnSDF(newModelStr.str());

  // Wait for the entity to spawn
  int i = 0;
  while (!this->HasEntity(modelName) && i < 20)
  {
    common::Time::MSleep(100);
    ++i;
  }
  if (i > 20)
    gzthrow("Unable to spawn model");

  world->StepWorld(5);
  physics::ModelPtr model = world->GetModel(modelName);

  math::Vector3 vel;

  physics::Link_V links = model->GetLinks();
  unsigned int linkCount = 2;
  EXPECT_EQ(links.size(), linkCount);
  for (physics::Link_V::const_iterator iter = links.begin();
      iter != links.end(); ++iter)
  {
    // Links should not repel each other hence expecting zero x, y vel
    vel = (*iter)->GetWorldLinearVel();
    EXPECT_EQ(vel.x, 0);
    EXPECT_EQ(vel.y, 0);

    // Model should be falling
    EXPECT_LT(vel.z, 0);
  }
}

/////////////////////////////////////////////////
TEST_P(PhysicsTest, CollisionFiltering)
{
  CollisionFiltering(GetParam());
}

/////////////////////////////////////////////////
// This test verifies that gazebo doesn't crash when collisions occur
// and the <world><physics><ode><max_contacts> value is zero.
// The crash was reported in issue #593 on bitbucket
TEST_F(PhysicsTest, ZeroMaxContactsODE)
{
  // Load an empty world
  Load("worlds/zero_max_contacts.world");
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  physics::ModelPtr model = world->GetModel("ground_plane");
  ASSERT_TRUE(model);
}

INSTANTIATE_TEST_CASE_P(PhysicsEngines, PhysicsTest, PHYSICS_ENGINE_VALUES);

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
