/*
 * Copyright 2011 Nate Koenig & Andrew Howard
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
#include "physics/Physics.hh"
#include "physics/physics.h"
#include "SimplePendulumIntegrator.hh"

using namespace gazebo;
class PhysicsTest : public ServerFixture
{
};


TEST_F(PhysicsTest, State)
{
  Load("worlds/empty.world");
  physics::WorldPtr world = physics::get_world("default");
  EXPECT_TRUE(world != NULL);

  physics::WorldState worldState = world->GetState();
  physics::ModelState modelState = worldState.GetModelState(0);
  physics::LinkState linkState = modelState.GetLinkState(0);
  physics::CollisionState collisionState = linkState.GetCollisionState(0);

  math::Pose pose;
  EXPECT_EQ(1, worldState.GetModelStateCount());
  EXPECT_EQ(1, modelState.GetLinkStateCount());
  EXPECT_EQ(1, linkState.GetCollisionStateCount());
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
}

TEST_F(PhysicsTest, CollisionTest)
{
  // check conservation of mementum for linear inelastic collision
  Load("worlds/collision_test.world", true);
  physics::WorldPtr world = physics::get_world("default");
  EXPECT_TRUE(world != NULL);

  {
    // todo: get parameters from drop_test.world
    double test_duration = 1.1;
    double dt = world->GetPhysicsEngine()->GetStepTime();

    double f = 1000.0;
    double v = 0;
    double x = 0;

    int steps = test_duration/dt;

    for (int i = 0; i < steps; i++)
    {
      double t = world->GetSimTime().Double();
      // gzdbg << "debug v[" << v << "] x[" << z << "]\n";

      world->StepWorld(1);  // theoretical contact, but
      {
        physics::ModelPtr box_model = world->GetModel("box");
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
            box_model->GetLink("link")->SetForce(math::Vector3(1000, 0, 0));
          EXPECT_LT(fabs(pose.pos.x - x), 0.00001);
          EXPECT_LT(fabs(vel.x - v), 0.00001);
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
          if (t < 1.001)
          {
            EXPECT_EQ(pose.pos.x, 2);
            EXPECT_EQ(vel.x, 0);
          }
          else
          {
            EXPECT_LT(fabs(pose.pos.x - x - 1.0), 0.00001);
            EXPECT_LT(fabs(vel.x - v), 0.00001);
          }
        }
      }

      // integrate here to see when the collision should happen
      double impulse = dt*f;
      if (i == 0) v = v + impulse;
      else if (t >= 1.0) v = dt*f/ 2.0;  // inelastic col. w/ eqal mass.
      x = x + dt * v;
    }
  }
  Unload();
}

TEST_F(PhysicsTest, DropStuff)
{
  Load("worlds/drop_test.world", true);
  physics::WorldPtr world = physics::get_world("default");
  EXPECT_TRUE(world != NULL);


  {
    // todo: get parameters from drop_test.world
    double test_duration = 1.5;
    double z = 10.5;
    double v = 0.0;
    double g = -10.0;
    double dt = world->GetPhysicsEngine()->GetStepTime();

    // world->StepWorld(1428);  // theoretical contact, but
    // world->StepWorld(100);  // integration error requires few more steps

    int steps = test_duration/dt;
    bool post_contact_correction = false;

    for (int i = 0; i < steps; i++)
    {
      // integrate here to see when the collision should happen
      v = v + dt * g;
      z = z + dt * v;
      // gzdbg << "debug v[" << v << "] x[" << z << "]\n";

      world->StepWorld(1);  // theoretical contact, but
      {
        physics::ModelPtr box_model = world->GetModel("box");
        if (box_model)
        {
          math::Vector3 vel = box_model->GetWorldLinearVel();
          math::Pose pose = box_model->GetWorldPose();
          gzdbg << "box time [" << world->GetSimTime().Double()
                << "] sim z [" << pose.pos.z
                << "] exact z [" << z
                << "] sim vz [" << vel.z
                << "] exact vz [" << v << "]\n";
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
          gzdbg << "sphere time [" << world->GetSimTime().Double()
                << "] sim z [" << pose.pos.z
                << "] exact z [" << z
                << "] sim vz [" << vel.z
                << "] exact vz [" << v << "]\n";
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
          gzdbg << "cylinder time [" << world->GetSimTime().Double()
                << "] sim z [" << pose.pos.z
                << "] exact z [" << z
                << "] sim vz [" << vel.z
                << "] exact vz [" << v << "]\n";
          if (z > 0.5 || !post_contact_correction)
          {
            EXPECT_LT(fabs(vel.z - v), 0.0001);
            EXPECT_LT(fabs(pose.pos.z - z), 0.0001);
          }
          else
          {
            EXPECT_LT(fabs(vel.z), 0.011);
            EXPECT_LT(fabs(pose.pos.z - 0.5), 0.00001);
          }
        }
      }
      if (z < 0.5) post_contact_correction = true;
    }
  }
  Unload();
}

TEST_F(PhysicsTest, SimplePendulumTest)
{
  Load("worlds/simple_pendulums.world", true);
  physics::WorldPtr world = physics::get_world("default");
  EXPECT_TRUE(world != NULL);

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
    gzdbg << "total energy [" << e_start
          << "] pe[" << pe
          << "] ke[" << ke
          << "] p[" << pos.pos.z
          << "] v[" << vel
          << "]\n";
  }
  physicsEngine->SetStepTime(0.0001);
  physicsEngine->SetSORPGSIters(1000);

  {
    /* test with global contact_max_correcting_vel at 0 as set by world file
       here we expect significant energy loss as the velocity correction
       is set to 0
    */
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
        gzdbg << "total energy [" << e
              << "] pe[" << pe
              << "] ke[" << ke
              << "] p[" << pos.pos.z
              << "] v[" << vel
              << "] error[" << e - e_start
              << "] tol[" << e_tol
              << "]\n";

        EXPECT_LT(fabs(e - e_start), e_tol);
      }

      physics::JointPtr joint = model->GetJoint("joint_0");
      if (joint)
      {
        double integ_theta = (
          PendulumAngle(g, l, 1.57079633, 0.0, world->GetSimTime().Double(),
          0.000001) - 1.5707963);
        double actual_theta = joint->GetAngle(0).GetAsRadian();
        gzdbg << "time [" << world->GetSimTime().Double()
              << "] exact [" << integ_theta
              << "] actual [" << actual_theta
              << "] pose [" << model->GetWorldPose()
              << "]\n";
         EXPECT_LT(fabs(integ_theta - actual_theta) , 0.01);
      }
    }
  }



  {
    /* test with global contact_max_correcting_vel at 100
       here we expect much lower energy loss
    */
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
        gzdbg << "total energy [" << e
              << "] pe[" << pe
              << "] ke[" << ke
              << "] p[" << pos.pos.z
              << "] v[" << vel
              << "] error[" << e - e_start
              << "] tol[" << e_tol
              << "]\n";

        EXPECT_LT(fabs(e - e_start), e_tol);
      }

      physics::JointPtr joint = model->GetJoint("joint_0");
      if (joint)
      {
        double integ_theta = (
          PendulumAngle(g, l, 1.57079633, 0.0, world->GetSimTime().Double(),
          0.000001) - 1.5707963);
        double actual_theta = joint->GetAngle(0).GetAsRadian();
        gzdbg << "time [" << world->GetSimTime().Double()
              << "] exact [" << integ_theta
              << "] actual [" << actual_theta
              << "] pose [" << model->GetWorldPose()
              << "]\n";
         EXPECT_LT(fabs(integ_theta - actual_theta) , 0.01);
      }
    }
  }
  Unload();
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
