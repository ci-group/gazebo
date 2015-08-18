/*
 * Copyright (C) 2012-2015 Open Source Robotics Foundation
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

// This file causes include error! WHY?
// #include "gazebo/rendering/skyx/include/SkyX.h"

#include "gazebo/rendering/ogre_gazebo.h"
#include "gazebo/rendering/CameraLensPrivate.hh"
#include "gazebo/rendering/WideAngleCameraPrivate.hh"

#include "gazebo/rendering/RTShaderSystem.hh"
#include "gazebo/rendering/Conversions.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/WideAngleCamera.hh"

#include <GL/glew.h>
#include <GL/gl.h>


using namespace gazebo;
using namespace rendering;

CameraLens::CameraLens()
{
  this->dataPtr = new CameraLensPrivate;
}

CameraLens::~CameraLens()
{
  delete this->dataPtr;
}

//////////////////////////////////////////////////
void CameraLens::Init(float _c1, float _c2,
    std::string _fun, float _f, float _c3)
{
  this->dataPtr->c1 = _c1;
  this->dataPtr->c2 = _c2;
  this->dataPtr->c3 = _c3;
  this->dataPtr->f = _f;

  try
  {
    this->dataPtr->fun = CameraLensPrivate::MapFunctionEnum(_fun);
  }
  catch(...)
  {
    std::stringstream sstr;
    sstr << "Failed to create custom mapping with function [" << _fun << "]";

    gzthrow(sstr.str());
  }
}

//////////////////////////////////////////////////
void CameraLens::Init(std::string _name)
{
  this->SetType(_name);
}

//////////////////////////////////////////////////
void CameraLens::Load(sdf::ElementPtr _sdf)
{
  this->sdf = _sdf;

  Load();
}

//////////////////////////////////////////////////
void CameraLens::Load()
{
  if(!this->sdf->HasElement("type"))
    gzthrow("You should specify lens type using <type> element");

  if(this->IsCustom())
  {
    if(this->sdf->HasElement("custom_function"))
    {
      sdf::ElementPtr cf = this->sdf->GetElement("custom_function");

      this->Init(
        cf->Get<double>("c1"),
        cf->Get<double>("c2"),
        cf->Get<std::string>("fun"),
        cf->Get<double>("f"),
        cf->Get<double>("c3"));
    }
    else
      gzthrow("You need a <custom_function> element to use this lens type");
  }
  else
    this->Init(this->GetType());

  this->SetCutOffAngle(this->sdf->Get<double>("cutoff_angle"));
}

//////////////////////////////////////////////////
std::string CameraLens::GetType() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  return this->sdf->Get<std::string>("type");
}

//////////////////////////////////////////////////
bool CameraLens::IsCustom() const
{
  return GetType() == "custom";
}

//////////////////////////////////////////////////
float CameraLens::GetC1() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  return this->dataPtr->c1;
}

//////////////////////////////////////////////////
float CameraLens::GetC2() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  return this->dataPtr->c2;
}

//////////////////////////////////////////////////
float CameraLens::GetC3() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  return this->dataPtr->c3;
}

//////////////////////////////////////////////////
float CameraLens::GetF() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  return this->dataPtr->f;
}

//////////////////////////////////////////////////
std::string CameraLens::GetFun() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  return this->dataPtr->fun.AsString();
}

//////////////////////////////////////////////////
float CameraLens::GetCutOffAngle() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  return this->dataPtr->cutOffAngle;
}

//////////////////////////////////////////////////
bool CameraLens::GetScaleToHFOV() const
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  return this->sdf->Get<bool>("scale_to_hfov");
}

//////////////////////////////////////////////////
void CameraLens::SetType(std::string _type)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  // c1, c2, c3, f, fun
  std::map< std::string, std::tuple<float, float, float,
      float, std::string> > fun_types = {
    {"gnomonical",     std::make_tuple(1.0f, 1.0f, 0.0f, 1.0f, "tan")},
    {"stereographic",  std::make_tuple(2.0f, 2.0f, 0.0f, 1.0f, "tan")},
    {"equidistant",    std::make_tuple(1.0f, 1.0f, 0.0f, 1.0f, "id")},
    {"equisolid_angle",std::make_tuple(2.0f, 2.0f, 0.0f, 1.0f, "sin")},
    {"orthographic",   std::make_tuple(1.0f, 1.0f, 0.0f, 1.0f, "sin")}};

  fun_types.emplace("custom",
      std::make_tuple(this->GetC1(), this->GetC2(), this->GetC3(), this->GetF(),
        CameraLensPrivate::MapFunctionEnum(this->GetFun()).AsString()));

  decltype(fun_types)::mapped_type params;

  try
  {
    params = fun_types.at(_type);
  }
  catch(...)
  {
    std::stringstream sstr;
    sstr << "Unknown mapping function [" << _type << "]";

    gzthrow(sstr.str());
  }

  this->sdf->GetElement("type")->Set(_type);

  if(_type == "custom")
  {
    this->SetC1(std::get<0>(params));
    this->SetC2(std::get<1>(params));
    this->SetC3(std::get<2>(params));
    this->SetF(std::get<3>(params));
    this->SetFun(std::get<4>(params));
  }
  else
  {
    // Do not write values to SDF
    this->dataPtr->c1 = std::get<0>(params);
    this->dataPtr->c2 = std::get<1>(params);
    this->dataPtr->c3 = std::get<2>(params);
    this->dataPtr->f = std::get<3>(params);
    this->dataPtr->fun = CameraLensPrivate::MapFunctionEnum(std::get<4>(params));
  }
}

//////////////////////////////////////////////////
void CameraLens::SetC1(float _c)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  this->dataPtr->c1 = _c;

  if(!this->IsCustom())
    this->ConvertToCustom();

  this->sdf->GetElement("custom_function")->GetElement("c1")->Set((double)_c);
}

//////////////////////////////////////////////////
void CameraLens::SetC2(float _c)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  this->dataPtr->c2 = _c;

  if(!this->IsCustom())
    this->ConvertToCustom();

  this->sdf->GetElement("custom_function")->GetElement("c2")->Set((double)_c);
}

//////////////////////////////////////////////////
void CameraLens::SetC3(float _c)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  this->dataPtr->c3 = _c;

  if(!this->IsCustom())
    this->ConvertToCustom();

  this->sdf->GetElement("custom_function")->GetElement("c3")->Set((double)_c);
}

//////////////////////////////////////////////////
void CameraLens::SetF(float _f)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  this->dataPtr->f = _f;

  if(!this->IsCustom())
    this->ConvertToCustom();

  this->sdf->GetElement("custom_function")->GetElement("f")->Set((double)_f);
}

void CameraLens::SetFun(std::string _fun)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  if(!this->IsCustom())
    this->ConvertToCustom();

  try
  {
    this->dataPtr->fun = CameraLensPrivate::MapFunctionEnum(_fun);
  }
  catch(...)
  {
    std::stringstream sstr;
    sstr << "Failed to create custom mapping with function [" << _fun << "]";

    gzthrow(sstr.str());
  }

  this->sdf->GetElement("custom_function")->GetElement("fun")->Set(_fun);
}

//////////////////////////////////////////////////
void CameraLens::SetCutOffAngle(float _angle)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  this->dataPtr->cutOffAngle = _angle;

  this->sdf->GetElement("cutoff_angle")->Set((double)_angle);
}

//////////////////////////////////////////////////
void CameraLens::SetScaleToHFOV(bool _scale)
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  this->sdf->GetElement("scale_to_hfov")->Set(_scale);
}

//////////////////////////////////////////////////
void CameraLens::SetUniformVariables(Ogre::Pass *_pass,
    float _ratio, float _hfov)
{
  Ogre::GpuProgramParametersSharedPtr uniforms =
    _pass->getFragmentProgramParameters();

  math::Vector3 fun_m[] = {
    math::Vector3(1, 0, 0),
    math::Vector3(0, 1, 0),
    math::Vector3(0, 0, 1)
  };

  uniforms->setNamedConstant("c1", static_cast<Ogre::Real>(this->dataPtr->c1));
  uniforms->setNamedConstant("c2", static_cast<Ogre::Real>(this->dataPtr->c2));
  uniforms->setNamedConstant("c3", static_cast<Ogre::Real>(this->dataPtr->c3));

  if(this->GetScaleToHFOV())
  {
    float param = (_hfov/2)/this->dataPtr->c2+this->dataPtr->c3;
    float fun_res = this->dataPtr->fun.Apply(param);

    float new_f = 1.0f/(this->dataPtr->c1*fun_res);

    uniforms->setNamedConstant("f", static_cast<Ogre::Real>(new_f));
  }
  else
    uniforms->setNamedConstant("f", static_cast<Ogre::Real>(this->dataPtr->f));

  auto vec_fun = this->dataPtr->fun.AsVector3();

  uniforms->setNamedConstant("fun", Ogre::Vector3(
      vec_fun.x, vec_fun.y, vec_fun.z));

  uniforms->setNamedConstant("cutOffAngle",
    static_cast<Ogre::Real>(this->dataPtr->cutOffAngle));

  Ogre::GpuProgramParametersSharedPtr uniforms_vs =
    _pass->getVertexProgramParameters();

  uniforms_vs->setNamedConstant("ratio", static_cast<Ogre::Real>(_ratio));
}

//////////////////////////////////////////////////
void CameraLens::ConvertToCustom()
{
  std::lock_guard<std::recursive_mutex> lock(this->dataPtr->dataMutex);

  this->SetType("custom");

  sdf::ElementPtr cf = this->sdf->AddElement("custom_function");

  cf->AddElement("c1")->Set((double)this->dataPtr->c1);
  cf->AddElement("c2")->Set((double)this->dataPtr->c2);
  cf->AddElement("c3")->Set((double)this->dataPtr->c3);
  cf->AddElement("f")->Set((double)this->dataPtr->f);

  cf->AddElement("fun")->Set(this->dataPtr->fun.AsString());
}

//////////////////////////////////////////////////
WideAngleCamera::WideAngleCamera(const std::string &_namePrefix,
    ScenePtr _scene, bool _autoRender, int _textureSize):
  Camera(_namePrefix, _scene, _autoRender),
  envTextureSize(_textureSize)
{
  this->dataPtr = new WideAngleCameraPrivate;
  this->lens = new CameraLens();

  envCubeMapTexture = NULL;
  for(int i=0;i<6;i++)
  {
    envCameras[i] = NULL;
    envRenderTargets[i] = NULL;
  }
}

//////////////////////////////////////////////////
WideAngleCamera::~WideAngleCamera()
{
  delete this->dataPtr;
  delete this->lens;
}

//////////////////////////////////////////////////
void WideAngleCamera::Init()
{
  Camera::Init();

  this->CreateEnvRenderTexture(this->scopedUniqueName + "_envRttTex");
}

//////////////////////////////////////////////////
void WideAngleCamera::Load()
{
  Camera::Load();

  this->CreateEnvCameras();

  if(this->sdf->HasElement("lens"))
  {
    sdf::ElementPtr sdf_lens = this->sdf->GetElement("lens");

    this->lens->Load(sdf_lens);

    if(sdf_lens->HasElement("env_texture_size"))
      this->envTextureSize = sdf_lens->Get<int>("env_texture_size");
  }
  else
    this->lens->Load();
}

//////////////////////////////////////////////////
void WideAngleCamera::Fini()
{
  for(int i=0;i<6;i++)
  {
    RTShaderSystem::DetachViewport(this->envViewports[i], this->GetScene());

    this->envRenderTargets[i]->removeAllViewports();
    this->envRenderTargets[i] = NULL;

    this->GetScene()->GetManager()->destroyCamera(envCameras[i]->getName());
    envCameras[i] = NULL;
  }

  if(this->envCubeMapTexture)
    Ogre::TextureManager::getSingleton().remove(this->envCubeMapTexture->getName());
  this->envCubeMapTexture = NULL;

  Camera::Fini();
}

//////////////////////////////////////////////////
int WideAngleCamera::GetEnvTextureSize() const
{
  std::lock_guard<std::mutex> lock(this->dataPtr->dataMutex);

  return this->envTextureSize;
}

//////////////////////////////////////////////////
CameraLens *WideAngleCamera::GetLens()
{
  return this->lens;
}

//////////////////////////////////////////////////
void WideAngleCamera::SetRenderTarget(Ogre::RenderTarget *_target)
{
  Camera::SetRenderTarget(_target);

  if(this->renderTarget)
  {
    this->cubeMapCompInstance =
      Ogre::CompositorManager::getSingleton().addCompositor(this->viewport,
          "WideCameraLensMap/ParametrisedMap");

    if(this->envCubeMapTexture)
    {
      this->compMat =
          Ogre::MaterialManager::getSingleton().getByName("Gazebo/WideLensMap");

      if(!this->compMat->getTechnique(0)->getPass(0)->getNumTextureUnitStates())
        this->compMat->getTechnique(0)->getPass(0)->createTextureUnitState();

      this->cubeMapCompInstance->addListener(this);
    }
    else
      gzerr << "Compositor texture MISSING";


    this->cubeMapCompInstance->setEnabled(true);
  }
}

//////////////////////////////////////////////////
void WideAngleCamera::SetEnvTextureSize(int _size)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->dataMutex);

  if(this->sdf->HasElement("env_texture_size"))
    this->sdf->AddElement("env_texture_size")->Set(_size);

  this->sdf->GetElement("env_texture_size")->Set(_size);
}

//////////////////////////////////////////////////
void WideAngleCamera::CreateEnvCameras()
{
  for(int i=0;i<6;i++)
  {
    std::stringstream name_str;

    name_str << this->scopedUniqueName << "_env_" << i;

    envCameras[i] =
        this->GetScene()->GetManager()->createCamera(name_str.str());

    envCameras[i]->setFixedYawAxis(false);
    envCameras[i]->setFOVy(Ogre::Degree(90));
    envCameras[i]->setAspectRatio(1);

    envCameras[i]->setNearClipDistance(0.01);
    envCameras[i]->setFarClipDistance(1000);
  }
}

//////////////////////////////////////////////////
void WideAngleCamera::SetClipDist()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->dataMutex);

  sdf::ElementPtr clipElem = this->sdf->GetElement("clip");
  if (!clipElem)
    gzthrow("Camera has no <clip> tag.");

  for(int i=0;i<6;i++)
  {
    if(this->envCameras[i])
    {
      this->envCameras[i]->setNearClipDistance(clipElem->Get<double>("near"));
      this->envCameras[i]->setFarClipDistance(clipElem->Get<double>("far"));
      this->envCameras[i]->setRenderingDistance(clipElem->Get<double>("far"));
    }
    else
    {
      gzerr << "Setting clip distances failed -- no camera yet\n";
      break;
    }
  }
}

//////////////////////////////////////////////////
void WideAngleCamera::CreateEnvRenderTexture(const std::string &_textureName)
{
  int fsaa = 4;

#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR < 8
  fsaa = 0;
#endif

  this->envCubeMapTexture = Ogre::TextureManager::getSingleton().createManual(
      this->scopedUniqueName+"::"+_textureName,
      "General",
      Ogre::TEX_TYPE_CUBE_MAP,
      this->envTextureSize,
      this->envTextureSize,
      0,
      Ogre::PF_A8R8G8B8,
      Ogre::TU_RENDERTARGET,
      0,
      false,
      fsaa).getPointer();

  for(int i=0;i<6;i++)
  {
    Ogre::RenderTarget *rtt;
    rtt = this->envCubeMapTexture->getBuffer(i)->getRenderTarget();

    Ogre::Viewport *vp = rtt->addViewport(this->envCameras[i]);
    vp->setClearEveryFrame(true);
    vp->setShadowsEnabled(true);
    vp->setOverlaysEnabled(false);

    RTShaderSystem::AttachViewport(vp, this->GetScene());

    vp->setBackgroundColour(
      Conversions::Convert(this->scene->GetBackgroundColor()));
    vp->setVisibilityMask(GZ_VISIBILITY_ALL &
        ~(GZ_VISIBILITY_GUI | GZ_VISIBILITY_SELECTABLE));

    this->envViewports[i] = vp;

    //FIXME: problem with Skyx include
    // if (this->GetScene()->GetSkyX())
    //   rtt->addListener(this->GetScene()->GetSkyX());

    this->envRenderTargets[i] = rtt;
  }
}

//////////////////////////////////////////////////
void WideAngleCamera::RenderImpl()
{
  // std::lock_guard<std::mutex> lock(this->dataPtr->renderMutex);

  math::Quaternion orient = this->GetWorldRotation();
  math::Vector3 pos = this->GetWorldPosition();

  for(int i=0;i<6;i++)
  {
    this->envCameras[i]->setPosition(this->camera->getRealPosition());
    this->envCameras[i]->setOrientation(this->camera->getRealOrientation());
  }

  this->envCameras[0]->rotate(this->camera->getDerivedUp(), Ogre::Degree(-90));
  this->envCameras[1]->rotate(this->camera->getDerivedUp(), Ogre::Degree(90));
  this->envCameras[2]->pitch(Ogre::Degree(90));
  this->envCameras[3]->pitch(Ogre::Degree(-90));
  this->envCameras[5]->rotate(this->camera->getDerivedUp(), Ogre::Degree(180));

  for(int i=0;i<6;i++)
    this->envRenderTargets[i]->update();

  compMat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(
    this->envCubeMapTexture->getName());

  this->renderTarget->update();
}

//////////////////////////////////////////////////
void WideAngleCamera::notifyMaterialRender(Ogre::uint32 /*_pass_id*/,
    Ogre::MaterialPtr &_material)
{
  if(_material.isNull())
    return;

  Ogre::Technique *pTechnique = _material->getBestTechnique();
  if(!pTechnique)
    return;

  Ogre::Pass *pPass = pTechnique->getPass(0);
  if(!pPass || !pPass->hasFragmentProgram())
    return;

  if(!this->GetLens())
  {
    gzerr << "No lens\n";
    return;
  }

  this->GetLens()->SetUniformVariables(pPass,
    this->GetAspectRatio(),
    this->GetLens()->GetCutOffAngle()*2);

  //XXX: OGRE does not allow to enable cubemap filtering extention thru it's API,
  // suppose that this function was invoked in a thread that has OpenGL context
  glEnable(GL_ARB_seamless_cube_map);
  // Some drivers do not support this extention
}
