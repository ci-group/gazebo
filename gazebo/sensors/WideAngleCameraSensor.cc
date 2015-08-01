

#include "WideAngleCameraSensor.hh"

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


using namespace gazebo;
using namespace sensors;

GZ_REGISTER_STATIC_SENSOR("wideanglecamera", WideAngleCameraSensor)

WideAngleCameraSensor::WideAngleCameraSensor()
{
}

void WideAngleCameraSensor::Init()
{
  if (rendering::RenderEngine::Instance()->GetRenderPathType() ==
      rendering::RenderEngine::NONE)
  {
    gzerr << "Unable to create WideAngleCameraSensor. Rendering is disabled.\n";
    return;
  }

  std::string worldName = this->world->GetName();

  if (!worldName.empty())
  {
    this->scene = rendering::get_scene(worldName);
    if (!this->scene)
    {
      this->scene = rendering::create_scene(worldName, false, true);

      // This usually means rendering is not available
      if (!this->scene)
      {
        gzerr << "Unable to create WideAngleCameraSensor.\n";
        return;
      }
    }

    this->camera = this->scene->CreateWideAngleCamera(
        this->sdf->Get<std::string>("name"), false);

    if (!this->camera)
    {
      gzerr << "Unable to create wide angle camera sensor[mono_camera]\n";
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
    this->camera->CreateRenderTexture(this->GetName() + "_RttTex");

    math::Pose cameraPose = this->pose;
    if (cameraSdf->HasElement("pose"))
      cameraPose = cameraSdf->Get<math::Pose>("pose") + cameraPose;

    this->camera->SetWorldPose(cameraPose);
    this->camera->AttachToVisual(this->parentId, true);

    if (cameraSdf->HasElement("noise"))
    {
      this->noises[CAMERA_NOISE] =
        NoiseFactory::NewNoiseModel(cameraSdf->GetElement("noise"),
        this->GetType());
      this->noises[CAMERA_NOISE]->SetCamera(this->camera);
    }
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

void WideAngleCameraSensor::Load(const std::string &_worldName)
{
  gzerr << "wideanglecamera::Load" << std::endl;
  Sensor::Load(_worldName);
  this->imagePub = this->node->Advertise<msgs::ImageStamped>(
      this->GetTopic(), 50);

  std::string lensTopicName = "~/";
  lensTopicName += this->parentName + "/" + this->GetName() + "/lens_";
  boost::replace_all(lensTopicName, "::", "/");

  sdf::ElementPtr lensSdf = this->sdf->GetElement("camera")->GetElement("lens");
  if(lensSdf->HasElement("advertise") && lensSdf->Get<bool>("advertise"))
    this->lensPub = this->node->Advertise<msgs::CameraLensCmd>(
      lensTopicName+"info", 50);

  this->lensSub = this->node->Subscribe(lensTopicName+"control",
      &WideAngleCameraSensor::OnCtrlMessage,this);
}

void WideAngleCameraSensor::Fini()
{
  this->lensPub.reset();
  this->lensSub.reset();

  CameraSensor::Fini();
}

bool WideAngleCameraSensor::UpdateImpl(bool _force)
{
  if(!CameraSensor::UpdateImpl(_force))
    return false;

  if(this->lensPub && this->lensPub->HasConnections())
  {
    msgs::CameraLensCmd msg;

    rendering::WideAngleCameraPtr wcamera = 
      boost::dynamic_pointer_cast<rendering::WideAngleCamera>(this->camera);

    const rendering::CameraLens *lens = wcamera->GetLens();

    msg.set_name(this->GetName());
    msg.set_destiny(msgs::CameraLensCmd_CmdDestiny_INFO);
    msg.set_type(lens->GetType());

    msg.set_c1(lens->GetC1());
    msg.set_c2(lens->GetC2());
    msg.set_c3(lens->GetC3());
    msg.set_f(lens->GetF());

    msg.set_fun(lens->GetFun());
    msg.set_circular(lens->IsCircular());
    msg.set_cutoff_angle(lens->GetCutOffAngle());

    msg.set_env_texture_size(wcamera->GetEnvTextureSize());

    this->lensPub->Publish(msg);
  }

  return true;
}

void WideAngleCameraSensor::OnCtrlMessage(ConstCameraLensCmdPtr &_msg)
{
  if(_msg->destiny() != msgs::CameraLensCmd_CmdDestiny_SET);
    return;

  rendering::WideAngleCameraPtr wcamera =
      boost::dynamic_pointer_cast<rendering::WideAngleCamera>(this->camera);

  rendering::CameraLens *lens =
      const_cast<rendering::CameraLens*>(wcamera->GetLens());

  if(_msg->has_type())
    lens->SetType(_msg->type());

  if(_msg->has_c1())
    lens->SetC1(_msg->c1());

  if(_msg->has_c2())
    lens->SetC2(_msg->c2());

  if(_msg->has_c3())
    lens->SetC3(_msg->c3());

  if(_msg->has_f())
    lens->SetF(_msg->f());

  if(_msg->has_cutoff_angle())
    lens->SetCutOffAngle(_msg->cutoff_angle());

  if(_msg->has_fun())
    lens->SetFun(_msg->fun());

  if(_msg->has_circular())
    lens->SetCircular(_msg->circular());
}