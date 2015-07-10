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
#include <sys/time.h>
#include <gtest/gtest.h>
#include <ignition/math/Vector3d.hh>

#include "gazebo/test/ServerFixture.hh"
#include "gazebo/test/helper_physics_generator.hh"
#include "gazebo/sensors/MagnetometerSensor.hh"

#define TOL 1e-4

using namespace gazebo;

class MagnetometerSensor_TEST : public ServerFixture,
  public testing::WithParamInterface<const char*>
{
  /// \brief Check that a model at (0,0,0,0,0,0) has mag field equal to global
  public: void BasicMagnetometerSensorCheck(const std::string &_physicsEngine);

  /// \brief Drop a model with imu sensor and measure its linear acceleration
  public: void RotateMagnetometerSensorCheck(const std::string &_physicsEngine);
};

// A noise-free magnetic field strength sensor
static std::string magSensorString =
"<sdf version='1.5'>"
"  <sensor name='magnetometer' type='magnetometer'>"
"    <always_on>1</always_on>"
"    <update_rate>10.0</update_rate>"
"    <magnetometer>"
"    </magnetometer>"
"  </sensor>"
"</sdf>";

/////////////////////////////////////////////////
void MagnetometerSensor_TEST::BasicMagnetometerSensorCheck(
  const std::string &_physicsEngine)
{
  Load("worlds/empty.world", false, _physicsEngine);
  sensors::SensorManager *mgr = sensors::SensorManager::Instance();

  physics::WorldPtr world = physics::get_world("default");

  sdf::ElementPtr sdf(new sdf::Element);
  sdf::initFile("sensor.sdf", sdf);
  sdf::readString(magSensorString, sdf);

  // Create the IMU sensor
  std::string sensorName = mgr->CreateSensor(sdf, "default",
      "ground_plane::link", 0);

  // Make sure the returned sensor name is correct
  EXPECT_EQ(sensorName,
    std::string("default::ground_plane::link::magnetometer"));

  // Update the sensor manager so that it can process new sensors.
  mgr->Update();

  // Get a pointer to the IMU sensor
  sensors::MagnetometerSensorPtr sensor =
    boost::dynamic_pointer_cast<sensors::MagnetometerSensor>
      (mgr->GetSensor(sensorName));

  // Make sure the above dynamic cast worked.
  EXPECT_TRUE(sensor != NULL);

  // At pose [0,0,0,0,0,0] the body frame magnetic field should be default
  EXPECT_EQ(sensor->MagneticField(),
      world->GetPhysicsEngine()->MagneticField());
}

void MagnetometerSensor_TEST::RotateMagnetometerSensorCheck(
  const std::string &_physicsEngine)
{
  Load("worlds/empty.world", true, _physicsEngine);
  physics::WorldPtr world = physics::get_world("default");
  ASSERT_TRUE(world != NULL);

  // Verify physics engine type
  physics::PhysicsEnginePtr physics = world->GetPhysicsEngine();
  ASSERT_TRUE(physics != NULL);
  EXPECT_EQ(physics->GetType(), _physicsEngine);

  // Spawna  magnetometer sensor with a PI/2 aniclockwise rotation about U axis
  std::string modelName = "magModel";
  std::string magSensorName = "magSensor";
  ignition::math::Pose modelPose(0, 0, 0, 0, 0, 1.57079632679);
  std::string topic = "~/" + magSensorName + "_" + _physicsEngine;
  SpawnUnitMagnetometerSensor(modelName, magSensorName,
      "box", topic, modelPose.pos, modelPose.rot.GetAsEuler());

  sensors::SensorPtr sensor = sensors::get_sensor(magSensorName);
  sensors::MagnetometerSensorPtr magSensor =
      boost::dynamic_pointer_cast<sensors::MagnetometerSensor>(sensor);

  ASSERT_TRUE(magSensor != NULL);

  sensors::SensorManager::Instance()->Init();
  magSensor->SetActive(true);

  // Determine the magnetic field in the body frame
  ignition::math::Vector3d field = world->GetPhysicsEngine()->MagneticField();
  field = modelPose.rot.GetInverse().RotateVector(M);

  // Check for match
  EXPECT_EQ(magSensor->MagneticField(), field);
}

/////////////////////////////////////////////////
TEST_P(MagnetometerSensor_TEST, BasicMagnetometerSensorCheck)
{
  BasicMagnetometerSensorCheck(GetParam());
}

/////////////////////////////////////////////////
TEST_P(MagnetometerSensor_TEST, RotateMagnetometerSensorCheck)
{
  RotateMagnetometerSensorCheck(GetParam());
}

INSTANTIATE_TEST_CASE_P(PhysicsEngines, MagnetometerSensor_TEST,
                        PHYSICS_ENGINE_VALUES);

/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
