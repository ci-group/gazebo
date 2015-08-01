

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
  gzerr << "wideanglecamera::__init__" << std::endl;
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

  std::string projTopicName = "~/";
  projTopicName += this->parentName + "/" + this->GetName() + "/lens_";
  boost::replace_all(projTopicName, "::", "/");

  sdf::ElementPtr projSdf = this->sdf->GetElement("camera")->GetElement("lens");
  if(projSdf->HasElement("advertise") && projSdf->Get<bool>("advertise"))
    this->projPub = this->node->Advertise<msgs::CameraProjectionCmd>(
      projTopicName+"info", 50);

  this->node->Subscribe(projTopicName+"control",&WideAngleCameraSensor::OnCtrlMessage,this);
}

bool WideAngleCameraSensor::UpdateImpl(bool _force)
{
  if(!CameraSensor::UpdateImpl(_force))
    return false;

  if(this->projPub && this->projPub->HasConnections())
  {
    msgs::CameraProjectionCmd msg;

    rendering::WideAngleCameraPtr wcamera = 
      boost::dynamic_pointer_cast<rendering::WideAngleCamera>(this->camera);

    const rendering::CameraProjection *proj = wcamera->GetProjection();

    msg.set_name(this->GetName());
    msg.set_destiny(msgs::CameraProjectionCmd_CmdDestiny_INFO);
    msg.set_type(proj->GetType());

    msg.set_c1(proj->GetC1());
    msg.set_c2(proj->GetC2());
    msg.set_c3(proj->GetC3());
    msg.set_f(proj->GetF());

    msg.set_fun(proj->GetFun());
    msg.set_circular(proj->IsCircular());
    msg.set_cutoff_angle(proj->GetCutOffAngle());

    msg.set_env_texture_size(wcamera->GetEnvTextureSize());

    this->projPub->Publish(msg);
  }

  return true;
}

void WideAngleCameraSensor::OnCtrlMessage(ConstCameraProjectionCmdPtr &_msg)
{
  if(_msg->destiny() != msgs::CameraProjectionCmd_CmdDestiny_SET);
    return;

  rendering::WideAngleCameraPtr wcamera =
      boost::dynamic_pointer_cast<rendering::WideAngleCamera>(this->camera);

  rendering::CameraProjection *proj =
      const_cast<rendering::CameraProjection*>(wcamera->GetProjection());

  if(_msg->has_type())
    proj->SetType(_msg->type());

  if(_msg->has_c1())
    proj->SetC1(_msg->c1());

  if(_msg->has_c2())
    proj->SetC2(_msg->c2());

  if(_msg->has_c3())
    proj->SetC3(_msg->c3());

  if(_msg->has_f())
    proj->SetF(_msg->f());

  if(_msg->has_cutoff_angle())
    proj->SetCutOffAngle(_msg->cutoff_angle());

  if(_msg->has_fun())
    proj->SetFun(_msg->fun());

  if(_msg->has_circular())
    proj->SetCircular(_msg->circular());
}