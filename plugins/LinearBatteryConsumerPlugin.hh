/*
 * Copyright (C) 2015-2016 Open Source Robotics Foundation
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

#ifndef GAZEBO_PLUGINS_LINEARBATTERYCONSUMER_PLUGIN_HH_
#define GAZEBO_PLUGINS_LINEARBATTERYCONSUMER_PLUGIN_HH_

#include "gazebo/common/Plugin.hh"
#include "gazebo/common/CommonTypes.hh"

namespace gazebo
{
  /// \brief A plugin that manages a linear battery consumer.
  class GAZEBO_VISIBLE LinearBatteryConsumerPlugin : public ModelPlugin
  {
    /// \brief Constructor.
    public: LinearBatteryConsumerPlugin();

    /// \brief Destructor.
    public: ~LinearBatteryConsumerPlugin();

    // Documentation Inherited.
    public: virtual void Load(physics::ModelPtr _model, sdf::ElementPtr _sdf);

    /// \brief Pointer to battery.
    private: common::BatteryPtr battery;

    /// \brief Battery consumer identifier.
    private: int32_t consumerId;
  };
}
#endif
