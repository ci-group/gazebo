/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * Desc: Projector
 * Author: Jared Duke, John Hsu, Nate Koenig
 */

#include "rendering/RTShaderSystem.hh"

#include "rendering/Conversions.hh"
#include "rendering/Visual.hh"
#include "rendering/Scene.hh"
#include "rendering/Projector.hh"

using namespace gazebo;
using namespace rendering;

typedef std::map<std::string, Ogre::Pass*> OgrePassMap;
typedef OgrePassMap::iterator OgrePassMapIterator;

/////////////////////////////////////////////////
Projector::Projector(rendering::VisualPtr _parent)
{
  this->visual = _parent;
}

/////////////////////////////////////////////////
Projector::~Projector()
{
  // Ogre cleanup
  Ogre::Root::getSingletonPtr()->removeFrameListener(&this->projector);
}

/////////////////////////////////////////////////
void Projector::Load(const math::Pose &_pose,
                     const std::string &_textureName,
                     double _nearClip,
                     double _farClip,
                     double _fov)
{
  if (!this->visual)
  {
    gzerr << "Projector does not have a valid parent visual\n";
    return;
  }

  int retryCount = 0;

  // Initialize the projector
  while (!this->projector.initialized && retryCount < 10)
  {
    // init
    this->projector.Init(this->visual, _textureName, _nearClip, _farClip, _fov);

    // set the projector pose relative to body
    this->projector.SetPose(_pose);

    // Add the projector as an Ogre frame listener
    Ogre::Root::getSingletonPtr()->addFrameListener(&this->projector);

    if (!this->projector.initialized)
    {
      gzwarn << "starting projector failed, retrying in 1 sec.\n";
      common::Time::MSleep(1000);
      ++retryCount;
    }
  }
/*
  // Start projector
  this->add_model_event_ = gazebo::event::Events::ConnectWorldUpdateStart(
    boost::bind(&Projector::ToggleProjector, this, true));
    */
}

/////////////////////////////////////////////////
void Projector::Load(sdf::ElementPtr _sdf)
{
  math::Pose pose;
  std::string textureName;
  double nearClip = 0.1;
  double farClip = 10.0;
  double fov = M_PI * 0.25;

  if (_sdf->HasElement("pose"))
    pose = _sdf->GetValuePose("pose");

  if (_sdf->HasElement("texture_name"))
    textureName = _sdf->GetValueString("texture_name");

  if (_sdf->HasElement("near_clip"))
    nearClip = _sdf->GetValueDouble("near_clip");

  if (_sdf->HasElement("far_clip"))
    farClip = _sdf->GetValueDouble("far_clip");

  if (_sdf->HasElement("fov"))
    fov = _sdf->GetValueDouble("fov");

  this->Load(pose, textureName, nearClip, farClip, fov);
}

/////////////////////////////////////////////////
void Projector::Load(const msgs::Projector &_msg)
{
  math::Pose pose;
  std::string textureName;
  double nearClip = 0.1;
  double farClip = 10.0;
  double fov = M_PI * 0.25;

  if (_msg.has_pose())
    pose = msgs::Convert(_msg.pose());

  if (_msg.has_texture())
    textureName = _msg.texture();

  if (_msg.has_near_clip())
    nearClip = _msg.near_clip();

  if (_msg.has_far_clip())
    farClip = _msg.far_clip();

  if (_msg.has_fov())
    fov = _msg.fov();

  this->Load(pose, textureName, nearClip, farClip, fov);
}

/////////////////////////////////////////////////
void Projector::SetTexture(const std::string &_textureName)
{
  this->projector.SetTexture(_textureName);
}

/////////////////////////////////////////////////
void Projector::Toggle()
{
  // if not headless
  if (this->projector.initialized)
  {
    this->projector.SetEnabled(!this->projector.enabled);
    rendering::RTShaderSystem::Instance()->UpdateShaders();
  }
  else
    gzwarn << "could not start projector, toggle failed\n";
}

/////////////////////////////////////////////////
Projector::ProjectorFrameListener::ProjectorFrameListener()
{
  this->enabled = false;
  this->initialized = false;
  this->usingShaders = false;

  this->node = NULL;
  this->filterNode = NULL;
  this->projectorQuery = NULL;
  this->frustum = NULL;
  this->filterFrustum = NULL;

  this->nodeName = "Projector";
  this->filterNodeName = "ProjectorFilter";
}

