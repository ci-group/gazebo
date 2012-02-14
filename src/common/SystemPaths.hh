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
/* Desc: Gazebo configuration on this computer
 * Date: 3 May 2008
 */

#ifndef GAZEBO_SYSTEMPATHS_HH
#define GAZEBO_SYSTEMPATHS_HH

#include <stdio.h>

#define LINUX
#ifdef WINDOWS
  #include <direct.h>
  #define GetCurrentDir _getcwd
#else
  #include <unistd.h>
  #define GetCurrentDir getcwd
#endif

#include <string>
#include <list>

#include "common/SingletonT.hh"


namespace gazebo
{
  namespace common
  {
    /// \addtogroup gazebo_common Common
    /// \{
    /// \brief Functions to handle getting system paths
    class SystemPaths : public SingletonT<SystemPaths>
    {
      private: SystemPaths();

      /// \brief Get the log path
      public: std::string GetLogPath() const;

      /// \brief Get the gazebo install paths
      public: const std::list<std::string> &GetGazeboPaths();

      /// \brief Get the ogre install paths
      public: const std::list<std::string> &GetOgrePaths();

      /// \brief Get the plugin paths
      public: const std::list<std::string> &GetPluginPaths();

      /// \brief Get the model path extension
      public: std::string GetModelPathExtension();

      /// \brief Get the world path extension
      public: std::string GetWorldPathExtension();

      public: std::string FindFileWithGazeboPaths(const std::string &_filename);

      /// \brief Add colon delimited paths to Gazebo install
      public: void AddGazeboPaths(std::string _path);

      /// \brief Add colon delimited paths to ogre install
      public: void AddOgrePaths(std::string _path);

      /// \brief Add colon delimited paths to plugins
      public: void AddPluginPaths(std::string _path);

      public: void ClearGazeboPaths();
      public: void ClearOgrePaths();
      public: void ClearPluginPaths();

      /// Paths gazebo install
      private: std::list<std::string> gazeboPaths;

      /// Paths to the ogre install
      private: std::list<std::string> ogrePaths;

      /// Paths to the plugins
      private: std::list<std::string> pluginPaths;

      private: std::string logPath;

      private: friend class SingletonT<SystemPaths>;
    };
    /// \}
  }
}
#endif


