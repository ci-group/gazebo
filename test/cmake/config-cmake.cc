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
#include <stdio.h>

TEST(Cmake, Config)
{
  char cmd[1024];

  snprintf(cmd, sizeof(cmd), "cmake %s -DGAZEBO_VERSION=%f", SOURCE_DIR, GAZEBO_VERSION);
  ASSERT_EQ(system(cmd), 0);
  snprintf(cmd, sizeof(cmd), "make");
  ASSERT_EQ(system(cmd), 0);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
