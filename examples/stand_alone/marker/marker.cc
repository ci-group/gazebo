/*
 * Copyright (C) 2012-2015 Open Source Robotics Foundation
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
#include <ignition/math/transport.hh>
#include <gazebo/msgs/msgs.hh>

#include <iostream>

/////////////////////////////////////////////////
int main(int _argc, char **_argv)
{
  // Load gazebo
  //gazebo::client::setup(_argc, _argv);

  // Create our node for communication
  //gazebo::transport::NodePtr node(new gazebo::transport::Node());
  //node->Init();

  ignition::transport::Node node;

  // Publish to a Gazebo topic
  // gazebo::transport::PublisherPtr pub =
  node.Advertise<gazebo::msgs::Marker>("/marker");

  // Wait for a subscriber to connect
  // node.WaitForConnection();

  // Create the marker message
  gazebo::msgs::Marker markerMsg;
  markerMsg.set_ns("default");
  markerMsg.set_id(0);
  markerMsg.set_action(gazebo::msgs::Marker::ADD_MODIFY);
  markerMsg.set_type(gazebo::msgs::Marker::SPHERE);

  gazebo::msgs::Material *matMsg = markerMsg.mutable_material();
  matMsg->mutable_script()->set_name("Gazebo/BlueLaser");

  // The rest of this function add different shapes and/or modifies shapes
  // Read the std::cout statements to figure out what each node.Publish
  // accomplishes.

  std::cout << "Spawning a sphere at the origin\n";
  gazebo::common::Time::Sleep(4);
  node.Publish(markerMsg);

  std::cout << "Moving the sphere to x=0, y=0, z=1\n";
  gazebo::msgs::Set(markerMsg.mutable_pose(),
                    ignition::math::Pose3d(0, 0, 1, 0, 0, 0));
  gazebo::common::Time::Sleep(4);
  node.Publish(markerMsg);

  std::cout << "Shrinking the sphere\n";
  gazebo::msgs::Set(markerMsg.mutable_scale(),
                    ignition::math::Vector3d(0.2, 0.2, 0.2));
  gazebo::common::Time::Sleep(4);
  node.Publish(markerMsg);

  std::cout << "Changing the sphere to red\n";
  matMsg->mutable_script()->set_name("Gazebo/Red");
  gazebo::common::Time::Sleep(4);
  node.Publish(markerMsg);

  std::cout << "Adding a green box\n";
  markerMsg.set_id(1);
  markerMsg.set_action(gazebo::msgs::Marker::ADD_MODIFY);
  markerMsg.set_type(gazebo::msgs::Marker::BOX);
  matMsg->mutable_script()->set_name("Gazebo/Green");
  gazebo::msgs::Set(markerMsg.mutable_scale(),
                    ignition::math::Vector3d(1.0, 1.0, 1.0));
  gazebo::msgs::Set(markerMsg.mutable_pose(),
                    ignition::math::Pose3d(2, 0, .5, 0, 0, 0));
  gazebo::common::Time::Sleep(4);
  node.Publish(markerMsg);

  gazebo::common::Time::Sleep(4);

  std::cout << "Change the green box to a cylinder\n";
  markerMsg.set_type(gazebo::msgs::Marker::CYLINDER);
  node.Publish(markerMsg);

  gazebo::common::Time::Sleep(4);

  std::cout << "Adding a line between the sphere and cylinder\n";
  markerMsg.set_id(2);
  gazebo::msgs::Set(markerMsg.mutable_pose(),
                    ignition::math::Pose3d(0, 0, 0, 0, 0, 0));
  markerMsg.set_action(gazebo::msgs::Marker::ADD_MODIFY);
  markerMsg.set_type(gazebo::msgs::Marker::LINE_LIST);
  gazebo::msgs::Set(markerMsg.add_point(), ignition::math::Vector3d(0, 0, 1.1));
  gazebo::msgs::Set(markerMsg.add_point(), ignition::math::Vector3d(2, 0, 0.5));
  node.Publish(markerMsg);

  gazebo::common::Time::Sleep(4);

  std::cout << "Adding a square around the origin\n";
  markerMsg.set_id(3);
  markerMsg.set_action(gazebo::msgs::Marker::ADD_MODIFY);
  markerMsg.set_type(gazebo::msgs::Marker::LINE_STRIP);
  gazebo::msgs::Set(markerMsg.mutable_point(0),
      ignition::math::Vector3d(0.5, 0.5, 0.05));
  gazebo::msgs::Set(markerMsg.mutable_point(1),
      ignition::math::Vector3d(0.5, -0.5, 0.05));
  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(-0.5, -0.5, 0.05));
  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(-0.5, 0.5, 0.05));
  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(0.5, 0.5, 0.05));
  node.Publish(markerMsg);

  gazebo::common::Time::Sleep(4);

  std::cout << "Adding 100 points inside the square\n";
  markerMsg.set_id(4);
  markerMsg.set_action(gazebo::msgs::Marker::ADD_MODIFY);
  markerMsg.set_type(gazebo::msgs::Marker::POINTS);
  markerMsg.clear_point();
  for (int i = 0; i < 100; ++i)
  {
    gazebo::msgs::Set(markerMsg.add_point(),
        ignition::math::Vector3d(
          ignition::math::Rand::DblUniform(-0.49, 0.49),
          ignition::math::Rand::DblUniform(-0.49, 0.49),
          0.05));
  }
  node.Publish(markerMsg);

  gazebo::common::Time::Sleep(4);

  std::cout << "Adding HELLO at 0, 0, 2\n";
  markerMsg.set_id(5);
  markerMsg.set_action(gazebo::msgs::Marker::ADD_MODIFY);
  markerMsg.set_type(gazebo::msgs::Marker::TEXT);
  markerMsg.set_text("HELLO");
  gazebo::msgs::Set(markerMsg.mutable_scale(),
                    ignition::math::Vector3d(0.2, 0.2, 0.2));
  gazebo::msgs::Set(markerMsg.mutable_pose(),
                    ignition::math::Pose3d(0, 0, 2, 0, 0, 0));
  node.Publish(markerMsg);

  gazebo::common::Time::Sleep(4);

  std::cout << "Adding a semi-circular triangle fan\n";
  markerMsg.set_id(6);
  markerMsg.set_action(gazebo::msgs::Marker::ADD_MODIFY);
  markerMsg.set_type(gazebo::msgs::Marker::TRIANGLE_FAN);
  markerMsg.clear_point();
  gazebo::msgs::Set(markerMsg.mutable_pose(),
                    ignition::math::Pose3d(0, 1.5, 0, 0, 0, 0));
  gazebo::msgs::Set(markerMsg.add_point(),
        ignition::math::Vector3d(0, 0, 0.05));
  double radius = 2;
  for (double t = 0; t <= M_PI; t+= 0.01)
  {
    gazebo::msgs::Set(markerMsg.add_point(),
        ignition::math::Vector3d(radius * cos(t), radius * sin(t), 0.05));
  }
  node.Publish(markerMsg);

  gazebo::common::Time::Sleep(4);

  std::cout << "Adding two triangles using a triangle list\n";
  markerMsg.set_id(7);
  markerMsg.set_action(gazebo::msgs::Marker::ADD_MODIFY);
  markerMsg.set_type(gazebo::msgs::Marker::TRIANGLE_LIST);
  markerMsg.clear_point();
  gazebo::msgs::Set(markerMsg.mutable_pose(),
                    ignition::math::Pose3d(0, -1.5, 0, 0, 0, 0));
  gazebo::msgs::Set(markerMsg.add_point(),
        ignition::math::Vector3d(0, 0, 0.05));
  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(1, 0, 0.05));
  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(1, 1, 0.05));

  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(1, 1, 0.05));
  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(2, 1, 0.05));
  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(2, 2, 0.05));

  node.Publish(markerMsg);

  gazebo::common::Time::Sleep(4);

  std::cout << "Adding a rectangular triangle strip\n";
  markerMsg.set_id(8);
  markerMsg.set_action(gazebo::msgs::Marker::ADD_MODIFY);
  markerMsg.set_type(gazebo::msgs::Marker::TRIANGLE_STRIP);
  markerMsg.clear_point();
  gazebo::msgs::Set(markerMsg.mutable_pose(),
                    ignition::math::Pose3d(-2, -2, 0, 0, 0, 0));
  gazebo::msgs::Set(markerMsg.add_point(),
        ignition::math::Vector3d(0, 0, 0.05));
  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(1, 0, 0.05));
  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(0, 1, 0.05));

  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(1, 1, 0.05));
  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(0, 2, 0.05));
  gazebo::msgs::Set(markerMsg.add_point(),
      ignition::math::Vector3d(1, 2, 0.05));

  node.Publish(markerMsg);

  gazebo::common::Time::Sleep(4);

  std::cout << "Delete all the markers\n";
  markerMsg.set_action(gazebo::msgs::Marker::DELETE_ALL);
  node.Publish(markerMsg);

  // Make sure to shut everything down.
  // gazebo::client::shutdown();
}
