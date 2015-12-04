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
#include <mutex>

#include <boost/algorithm/string.hpp>

#include "gazebo/common/Events.hh"
#include "gazebo/common/Exception.hh"
#include "gazebo/common/Image.hh"

#include "gazebo/transport/transport.hh"
#include "gazebo/msgs/msgs.hh"

#include "gazebo/physics/World.hh"

#include "gazebo/rendering/RenderEngine.hh"
#include "gazebo/rendering/Camera.hh"
#include "gazebo/rendering/WideAngleCamera.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/RenderingIface.hh"

#include "gazebo/sensors/SensorFactory.hh"
#include "gazebo/sensors/CameraSensor.hh"
#include "gazebo/sensors/Noise.hh"
#include "gazebo/sensors/WideAngleCameraSensorPrivate.hh"
#include "gazebo/sensors/WideAngleCameraSensor.hh"

using namespace gazebo;
using namespace sensors;

GZ_REGISTER_STATIC_SENSOR("wideanglecamera", WideAngleCameraSensor)

//////////////////////////////////////////////////
WideAngleCameraSensor::WideAngleCameraSensor()
: CameraSensor(*new WideAngleCameraSensorPrivate),
  dataPtr(std::static_pointer_cast<WideAngleCameraSensorPrivate>(this->dPtr))
{
}

//////////////////////////////////////////////////
void WideAngleCameraSensor::Init()
{
  if (rendering::RenderEngine::Instance()->GetRenderPathType() ==
      rendering::RenderEngine::NONE)
  {
    gzerr << "Unable to create WideAngleCameraSensor. Rendering is disabled."
        << std::endl;
    return;
  }

  std::string worldName = this->dataPtr->world->GetName();

  if (!worldName.empty())
  {
    this->dataPtr->scene = rendering::get_scene(worldName);
    if (!this->dataPtr->scene)
    {
      this->dataPtr->scene = rendering::create_scene(worldName, false, true);

      // This usually means rendering is not available
      if (!this->dataPtr->scene)
      {
        gzerr << "Unable to create WideAngleCameraSensor." << std::endl;
        return;
      }
    }

    this->dataPtr->camera = this->dataPtr->scene->CreateWideAngleCamera(
        this->dataPtr->sdf->Get<std::string>("name"), false);

    if (!this->dataPtr->camera)
    {
      gzerr << "Unable to create wide angle camera sensor[mono_camera]"
          << std::endl;
      return;
    }
    this->dataPtr->camera->SetCaptureData(true);

    sdf::ElementPtr cameraSdf = this->dataPtr->sdf->GetElement("camera");
    this->dataPtr->camera->Load(cameraSdf);

    // Do some sanity checks
    if (this->dataPtr->camera->GetImageWidth() == 0 ||
        this->dataPtr->camera->GetImageHeight() == 0)
    {
      gzerr << "image has zero size" << std::endl;
      return;
    }

    this->dataPtr->camera->Init();
    this->dataPtr->camera->CreateRenderTexture(this->Name() + "_RttTex");

    ignition::math::Pose3d cameraPose = this->dataPtr->pose;
    if (cameraSdf->HasElement("pose"))
      cameraPose = cameraSdf->Get<ignition::math::Pose3d>("pose") + cameraPose;

    this->dataPtr->camera->SetWorldPose(cameraPose);
    this->dataPtr->camera->AttachToVisual(this->ParentId(), true);

    if (cameraSdf->HasElement("noise"))
    {
      this->dataPtr->noises[CAMERA_NOISE] =
        NoiseFactory::NewNoiseModel(cameraSdf->GetElement("noise"),
        this->Type());
      this->dataPtr->noises[CAMERA_NOISE]->SetCamera(this->dataPtr->camera);
    }
  }
  else
    gzerr << "No world name" << std::endl;

  // Disable clouds and moon on server side until fixed and also to improve
  // performance
  this->dataPtr->scene->SetSkyXMode(rendering::Scene::GZ_SKYX_ALL &
      ~rendering::Scene::GZ_SKYX_CLOUDS &
      ~rendering::Scene::GZ_SKYX_MOON);

  Sensor::Init();
}

