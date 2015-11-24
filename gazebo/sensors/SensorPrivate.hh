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
#ifndef _GAZEBO_SENSOR_PRIVATE_HH_
#define _GAZEBO_SENSOR_PRIVATE_HH_

#include <vector>
#include <mutex>
#include <map>
#include <string>
#include <sdf/sdf.hh>
#include <ignition/math/Pose3.hh>

#include "gazebo/common/Event.hh"
#include "gazebo/common/Time.hh"
#include "gazebo/sensors/SensorTypes.hh"
#include "gazebo/sensors/PhysicsTypes.hh"
#include "gazebo/transport/TransportTypes.hh"

namespace gazebo
{
  namespace sensors
  {
    /// \internal
    /// \brief Sensor private data
    class SensorProtected
    {
      /// \brief True if sensor generation is active.
      protected: bool active;

      /// \brief Pointer the the SDF element for the sensor.
      protected: sdf::ElementPtr sdf;

      /// \brief Pose of the sensor.
      protected: ignition::math::Pose3d pose;

      /// \brief All event connections.
      protected: std::vector<event::ConnectionPtr> connections;

      /// \brief Node for communication.
      protected: transport::NodePtr node;

      /// \brief Subscribe to pose updates.
      protected: transport::SubscriberPtr poseSub;

      /// \brief Name of the parent.
      protected: std::string parentName;

      /// \brief The sensor's parent ID.
      protected: uint32_t parentId;

      /// \brief All the plugins for the sensor.
      protected: std::vector<SensorPluginPtr> plugins;

      /// \brief Pointer to the world.
      protected: gazebo::physics::WorldPtr world;

      /// \brief Pointer to the Scene
      protected: gazebo::rendering::ScenePtr scene;

      /// \brief Desired time between updates, set indirectly by
      ///        Sensor::SetUpdateRate.
      protected: common::Time updatePeriod;

      /// \brief Time of the last update.
      protected: common::Time lastUpdateTime;

      /// \brief Stores last time that a sensor measurement was generated;
      ///        this value must be updated within each sensor's UpdateImpl
      protected: common::Time lastMeasurementTime;

      /// \brief Noise added to sensor data
      /// The key maps to a SensorNoiseType, and is kept as an int value
      /// for backward compatibilty with Gazebo 5&6.
      /// \todo: Change to std::map<SensorNoiseType, NoisePtr> in Gazebo7.
      /// Adding the word GAZEBO_DEPRECATED here so that a grep will find
      /// the above note.
      protected: std::map<int, NoisePtr> noises;
    };

    class SensorPrivate
    {
      /// \brief Mutex to protect resetting lastUpdateTime.
      public: std::mutex mutexLastUpdateTime;

      /// \brief Event triggered when a sensor is updated.
      public: event::EventT<void()> updated;

      /// \brief Subscribe to control message.
      public: transport::SubscriberPtr controlSub;

      /// \brief Publish sensor data.
      public: transport::PublisherPtr sensorPub;

      /// \brief The category of the sensor.
      public: SensorCategory category;

      /// \brief Keep track how much the update has been delayed.
      public: common::Time updateDelay;

      /// \brief The sensors unique ID.
      public: uint32_t id;

      /// \brief An SDF pointer that allows us to only read the sensor.sdf
      /// file once, which in turns limits disk reads.
      public: static sdf::ElementPtr sdfSensor;
    };
    /// \}
  }
}
#endif
