/* Copyright 2011 Nate Koenig & Andrew Howard
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
#include "physics/Physics.hh"
#include "physics/World.hh"
#include "physics/Model.hh"
#include "math/Pose.hh"
#include "gazebo.h"

namespace gazebo
{ 
  class MoviePR2 : public ModelPlugin
  {
    public: MoviePR2();

    public: void Load(physics::ModelPtr _model, sdf::ElementPtr _sdf);
    public: virtual void Init();

    private: void TuckArms();
    private: void OnJointAnimation(ConstJointAnimationPtr &_anim);

    private: transport::NodePtr node;
    private: transport::SubscriberPtr jointAnimSub;

    private: physics::WorldPtr world;
    private: physics::ModelPtr pr2;
    private: event::Connection_V connections;
  };
}
