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
#ifndef _PRESETMANAGER_PRIVATE_HH_
#define _PRESETMANAGER_PRIVATE_HH_

#include <map>
#include <string>
#include "gazebo/physics/PhysicsEngine.hh"

namespace gazebo
{
  namespace physics
  {
    class PresetPrivate
    {
      public: std::string name;
      public: std::map<std::string, boost::any> parameterMap;
      public: sdf::ElementPtr elementSDF;
    };

    class Preset;

    class PresetManagerPrivate
    {
      public: Preset* currentPreset;
      public: std::map<std::string, Preset> presetProfiles;
      public: PhysicsEnginePtr physicsEngine;
    };
  }  // namespace physics
}  // namespace gazebo 

#endif
