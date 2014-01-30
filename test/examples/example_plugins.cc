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
#include <string>
#include <boost/filesystem.hpp>

#include "gazebo/common/Console.hh"

///////////////////////////////////////////////////////////////////
// Create a temporary build folder
boost::filesystem::path createTempBuildFolder(const std::string &_prefix)
{
  boost::filesystem::path path = boost::filesystem::temp_directory_path();
  path /= boost::filesystem::unique_path(_prefix + "-%%%%-%%%%-%%%%-%%%%");
  boost::filesystem::create_directories(path);
  gzdbg << "mkdir " << path.string() << std::endl;
  return path;
}

///////////////////////////////////////////////////////////////////
// Get path to source folder with specified suffix
boost::filesystem::path getSourcePath(const std::string &_suffix)
{
  boost::filesystem::path path = CMAKE_SOURCE_DIR;
  path /= std::string("examples");
  path /= std::string("plugins");
  path /= _suffix;
  gzdbg << "source " << path.string() << std::endl;
  return path;
}

///////////////////////////////////////////////////////////////////
// 
void BuildExamplePlugins(const std::string &_prefix,
                         const std::string &_suffix)
{
  // get a unique temporary build folder name
  boost::filesystem::path build = createTempBuildFolder(_prefix);
 
  // construct path of source folder
  boost::filesystem::path source = getSourcePath(_suffix);

  char cmd[1024];

  // cd build && cmake source
  snprintf(cmd, sizeof(cmd), "cd %s && cmake %s && make",
    build.c_str(), source.c_str());
  ASSERT_EQ(system(cmd), 0);
}

///////////////////////////////////////////////////////////////////
// This test verifies that the HelloWorld plugin builds successfully
// It doesn't try to execute the plugin.
TEST(ExamplePlugins, HelloWorld)
{
  BuildExamplePlugins("build_HelloWorld", "hello_world");
}

///////////////////////////////////////////////////////////////////
// This test verifies that the WorldEdit plugin builds successfully
// It doesn't try to execute the plugin.
TEST(ExamplePlugins, WorldEdit)
{
  BuildExamplePlugins("build_WorldEdit", "world_edit");
}

///////////////////////////////////////////////////////////////////
// This test verifies that the ModelPush plugin builds successfully
// It doesn't try to execute the plugin.
TEST(ExamplePlugins, ModelPush)
{
  BuildExamplePlugins("build_ModelPush", "model_push");
}

///////////////////////////////////////////////////////////////////
// This test verifies that the Factory plugin builds successfully
// It doesn't try to execute the plugin.
TEST(ExamplePlugins, Factory)
{
  BuildExamplePlugins("build_Factory", "factory");
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
