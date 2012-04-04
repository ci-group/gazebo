/*
 * Copyright 2011 Nate Koenig & Andrew Howard
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use _mat file except in compliance with the License.
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
#include "common/Color.hh"
#include "rendering/ogre.h"
#include "common/Console.hh"
#include "rendering/Material.hh"

using namespace gazebo;
using namespace rendering;

//////////////////////////////////////////////////
void Material::CreateMaterials()
{
  Ogre::MaterialPtr mat;
  Ogre::Technique *tech;
  Ogre::Pass *pass;
  Ogre::TextureUnitState *texState;

  /*mat = Ogre::MaterialManager::getSingleton().create(
      "__GAZEBO_PURPLE_MATERIAL__", "General");
  tech = mat->getTechnique(0);
  pass = tech->getPass(0);
  tech->setLightingEnabled(false);
  pass->setAmbient(1.0, 0.0, 1.0);
  pass->setDiffuse(1.0, 0.0, 1.0, 0.5);
  texState = pass->createTextureUnitState();
  texState->setColourOperationEx(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
      Ogre::LBS_CURRENT, Ogre::ColourValue(1, 0, 1));

  mat = Ogre::MaterialManager::getSingleton().create(
      "__GAZEBO_RED_MATERIAL__", "General");
  tech = mat->getTechnique(0);
  pass = tech->getPass(0);
  tech->setLightingEnabled(false);
  pass->setAmbient(1.0, 0.0, 0.0);
  pass->setDiffuse(1.0, 0.0, 0.0, 0.5);
  texState = pass->createTextureUnitState();
  texState->setColourOperationEx(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
      Ogre::LBS_CURRENT, Ogre::ColourValue(1, 0, 0));

  mat = Ogre::MaterialManager::getSingleton().create(
      "__GAZEBO_GREEN_MATERIAL__", "General");
  tech = mat->getTechnique(0);
  pass = tech->getPass(0);
  tech->setLightingEnabled(false);
  pass->setAmbient(0.0, 1.0, 0.0);
  pass->setDiffuse(0.0, 1.0, 0.0, 0.5);
  texState = pass->createTextureUnitState();
  texState->setColourOperationEx(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
      Ogre::LBS_CURRENT, Ogre::ColourValue(0, 1, 0));

  mat = Ogre::MaterialManager::getSingleton().create(
      "__GAZEBO_BLUE_MATERIAL__", "General");
  tech = mat->getTechnique(0);
  pass = tech->getPass(0);
  tech->setLightingEnabled(false);
  pass->setAmbient(0.0, 0.0, 1.0);
  pass->setDiffuse(0.0, 0.0, 1.0, 0.5);
  texState = pass->createTextureUnitState();
  texState->setColourOperationEx(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
      Ogre::LBS_CURRENT, Ogre::ColourValue(0, 0, 1));




  mat = Ogre::MaterialManager::getSingleton().create(
      "__GAZEBO_TRANS_RED_MATERIAL__", "General");
  tech = mat->getTechnique(0);
  pass = tech->getPass(0);
  tech->setLightingEnabled(false);
  tech->setDepthWriteEnabled(false);
  pass->setAmbient(1.0, 0.0, 0.0);
  pass->setDiffuse(1.0, 0.0, 0.0, 0.5);
  pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
  texState = pass->createTextureUnitState();
  texState->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
                               Ogre::LBS_CURRENT, 0.5);
  texState->setColourOperationEx(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
      Ogre::LBS_CURRENT, Ogre::ColourValue(1, 0, 0));

  mat = Ogre::MaterialManager::getSingleton().create(
      "__GAZEBO_TRANS_GREEN_MATERIAL__", "General");
  tech = mat->getTechnique(0);
  pass = tech->getPass(0);
  tech->setLightingEnabled(false);
  tech->setDepthWriteEnabled(false);
  pass->setAmbient(0.0, 1.0, 0.0);
  pass->setDiffuse(0.0, 1.0, 0.0, 0.5);
  pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
  texState = pass->createTextureUnitState();
  texState->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
                               Ogre::LBS_CURRENT, 0.5);
  texState->setColourOperationEx(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
      Ogre::LBS_CURRENT, Ogre::ColourValue(0, 1, 0));

  mat = Ogre::MaterialManager::getSingleton().create(
      "__GAZEBO_TRANS_BLUE_MATERIAL__", "General");
  tech = mat->getTechnique(0);
  pass = tech->getPass(0);
  tech->setLightingEnabled(false);
  tech->setDepthWriteEnabled(false);
  pass->setAmbient(0.0, 0.0, 1.0);
  pass->setDiffuse(0.0, 0.0, 1.0, 0.5);
  pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
  texState = pass->createTextureUnitState();
  texState->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
                               Ogre::LBS_CURRENT, 0.5);
  texState->setColourOperationEx(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
      Ogre::LBS_CURRENT, Ogre::ColourValue(0, 0, 1));

  mat = Ogre::MaterialManager::getSingleton().create(
      "__GAZEBO_TRANS_PURPLE_MATERIAL__", "General");
  tech = mat->getTechnique(0);
  pass = tech->getPass(0);
  tech->setLightingEnabled(false);
  tech->setDepthWriteEnabled(false);
  pass->setAmbient(1.0, 0.0, 1.0);
  pass->setDiffuse(1.0, 0.0, 1.0, 0.5);
  pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
  texState = pass->createTextureUnitState();
  texState->setAlphaOperation(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
                               Ogre::LBS_CURRENT, 0.5);
  texState->setColourOperationEx(Ogre::LBX_SOURCE1, Ogre::LBS_MANUAL,
      Ogre::LBS_CURRENT, Ogre::ColourValue(1, 0, 1));
  */
}

//////////////////////////////////////////////////
void Material::Update(const gazebo::common::Material *_mat)
{
  Ogre::MaterialPtr matPtr;

  if (Ogre::MaterialManager::getSingleton().resourceExists(_mat->GetName()))
    matPtr = Ogre::MaterialManager::getSingleton().getByName(
        _mat->GetName(), "General");
  else
    matPtr = Ogre::MaterialManager::getSingleton().create(
        _mat->GetName(), "General");

  matPtr->setReceiveShadows(false);
  Ogre::Pass *pass = matPtr->getTechnique(0)->getPass(0);

  common::Color ambient =  _mat->GetAmbient();
  common::Color diffuse =  _mat->GetDiffuse();
  common::Color specular = _mat->GetSpecular();
  common::Color emissive = _mat->GetEmissive();

  pass->setLightingEnabled(_mat->GetLighting());
  pass->setDiffuse(diffuse.R(), diffuse.G(), diffuse.B(), diffuse.A());
  pass->setAmbient(ambient.R(), ambient.G(), ambient.B());

  if (diffuse.A() < 1.0)
  {
    pass->setDepthWriteEnabled(false);
    pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
  }

  pass->setSpecular(specular.R(), specular.G(), specular.B(), specular.A());
  pass->setSelfIllumination(emissive.R(), emissive.G(), emissive.B());

  pass->setShininess(_mat->GetShininess());

  // Only add the texture unit if it's not present in the material
  if (!_mat->GetTextureImage().empty() &&
      pass->getTextureUnitState(_mat->GetTextureImage()) == NULL)
  {
    Ogre::TextureUnitState *texState = pass->createTextureUnitState(
        _mat->GetTextureImage());
    texState->setTextureName(_mat->GetTextureImage());
    texState->setName(_mat->GetTextureImage());
  }
}
