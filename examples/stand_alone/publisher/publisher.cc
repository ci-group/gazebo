/*
 * Copyright 2012 Open Source Robotics Foundation
 * Copyright 2013 Dereck Wonnacott
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

#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>
#include <math/gzmath.hh>

#include <iostream>


/////////////////////////////////////////////////
int main()
{
  // Initialize transport
  gazebo::transport::init();

  // Create our node for communication
  gazebo::transport::NodePtr node(new gazebo::transport::Node());
  node->Init();
  
  // Start transport
  gazebo::transport::run();

  // Publish to a Gazebo topic
  gazebo::transport::PublisherPtr pub = node->Advertise<gazebo::msgs::Pose>("~/pose_example");
  
  // Wait for a subscriber to connect
  pub->WaitForConnection();
  
  // Generate a pose
  gazebo::math::Pose pose(1,2,3,4,5,6);
  
  // Convert to a pose message
  gazebo::msgs::Pose msg;
  gazebo::msgs::Set(&msg, pose);
    
  // Busy wait loop...replace with your own code as needed.
  while (true)
  {
    gazebo::common::Time::MSleep(100);
    pub->Publish(msg);
  }  
    
  // Make sure to shut everything down.
  gazebo::transport::fini();
}
