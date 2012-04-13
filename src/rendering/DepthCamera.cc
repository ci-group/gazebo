/*
 * Copyright 2011 Nate Koenig & Andrew Howard
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

#include <dirent.h>
#include <sstream>

#include "sdf/sdf.h"
#include "rendering/ogre.h"
#include "rendering/RTShaderSystem.hh"

#include "common/Events.hh"
#include "common/Console.hh"
#include "common/Exception.hh"
#include "math/Pose.hh"

#include "rendering/Visual.hh"
#include "rendering/Conversions.hh"
#include "rendering/Scene.hh"
#include "rendering/DepthCamera.hh"

using namespace gazebo;
using namespace rendering;


//////////////////////////////////////////////////
DepthCamera::DepthCamera(const std::string &_namePrefix, Scene *_scene,
                         bool _autoRender)
: Camera(_namePrefix, _scene, _autoRender)
{
  this->depthTarget = NULL;
  this->depthBuffer = NULL;
  this->depthMaterial = NULL;
  this->pcdTarget = NULL;
  this->pcdBuffer = NULL;
  this->pcdMaterial = NULL;
  this->output_points = false;
}

//////////////////////////////////////////////////
DepthCamera::~DepthCamera()
{
}

//////////////////////////////////////////////////
void DepthCamera::Load(sdf::ElementPtr &_sdf)
{
  Camera::Load(_sdf);
  if (_sdf->GetElement("depth_camera") &&
      _sdf->GetElement("depth_camera")->GetValueString("output") == "points")
    this->output_points = true;
}

//////////////////////////////////////////////////
void DepthCamera::Load()
{
  Camera::Load();
}

//////////////////////////////////////////////////
void DepthCamera::Init()
{
  Camera::Init();
}

//////////////////////////////////////////////////
void DepthCamera::Fini()
{
  Camera::Fini();
}

//////////////////////////////////////////////////
void DepthCamera::CreateDepthTexture(const std::string &_textureName)
{
  // Create the depth buffer
  std::string depthMaterialName = this->GetName() + "_RttMat_Camera_Depth";

  this->depthTexture = Ogre::TextureManager::getSingleton().createManual(
      _textureName,
      Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
      Ogre::TEX_TYPE_2D,
      this->GetImageWidth(), this->GetImageHeight(), 0,
      Ogre::PF_FLOAT32_R,
      Ogre::TU_RENDERTARGET).getPointer();

  this->depthTarget = this->depthTexture->getBuffer()->getRenderTarget();
  this->depthTarget->setAutoUpdated(false);

  this->SetDepthTarget(this->depthTarget);

  this->depthViewport->setOverlaysEnabled(false);
  this->depthViewport->setBackgroundColour(
      Ogre::ColourValue(Ogre::ColourValue(0, 0, 0)));

  // Create materials for all the render textures.
  Ogre::MaterialPtr matPtr = Ogre::MaterialManager::getSingleton().create(
      depthMaterialName,
      Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

  matPtr->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
  matPtr->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
  matPtr->getTechnique(0)->getPass(0)->setLightingEnabled(false);

  matPtr->getTechnique(0)->getPass(0)->createTextureUnitState(
      _textureName);

  this->depthMaterial = (Ogre::Material*)(
      Ogre::MaterialManager::getSingleton().getByName("Gazebo/DepthMap").get());

  this->depthMaterial->load();

  if (this->output_points)
  {
    this->pcdTexture = Ogre::TextureManager::getSingleton().createManual(
        _textureName + "_pcd",
        "General",
        Ogre::TEX_TYPE_2D,
        this->GetImageWidth(), this->GetImageHeight(), 0,
        Ogre::PF_FLOAT32_RGBA,
        Ogre::TU_RENDERTARGET).getPointer();

    this->pcdTarget = this->pcdTexture->getBuffer()->getRenderTarget();
    this->pcdTarget->setAutoUpdated(false);

    this->pcdViewport = this->pcdTarget->addViewport(this->camera);
    this->pcdViewport->setClearEveryFrame(true);
    this->pcdViewport->setBackgroundColour(
        Conversions::Convert(this->scene->GetBackgroundColor()));
    this->pcdViewport->setOverlaysEnabled(false);
    this->pcdViewport->setVisibilityMask(
        GZ_VISIBILITY_ALL & ~GZ_VISIBILITY_GUI);

    this->pcdMaterial = (Ogre::Material*)(
    Ogre::MaterialManager::getSingleton().getByName("Gazebo/XYZPoints").get());

    this->pcdMaterial->getTechnique(0)->getPass(0)->createTextureUnitState(
              this->renderTexture->getName());

    this->pcdMaterial->load();
  }

/*
  // Create a custom render queue invocation sequence for the depth
  // render texture
  Ogre::RenderQueueInvocationSequence* invocationSequence =
    Ogre::Root::getSingleton().createRenderQueueInvocationSequence(_textureName
    + "_DepthMap");

  // Add a render queue invocation to the sequence, and disable shadows for it
  Ogre::RenderQueueInvocation* invocation =
    invocationSequence->add(Ogre::RENDER_QUEUE_MAIN, _textureName + "_main");
  invocation->setSuppressShadows(true);

  // Set the render queue invocation sequence for the depth render texture
  // viewport
  this->depthViewport->setRenderQueueInvocationSequenceName(
  _textureName + "_DepthMap");
*/
}

