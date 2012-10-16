/*
 * Copyright 2011 Nate Koenig
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
#ifndef SENSORTYPES_HH
#define SENSORTYPES_HH

#include <vector>
#include <boost/shared_ptr.hpp>

/// \file
/// \ingroup gazebo_sensors
/// \brief Forward declarations and typedefs for sensors
namespace gazebo
{
  /// \ingroup gazebo_sensors
  /// \brief Sensors namespace
  namespace sensors
  {
    class Sensor;
    class RaySensor;
    class CameraSensor;
    class DepthCameraSensor;
    class ContactSensor;
    class GpuRaySensor;
    class RFIDSensor;
    class RFIDTag;

    /// \TODO Nate check file
    /// \def SensorPtr
    /// \brief Shared pointer to Sensor
    typedef boost::shared_ptr<Sensor> SensorPtr;

    /// \def RaySensorPtr
    /// \brief Shared pointer to Ray Sensor
    typedef boost::shared_ptr<RaySensor> RaySensorPtr;

    /// \def CameraSensorPtr
    /// \brief Shared pointer to Camera Sensor
    typedef boost::shared_ptr<CameraSensor> CameraSensorPtr;

    /// \def DepthCameraSensorPtr
    /// \brief Shared pointer to Depth Camera Sensor
    typedef boost::shared_ptr<DepthCameraSensor> DepthCameraSensorPtr;

    /// \def ContactSensorPtr
    /// \brief Shared pointer to Contact Sensor
    typedef boost::shared_ptr<ContactSensor> ContactSensorPtr;

    /// \def GpuRaySensorPtr
    /// \brief Shared pointer to GpuRaySensor
    typedef boost::shared_ptr<GpuRaySensor> GpuRaySensorPtr;

    /// \def RFIDSensorPtr
    /// \brief Shared pointer to RFIDSensor
    typedef boost::shared_ptr<RFIDSensor> RFIDSensorPtr;
    
    /// \def RFIDTagPtr
    /// \brief Shared pointer to RFIDTag
    typedef boost::shared_ptr<RFIDTag> RFIDTagPtr;

    /// \def Sensor_V
    /// \brief Vector of Sensor shared pointers
    typedef std::vector<SensorPtr> Sensor_V;

    /// \def RaySensor_V
    /// \brief Vector of RaySensor shared pointers
    typedef std::vector<RaySensorPtr> RaySensor_V;

    /// \def CameraSensor_V
    /// \brief Vector of CameraSensor shared pointers
    typedef std::vector<CameraSensorPtr> CameraSensor_V;

    /// \def DepthCameraSensor_V
    /// \brief Vector of DepthCameraSensor shared pointers
    typedef std::vector<DepthCameraSensorPtr> DepthCameraSensor_V;

    /// \def ContactSensor_V
    /// \brief Vector of ContactSensor shared pointers
    typedef std::vector<ContactSensorPtr> ContactSensor_V;

    /// \def GpuRaySensor_V
    /// \brief Vector of GpuRaySensor shared pointers
    typedef std::vector<GpuRaySensorPtr> GpuRaySensor_V;

    /// \def RFIDSensor_V
    /// \brief Vector of RFIDSensor shared pointers
    typedef std::vector<RFIDSensor> RFIDSensor_V;

    /// \def RFIDTag_V
    /// \brief Vector of RFIDTag shared pointers
    typedef std::vector<RFIDTag> RFIDTag_V;
  }
}
#endif