/////////////////////////////////////////////////
Projector::ProjectorFrameListener::~ProjectorFrameListener()
{
  this->RemovePassFromMaterials();

  if (this->node)
  {
    this->node->detachObject(this->frustum);
    this->visual->GetSceneNode()->removeAndDestroyChild(this->nodeName);
    this->node = NULL;
  }

  if (this->filterNode)
  {
    this->filterNode->detachObject(this->filterFrustum);
    this->visual->GetSceneNode()->removeAndDestroyChild(this->filterNodeName);
    this->filterNode = NULL;
  }

  delete this->frustum;
  delete this->filterFrustum;

  if (this->projectorQuery)
    this->sceneMgr->destroyQuery(this->projectorQuery);

  this->visual.reset();
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::Init(VisualPtr _visual,
                                             const std::string &_textureName,
                                             double _near,
                                             double _far,
                                             double _fov)
{
  if (this->initialized)
    return;

  if (_textureName.empty())
  {
    gzerr << "Projector is missing a texture\n";
    return;
  }

  this->visual = _visual;

  this->nodeName = this->visual->GetName() + "_Projector";
  this->filterNodeName = this->visual->GetName() + "_ProjectorFilter";

  this->frustum = new Ogre::Frustum();
  this->filterFrustum = new Ogre::Frustum();
  this->filterFrustum->setProjectionType(Ogre::PT_ORTHOGRAPHIC);

  this->sceneMgr = this->visual->GetScene()->GetManager();
  this->projectorQuery = this->sceneMgr->createPlaneBoundedVolumeQuery(
      Ogre::PlaneBoundedVolumeList());

  this->SetSceneNode();
  this->SetTexture(_textureName);
  this->SetFrustumClipDistance(_near, _far);
  this->SetFrustumFOV(_fov);

  this->initialized = true;
}

/////////////////////////////////////////////////
bool Projector::ProjectorFrameListener::frameStarted(
    const Ogre::FrameEvent &/*_evt*/)
{
  if (!this->initialized || !this->enabled || this->textureName.empty())
    return true;

  this->AddPassToVisibleMaterials();

  return true;
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::SetEnabled(bool _enabled)
{
  this->enabled = _enabled;
  if (!this->enabled)
    this->RemovePassFromMaterials();
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::SetUsingShaders(bool _usingShaders)
{
  this->usingShaders = _usingShaders;
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::SetSceneNode()
{
  if (this->node)
  {
    this->node->detachObject(this->frustum);
    this->visual->GetSceneNode()->removeAndDestroyChild(this->nodeName);
    this->node = NULL;
  }

  if (this->filterNode)
  {
    this->filterNode->detachObject(this->filterFrustum);
    this->visual->GetSceneNode()->removeAndDestroyChild(this->filterNodeName);
    this->filterNode = NULL;
  }

  this->node = this->visual->GetSceneNode()->createChildSceneNode(
      this->nodeName);

  this->filterNode = this->visual->GetSceneNode()->createChildSceneNode(
      this->filterNodeName);

  if (this->node)
    this->node->attachObject(this->frustum);

  if (this->filterNode)
  {
    this->filterNode->attachObject(this->filterFrustum);
    this->filterNode->setOrientation(
      Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y));
  }
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::SetPose(const math::Pose &_pose)
{
  Ogre::Quaternion ogreQuaternion = Conversions::Convert(_pose.rot);
  Ogre::Vector3 ogreVec = Conversions::Convert(_pose.pos);
  Ogre::Quaternion offsetQuaternion;

  this->node->setPosition(ogreVec);
  this->node->setOrientation(ogreQuaternion);
  this->filterNode->setPosition(ogreVec);

  offsetQuaternion = Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y);
  this->filterNode->setOrientation(offsetQuaternion + ogreQuaternion);
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::SetTexture(
    const std::string &_textureName)
{
  this->textureName = _textureName;
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::SetFrustumClipDistance(double _near,
                                                               double _far)
{
  this->frustum->setNearClipDistance(_near);
  this->filterFrustum->setNearClipDistance(_near);
  this->frustum->setFarClipDistance(_far);
  this->filterFrustum->setFarClipDistance(_far);
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::SetFrustumFOV(double _fov)
{
  this->frustum->setFOVy(Ogre::Radian(_fov));
  this->filterFrustum->setFOVy(Ogre::Radian(_fov));
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::AddPassToAllMaterials()
{
  std::list<std::string> allMaterials;

  Ogre::SceneManager::MovableObjectIterator it =
    this->sceneMgr->getMovableObjectIterator("Entity");

  while (it.hasMoreElements())
  {
    Ogre::Entity* entity = dynamic_cast<Ogre::Entity*>(it.getNext());
    if (entity && entity->getName().find("visual") != std::string::npos)

    for (unsigned int i = 0; i < entity->getNumSubEntities(); i++)
    {
      allMaterials.push_back(entity->getSubEntity(i)->getMaterialName());
    }
  }

  this->AddPassToMaterials(allMaterials);
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::AddPassToVisibleMaterials()
{
  std::list<std::string> newVisibleMaterials;
  Ogre::PlaneBoundedVolumeList volumeList;

  volumeList.push_back(this->frustum->getPlaneBoundedVolume());

  this->projectorQuery->setVolumes(volumeList);
  Ogre::SceneQueryResult result = this->projectorQuery->execute();

  // Find all visible materials
  Ogre::SceneQueryResultMovableList::iterator it;
  for (it = result.movables.begin(); it != result.movables.end(); ++it)
  {
    Ogre::Entity* entity = dynamic_cast<Ogre::Entity*>(*it);
    if (entity && entity->getName().find("visual") != std::string::npos)
    {
      for (unsigned int i = 0; i < entity->getNumSubEntities(); i++)
      {
        newVisibleMaterials.push_back(
          entity->getSubEntity(i)->getMaterialName());
      }
    }
  }

  this->AddPassToMaterials(newVisibleMaterials);
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::AddPassToMaterials(
    std::list<std::string> &_matList)
{
  _matList.remove("");
  _matList.unique();

  std::string invisibleMaterial;
  std::list<std::string>::iterator visibleMaterial;

  // Loop through all existing passes, removing those for materials
  //   not in the newlist and skipping pass creation for those in the
  //   newlist that have already been created
  OgrePassMapIterator used = projectorTargets.begin();
  while (used != projectorTargets.end())
  {
    visibleMaterial = std::find(_matList.begin(), _matList.end(), used->first);

    // Remove the pass if it applies to a material not in the new list
    if (visibleMaterial == _matList.end())
    {
      invisibleMaterial = used->first;
      ++used;
      this->RemovePassFromMaterial(invisibleMaterial);
    }
    // Otherwise remove it from the list of passes to be added
    else
    {
      _matList.remove(used->first);
      ++used;
    }
  }

  if (!_matList.empty())
  {
    // Add pass for new materials
    while (!_matList.empty())
    {
      this->AddPassToMaterial(_matList.front());
      _matList.erase(_matList.begin());
    }

    RTShaderSystem::Instance()->UpdateShaders();
  }
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::AddPassToMaterial(
    const std::string &_matName)
{
  if (this->projectorTargets.find(_matName) != this->projectorTargets.end())
  {
    gzerr << "Adding a Material [" << _matName << "] that already exists.";
    return;
  }

  Ogre::MaterialPtr mat = static_cast<Ogre::MaterialPtr>(
    Ogre::MaterialManager::getSingleton().getByName(_matName));
  Ogre::Pass *pass = mat->getTechnique(0)->createPass();

  if (this->usingShaders)
  {
    Ogre::Matrix4 viewProj = this->frustum->getProjectionMatrix() *
                             this->frustum->getViewMatrix();

    pass->setVertexProgram("Gazebo/TextureProjectionVP");

    // pass->setFragmentProgram("GazeboWorlds/TexProjectionFP");
    Ogre::GpuProgramParametersSharedPtr vsParams =
      pass->getVertexProgramParameters();

    Ogre::GpuProgramParametersSharedPtr psParams =
      pass->getFragmentProgramParameters();

    // vsParams->setNamedAutoConstant(
    //   "worldViewProjMatrix",
    //   GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX);
    // vsParams->setNamedAutoConstant(
    //   "worldMatrix",GpuProgramParameters::ACT_WORLD_MATRIX);
    // vsParams->setNamedConstant("texViewProjMatrix", viewProj);

    vsParams->setNamedAutoConstant(
      "worldMatrix", Ogre::GpuProgramParameters::ACT_WORLD_MATRIX);

    vsParams->setNamedConstant("texProjMatrix", viewProj);

    // psParams->setNamedConstant("projMap", viewProj);

    pass->setVertexProgramParameters(vsParams);

    // pass->setFragmentProgramParameters(psParams);
  }

  pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
  pass->setDepthBias(1);
  pass->setLightingEnabled(false);

  Ogre::TextureUnitState *texState =
    pass->createTextureUnitState(this->textureName);
  texState->setProjectiveTexturing(true, this->frustum);
  texState->setTextureAddressingMode(Ogre::TextureUnitState::TAM_BORDER);
  texState->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
  texState->setTextureBorderColour(Ogre::ColourValue(0.0, 0.0, 0.0, 0.0));
  texState->setColourOperation(Ogre::LBO_ALPHA_BLEND);

  texState = pass->createTextureUnitState("projection_filter.png");
  texState->setProjectiveTexturing(true, this->filterFrustum);
  texState->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
  texState->setTextureFiltering(Ogre::TFO_NONE);

  this->projectorTargets[_matName] = pass;
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::RemovePassFromMaterials()
{
  for (OgrePassMap::const_iterator it = this->projectorTargets.begin();
      it != this->projectorTargets.end(); ++it)
  {
    it->second->getParent()->removePass(it->second->getIndex());
  }
  this->projectorTargets.clear();
}

/////////////////////////////////////////////////
void Projector::ProjectorFrameListener::RemovePassFromMaterial(
    const std::string &_matName)
{
  this->projectorTargets[_matName]->getParent()->removePass(
    this->projectorTargets[_matName]->getIndex());
  this->projectorTargets.erase(this->projectorTargets.find(_matName));
}