//////////////////////////////////////////////////
void DepthCamera::PostRender()
{
  this->depthTarget->swapBuffers();

  if (this->newData && this->captureData)
  {
    Ogre::HardwarePixelBufferSharedPtr pixelBuffer;

    unsigned int width = this->GetImageWidth();
    unsigned int height = this->GetImageHeight();

    // Get access to the buffer and make an image and write it to file
    pixelBuffer = this->depthTexture->getBuffer();

    Ogre::PixelFormat format = pixelBuffer->getFormat();

    // Blit the depth buffer if needed
    if (!this->depthBuffer)
      this->depthBuffer = new float[width*height];

    Ogre::Box src_box(0, 0, width, height);
    Ogre::PixelBox dst_box(width, height,
        1, format, this->depthBuffer);

    pixelBuffer->lock(Ogre::HardwarePixelBuffer::HBL_NORMAL);
    pixelBuffer->blitToMemory(src_box, dst_box);
    pixelBuffer->unlock();  // FIXME: do we need to lock/unlock still?

    this->newDepthFrame(this->depthBuffer, width, height, 1, "FLOAT32");

    if (this->output_points)
    {
      this->pcdTarget->swapBuffers();
      Ogre::HardwarePixelBufferSharedPtr pcdPixelBuffer;

      // Get access to the buffer and make an image and write it to file
      pcdPixelBuffer = this->pcdTexture->getBuffer();

      Ogre::PixelFormat pcdFormat = pcdPixelBuffer->getFormat();

      size_t size = Ogre::PixelUtil::getMemorySize(width, height, 1, pcdFormat);

      // Blit the depth buffer if needed
      if (!this->pcdBuffer)
        this->pcdBuffer = new float[size];

      memset(this->pcdBuffer, 128, size);

      Ogre::Box pcd_src_box(0, 0, width, height);
      Ogre::PixelBox pcd_dst_box(width, height,
          1, Ogre::PF_FLOAT32_RGBA, this->pcdBuffer);

      pcdPixelBuffer->blitToMemory(pcd_src_box, pcd_dst_box);

      this->newRGBPointCloud(this->pcdBuffer, width, height, 1, "RGBPOINTS");
    }
  }

  // also new image frame for camera texture
  Camera::PostRender();

  this->newData = false;
}

