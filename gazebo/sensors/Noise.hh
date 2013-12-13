/*
 * Copyright (C) 2012-2013 Open Source Robotics Foundation
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

#ifndef _GAZEBO_NOISE_HH_
#define _GAZEBO_NOISE_HH_

#include <vector>
#include <string>

#include <sdf/sdf.hh>

#include "gazebo/sensors/SensorTypes.hh"

namespace gazebo
{
  namespace sensors
  {
    /// \addtogroup gazebo_sensors
    /// \{

    /// \class NoiseManager Noise.hh
    /// \brief Use this noise manager for creating and loading noise models.
    class NoiseManager
    {
      /// \brief Load a noise model based on the input sdf parameters and
      /// sensor type.
      /// \param[in] _sdf Noise sdf parameters.
      /// \param[in] _sensorType Type of sensor. This is currently used to
      /// distinguish between image and non image sensors in order to create
      /// the appropriate noise model.
      /// \return Pointer to the noise model created.
      public: static NoisePtr LoadNoiseModel(sdf::ElementPtr _sdf,
          const std::string &_sensorType = "");
    };

    /// \class Noise Noise.hh
    /// \brief Noise models for sensor output signals.
    class Noise
    {
      /// \brief Which noise types we support
      public: enum NoiseType
      {
        NONE,
        GAUSSIAN,
        GAUSSIAN_QUANTIZED,
        CUSTOM
      };

      /// \brief Constructor.
      public: Noise();

      /// \brief Destructor.
      public: virtual ~Noise();

      /// \brief Load noise parameters from sdf.
      /// \param[in] _sdf SDF parameters.
      /// \param[in] _sensor Type of sensor.
      public: virtual void Load(sdf::ElementPtr _sdf);

      /// \brief Apply noise to input data value.
      /// \param[in] _in Input data value.
      /// \return Data with noise applied.
      public: double Apply(double _in) const;

      /// \brief Apply noise to input data value. This gets overriden by
      /// derived classes, and called by Apply.
      /// \param[in] _in Input data value.
      /// \return Data with noise applied.
      public: virtual double ApplyImpl(double _in) const;

      /// \brief Finalize the noise model
      public: virtual void Fini();

      /// \brief Accessor for NoiseType.
      /// \return Type of noise currently in use.
      public: NoiseType GetNoiseType() const;

      /// \brief Register a custom noise callback.
      /// \param[in] _cb Callback function for applying a custom noise model.
      /// This is useful if users want to use their own noise model from a
      /// sensor plugin.
      public: virtual void SetCustomNoiseCallback(
          boost::function<double (double)> _cb);

      /// \brief Noise sdf element.
      private: sdf::ElementPtr sdf;

      /// \brief Which type of noise we're applying
      private: NoiseType type;

      /// \brief Callback function for applying custom noise to sensor data.
      private: boost::function<double (double)> customNoiseCallback;
    };
    /// \}
  }
}
#endif
