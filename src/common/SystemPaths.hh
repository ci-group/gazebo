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
 * Author: Jordi Polo
 * Date: 3 May 2008
 */

#ifndef GAZEBOCONFIG_HH
#define GAZEBOCONFIG_HH

#include <string>
#include <list>
#include <stdio.h>

#include "common/SingletonT.hh"

#define LINUX
#ifdef WINDOWS
 #include <direct.h>
 #define GetCurrentDir _getcwd
#else
 #include <unistd.h>
 #define GetCurrentDir getcwd
#endif

namespace gazebo
{
	namespace common
  {
    class SystemPaths
    {
      /// \brief Get the gazebo install paths
      public: static const std::list<std::string> &GetGazeboPaths(); 

      /// \brief Get the ogre install paths  
      public: static const std::list<std::string> &GetOgrePaths(); 

      /// \brief Get the plugin paths  
      public: static const std::list<std::string> &GetPluginPaths(); 

      /// \brief Get the model path extension
      public: static std::string GetModelPathExtension();

     /// \brief Get the world path extension
      public: static std::string GetWorldPathExtension();

      public: static std::string FindFileWithGazeboPaths(std::string filename);
  
    };
  }
}
#endif