void DepthCamera::UpdateRenderTarget(Ogre::RenderTarget *target,
          Ogre::Material *material, std::string matName)
{
  Ogre::RenderSystem *renderSys;
  Ogre::Viewport *vp = NULL;
  Ogre::SceneManager *sceneMgr = this->scene->GetManager();
  Ogre::Pass *pass;

  renderSys = this->scene->GetManager()->getDestinationRenderSystem();
  // Get pointer to the material pass
  pass = material->getBestTechnique()->getPass(0);

  // Render the depth texture
  // OgreSceneManager::_render function automatically sets farClip to 0.
  // Which normally equates to infinite distance. We don't want this. So
  // we have to set the distance every time.
  this->GetOgreCamera()->setFarClipDistance(this->GetFarClip());

  Ogre::AutoParamDataSource autoParamDataSource;

  vp = target->getViewport(0);

  // return 0 in case no renderable object is inside frustrum
  vp->setBackgroundColour(Ogre::ColourValue(Ogre::ColourValue(0, 0, 0)));

  Ogre::CompositorManager::getSingleton().setCompositorEnabled(
                                                vp, matName, true);

  // Need this line to render the ground plane. No idea why it's necessary.
  renderSys->_setViewport(vp);
  sceneMgr->_setPass(pass, true, false);
  autoParamDataSource.setCurrentPass(pass);
  autoParamDataSource.setCurrentViewport(vp);
  autoParamDataSource.setCurrentRenderTarget(target);
  autoParamDataSource.setCurrentSceneManager(sceneMgr);
  autoParamDataSource.setCurrentCamera(this->GetOgreCamera(), true);

  renderSys->setLightingEnabled(false);
  renderSys->_setFog(Ogre::FOG_NONE);

  // These two lines don't seem to do anything useful
  renderSys->_setProjectionMatrix(
      this->GetOgreCamera()->getProjectionMatrixRS());
  renderSys->_setViewMatrix(this->GetOgreCamera()->getViewMatrix(true));

#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR == 6
  pass->_updateAutoParamsNoLights(&autoParamDataSource);
#else
  pass->_updateAutoParams(&autoParamDataSource, 1);
#endif

  // NOTE: We MUST bind parameters AFTER updating the autos
  if (pass->hasVertexProgram())
  {
    renderSys->bindGpuProgram(
    pass->getVertexProgram()->_getBindingDelegate());

#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR == 6
    renderSys->bindGpuProgramParameters(Ogre::GPT_VERTEX_PROGRAM,
    pass->getVertexProgramParameters());
#else
    renderSys->bindGpuProgramParameters(Ogre::GPT_VERTEX_PROGRAM,
      pass->getVertexProgramParameters(), 1);
#endif
  }

  if (pass->hasFragmentProgram())
  {
    renderSys->bindGpuProgram(
    pass->getFragmentProgram()->_getBindingDelegate());

#if OGRE_VERSION_MAJOR == 1 && OGRE_VERSION_MINOR == 6
    renderSys->bindGpuProgramParameters(Ogre::GPT_FRAGMENT_PROGRAM,
    pass->getFragmentProgramParameters());
#else
      renderSys->bindGpuProgramParameters(Ogre::GPT_FRAGMENT_PROGRAM,
      pass->getFragmentProgramParameters(), 1);
#endif
  }
}

//////////////////////////////////////////////////
void DepthCamera::RenderImpl()
{
  Ogre::SceneManager *sceneMgr = this->scene->GetManager();

  sceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_NONE);
  sceneMgr->_suppressRenderStateChanges(true);

  this->UpdateRenderTarget(this->depthTarget,
                  this->depthMaterial, "Gazebo/DepthMap");

  // Does actual rendering
  this->depthTarget->update(false);

  sceneMgr->_suppressRenderStateChanges(false);
  sceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED);

  // for camera image
  Camera::RenderImpl();

  if (this->output_points)
  {
    sceneMgr->setShadowTechnique(Ogre::SHADOWTYPE_NONE);
    sceneMgr->_suppressRenderStateChanges(true);

    this->UpdateRenderTarget(this->pcdTarget,
                  this->pcdMaterial, "Gazebo/XYZPoints");

    this->pcdTarget->update(false);

    sceneMgr->_suppressRenderStateChanges(false);
    sceneMgr->setShadowTechnique(
            Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED);
  }
}

//////////////////////////////////////////////////
const float* DepthCamera::GetDepthData()
{
  return this->depthBuffer;
}

//////////////////////////////////////////////////
void DepthCamera::SetDepthTarget(Ogre::RenderTarget *target)
{
  this->depthTarget = target;

  if (this->depthTarget)
  {
    // Setup the viewport to use the texture
    this->depthViewport = this->depthTarget->addViewport(this->camera);
    this->depthViewport->setClearEveryFrame(true);
    this->depthViewport->setBackgroundColour(
        Conversions::Convert(this->scene->GetBackgroundColor()));
    this->depthViewport->setVisibilityMask(
        GZ_VISIBILITY_ALL & ~GZ_VISIBILITY_GUI);

    double ratio = static_cast<double>(this->depthViewport->getActualWidth()) /
                   static_cast<double>(this->depthViewport->getActualHeight());

    double hfov = this->GetHFOV().GetAsRadian();
    double vfov = 2.0 * atan(tan(hfov / 2.0) / ratio);
    // gzerr << "debug " << hfov << " " << vfov << " " << ratio << "\n";
    this->camera->setAspectRatio(ratio);
    this->camera->setFOVy(Ogre::Radian(vfov));
  }
}




