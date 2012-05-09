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
#include "physics/physics.h"
#include "common/Time.hh"

using namespace gazebo;
class Pioneer2dx : public ServerFixture
{
};

/////////////////////////////////////////////////
TEST_F(Pioneer2dx, StraightLine)
{
  Load("worlds/pioneer2dx.world");
  transport::PublisherPtr velPub = this->node->Advertise<gazebo::msgs::Pose>(
      "~/pioneer2dx/vel_cmd");

  gazebo::msgs::Pose msg;
  gazebo::msgs::Set(msg.mutable_position(),
      gazebo::math::Vector3(0.2, 0, 0));
  gazebo::msgs::Set(msg.mutable_orientation(),
      gazebo::math::Quaternion(0, 0, 0));
  velPub->Publish(msg);

  math::Pose startPose, endPose;
  startPose = this->poses["pioneer2dx"];

  common::Time startTime = this->simTime;
  common::Time currTime = this->simTime;

  struct timespec interval;
  struct timespec remainder;
  interval.tv_sec = 1 / 1000;
  interval.tv_nsec = (1 % 1000) * 1000000;

  while (currTime - startTime < common::Time(100, 0))
  {
    nanosleep(&interval, &remainder);
    common::Time::MSleep(100);
    currTime = this->simTime;
  }
  endPose = this->poses["pioneer2dx"];

  double dist = (currTime - startTime).Double() * 0.2;
  EXPECT_LT(fabs(endPose.pos.x - dist), 0.1);
  EXPECT_LT(fabs(endPose.pos.y), 0.5);
  EXPECT_LT(fabs(endPose.pos.z), 0.01);
}

/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
