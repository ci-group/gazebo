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
#ifndef BVHLOADER_HH
#define BVHLOADER_HH

#include <vector>
#include <map>
#include <string>

#include "math/Pose.hh"

#define X_POSITION 0
#define Y_POSITION 1
#define Z_POSITION 2
#define X_ROTATION 3
#define Y_ROTATION 4
#define Z_ROTATION 5

namespace gazebo
{
  namespace common
  {
    class Skeleton;

    class BVHLoader
    {
      public: BVHLoader();

      public: ~BVHLoader();

      public: Skeleton* Load(const std::string &filename, double scale);
    };
  }
}

#endif
