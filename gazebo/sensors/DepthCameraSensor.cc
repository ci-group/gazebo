/*
 * Copyright 2012 Open Source Robotics Foundation
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
/* Desc: A camera sensor using OpenGL
 * Author: Nate Koenig
 * Date: 15 July 2003
 */

#include <sstream>

#include "physics/World.hh"

#include "common/Events.hh"
#include "common/Exception.hh"

#include "transport/transport.hh"

#include "rendering/DepthCamera.hh"
#include "rendering/Scene.hh"
#include "rendering/Rendering.hh"

#include "sensors/SensorFactory.hh"
#include "sensors/DepthCameraSensor.hh"

using namespace gazebo;
using namespace sensors;

GZ_REGISTER_STATIC_SENSOR("depth", DepthCameraSensor)

//////////////////////////////////////////////////
DepthCameraSensor::DepthCameraSensor()
    : Sensor(sensors::IMAGE)
{
}

//////////////////////////////////////////////////
DepthCameraSensor::~DepthCameraSensor()
{
}

//////////////////////////////////////////////////
void DepthCameraSensor::SetParent(const std::string &_name)
{
  Sensor::SetParent(_name);
}

//////////////////////////////////////////////////
void DepthCameraSensor::Load(const std::string &_worldName,
                                   sdf::ElementPtr &_sdf)
{
  Sensor::Load(_worldName, _sdf);
}

//////////////////////////////////////////////////
void DepthCameraSensor::Load(const std::string &_worldName)
{
  Sensor::Load(_worldName);
}

//////////////////////////////////////////////////
void DepthCameraSensor::Init()
{
  std::string worldName = this->world->GetName();

  if (!worldName.empty())
  {
    this->scene = rendering::get_scene(worldName);

    if (!this->scene)
      this->scene = rendering::create_scene(worldName, false);

    this->camera = this->scene->CreateDepthCamera(
        this->sdf->GetValueString("name"), false);

    if (!this->camera)
    {
      gzerr << "Unable to create depth camera sensor\n";
      return;
    }
    this->camera->SetCaptureData(true);

    sdf::ElementPtr cameraSdf = this->sdf->GetElement("camera");
    this->camera->Load(cameraSdf);

    // Do some sanity checks
    if (this->camera->GetImageWidth() == 0 ||
        this->camera->GetImageHeight() == 0)
    {
      gzthrow("image has zero size");
    }

    this->camera->Init();
    this->camera->CreateRenderTexture(this->GetName() + "_RttTex_Image");
    this->camera->CreateDepthTexture(this->GetName() + "_RttTex_Depth");
    this->camera->SetWorldPose(this->pose);
    this->camera->AttachToVisual(this->parentName, true);
  }
  else
    gzerr << "No world name\n";

  // Disable clouds and moon on server side until fixed and also to improve
  // performance
  this->scene->SetSkyXMode(rendering::Scene::GZ_SKYX_ALL &
      ~rendering::Scene::GZ_SKYX_CLOUDS &
      ~rendering::Scene::GZ_SKYX_MOON);

  Sensor::Init();
}

//////////////////////////////////////////////////
void DepthCameraSensor::Fini()
{
  Sensor::Fini();
  this->scene->RemoveCamera(this->camera->GetName());
  this->camera.reset();
  this->scene.reset();
}

//////////////////////////////////////////////////
void DepthCameraSensor::SetActive(bool value)
{
  Sensor::SetActive(value);
}

//////////////////////////////////////////////////
bool DepthCameraSensor::UpdateImpl(bool /*_force*/)
{
  // Sensor::Update(force);
  if (!this->camera)
    return false;

  this->camera->Render();
  this->camera->PostRender();
  this->lastMeasurementTime = this->scene->GetSimTime();

  return true;
}

//////////////////////////////////////////////////
bool DepthCameraSensor::SaveFrame(const std::string &_filename)
{
  this->SetActive(true);
  return this->camera->SaveFrame(_filename);
}