//////////////////////////////////////////////////
void WideAngleCameraSensor::Load(const std::string &_worldName)
{
  Sensor::Load(_worldName);
  this->dataPtr->imagePub = this->dataPtr->node->Advertise<msgs::ImageStamped>(
      this->Topic(), 50);

  std::string lensTopicName = "~/";
  lensTopicName += this->ParentName() + "/" + this->Name() + "/lens/";
  boost::replace_all(lensTopicName, "::", "/");

  sdf::ElementPtr lensSdf =
    this->dataPtr->sdf->GetElement("camera")->GetElement("lens");

  // create a topic that publishes lens states
  this->dataPtr->lensPub = this->dataPtr->node->Advertise<msgs::CameraLens>(
    lensTopicName+"info", 1);

  this->dataPtr->lensSub =
    this->dataPtr->node->Subscribe(lensTopicName + "control",
        &WideAngleCameraSensor::OnCtrlMessage, this);
}

//////////////////////////////////////////////////
void WideAngleCameraSensor::Fini()
{
  this->dataPtr->lensPub.reset();
  this->dataPtr->lensSub.reset();

  CameraSensor::Fini();
}

//////////////////////////////////////////////////
bool WideAngleCameraSensor::UpdateImpl(bool _force)
{
  if (!CameraSensor::UpdateImpl(_force))
    return false;

  if (this->dataPtr->lensPub && this->dataPtr->lensPub->HasConnections())
  {
    std::lock_guard<std::mutex> lock(this->dataPtr->lensCmdMutex);

    for (; !this->dataPtr->hfovCmdQueue.empty();
        this->dataPtr->hfovCmdQueue.pop())
    {
      this->dataPtr->camera->SetHFOV(ignition::math::Angle(
            this->dataPtr->hfovCmdQueue.front()));
    }

    msgs::CameraLens msg;

    rendering::WideAngleCameraPtr wcamera =
      boost::dynamic_pointer_cast<rendering::WideAngleCamera>(
          this->dataPtr->camera);

    const rendering::CameraLens *lens = wcamera->Lens();

    msg.set_type(lens->Type());

    msg.set_c1(lens->C1());
    msg.set_c2(lens->C2());
    msg.set_c3(lens->C3());
    msg.set_f(lens->F());

    msg.set_fun(lens->Fun());
    msg.set_scale_to_hfov(lens->ScaleToHFOV());
    msg.set_cutoff_angle(lens->CutOffAngle());
    msg.set_hfov(wcamera->GetHFOV().Radian());

    msg.set_env_texture_size(wcamera->EnvTextureSize());

    this->dataPtr->lensPub->Publish(msg);
  }

  return true;
}

//////////////////////////////////////////////////
void WideAngleCameraSensor::OnCtrlMessage(ConstCameraLensPtr &_msg)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->lensCmdMutex);

  rendering::WideAngleCameraPtr wcamera =
      boost::dynamic_pointer_cast<rendering::WideAngleCamera>(
          this->dataPtr->camera);

  rendering::CameraLens *lens = (wcamera->Lens());

  if (_msg->has_type())
    lens->SetType(_msg->type());

  if (_msg->has_c1())
    lens->SetC1(_msg->c1());

  if (_msg->has_c2())
    lens->SetC2(_msg->c2());

  if (_msg->has_c3())
    lens->SetC3(_msg->c3());

  if (_msg->has_f())
    lens->SetF(_msg->f());

  if (_msg->has_cutoff_angle())
    lens->SetCutOffAngle(_msg->cutoff_angle());

  if (_msg->has_hfov())
    this->dataPtr->hfovCmdQueue.push(_msg->hfov());

  if (_msg->has_fun())
    lens->SetFun(_msg->fun());

  if (_msg->has_scale_to_hfov())
    lens->SetScaleToHFOV(_msg->scale_to_hfov());
}
