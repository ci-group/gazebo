/*
 * Copyright 2012 Nate Koenig
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
/* Desc: A persepective X11 OpenGL Camera Sensor
 * Author: Nate Koenig
 * Date: 15 July 2003
 */

#ifndef CAMERASENSOR_HH
#define CAMERASENSOR_HH

#include <string>

#include "sensors/Sensor.hh"
#include "msgs/MessageTypes.hh"
#include "transport/TransportTypes.hh"
#include "rendering/RenderTypes.hh"

namespace gazebo
{
  /// \ingroup gazebo_sensors
  /// \brief Sensors namespace
  namespace sensors
  {
    /// \class CameraSensor CameraSensor.hh sensors/sensors.hh
    /// \addtogroup gazebo_sensors Sensors
    /// \brief A set of sensor classes, functions, and definitions
    /// \{
    /// \brief Basic camera sensor
    /// This sensor is used for simulating standard monocular cameras
    class CameraSensor : public Sensor
    {
      /// \brief Constructor
      public: CameraSensor();

      /// \brief Destructor
      public: virtual ~CameraSensor();

      /// \brief Set the parent of the sensor
      /// \param _name The name of the parent
      public: virtual void SetParent(const std::string &_name);

      /// \brief Load the sensor with SDF parameters
      /// \param[in] _sdf SDF Sensor parameters
      /// \param[in] _worldName Name of world to load from
      public: virtual void Load(const std::string &_worldName,
                                sdf::ElementPtr _sdf);

      /// \brief Load the sensor with default parameters
      /// \param[in] _worldName Name of world to load from
      public: virtual void Load(const std::string &_worldName);

      /// \brief Initialize the camera
      public: virtual void Init();

      /// \brief Gets the topic name of the sensor
      /// \return Topic name
      /// @todo to be implemented
      public: virtual std::string GetTopic() const;

      /// \brief Update the sensor information
      /// \param[in] _force True if update is forced, false if not
      protected: virtual void UpdateImpl(bool _force);

      /// \brief Finalize the camera
      protected: virtual void Fini();

      /// \brief Set whether the sensor is active or not
      /// \param[in] _value True if active, false if not
      public: virtual void SetActive(bool _value);

      /// \brief Returns a pointer to the rendering::Camera
      /// \return The Pointer to the camera sensor
      /// \TODO: nate check
      public: rendering::CameraPtr GetCamera() const
              {return this->camera;}

      /// \brief Gets the width of the image in pixels
      /// \return The width in pixels of the image
      /// \TODO: nate check
      public: unsigned int GetImageWidth() const;

      /// \brief Gets the height of the image in pixels
      /// \return The height in pixels of the image
      /// \TODO: nate check
      public: unsigned int GetImageHeight() const;

      /// \brief Gets the raw image data from the sensor
      /// \return The pointer to the data array
      /// \TODO: nate check
      public: const unsigned char *GetImageData();

      /// \brief Saves the image to the disk
      /// \param[in] &_filename The name of the file to be saved
      /// \return True if successful, false if unsuccessful
      /// \TODO: nate check
      public: bool SaveFrame(const std::string &_filename);

      private: rendering::CameraPtr camera;
      private: rendering::ScenePtr scene;

      private: transport::PublisherPtr imagePub;
    };
    /// \}
  }
}
#endif
