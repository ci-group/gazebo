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
#include "gazebo/common/LogRecord.hh"

using namespace gazebo;
class LogRecordTest : public ServerFixture
{
};

TEST_F(LogRecordTest, Init)
{
  EXPECT_TRUE(common::LogRecord::Instance()->Init("test"));

  common::LogRecord::Instance()->Stop();
  common::LogRecord::Instance()->Stop();

  common::LogRecord::Instance()->Start();
  common::LogRecord::Instance()->Start();

  common::LogRecord::Instance()->Stop();
  common::LogRecord::Instance()->Start();


  common::LogRecord::Instance()->Start();
  common::LogRecord::Instance()->Stop();

  common::LogRecord::Instance()->Stop();
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
