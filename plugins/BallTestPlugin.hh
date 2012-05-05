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
#ifndef __GAZEBO_BALL_TEST_PLUGIN_HH__
#define __GAZEBO_BALL_TEST_PLUGIN_HH__

#include "common/common.h"
#include "physics/physics.h"
#include "gazebo.hh"

namespace gazebo
{
  class BallTestPlugin : public WorldPlugin
  {
    public: BallTestPlugin();
    public: virtual void Load(physics::WorldPtr _world, sdf::ElementPtr _sdf);
    public: virtual void Init();

    private: void OnUpdate();
    private: event::ConnectionPtr updateConnection;

    private: int index;
    private: physics::WorldPtr world;
  };
}
#endif
