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
#ifndef __PHYSICS_HH__
#define __PHYSICS_HH__

#include <string>

#include "physics/PhysicsTypes.hh"
#include "sdf/sdf.h"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{
    bool load();
    bool fini();

    WorldPtr create_world(const std::string &name ="");
    WorldPtr get_world(const std::string &name = "");

    void load_world(WorldPtr world, sdf::ElementPtr _sdf);
    void init_world(WorldPtr world);
    void run_world(WorldPtr world);
    void stop_world(WorldPtr world);
    void pause_world(WorldPtr world, bool pause);

    void load_worlds(sdf::ElementPtr _sdf);
    void init_worlds();
    void run_worlds();
    void stop_worlds();
    void pause_worlds(bool pause);
    void remove_worlds();

    /// \}
  }
}
#endif


