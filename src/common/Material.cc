/*
 * Copyright 2011 Nate Koenig & Andrew Howard
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include "common/Material.hh"

using namespace gazebo;
using namespace common;


unsigned int Material::counter = 0;

std::string Material::ShadeModeStr[SHADE_COUNT] = {"FLAT", "GOURAUD",
  "PHONG", "BLINN"};
std::string Material::BlendModeStr[BLEND_COUNT] = {"ADD", "MODULATE",
  "REPLACE"};

//////////////////////////////////////////////////
Material::Material()
{
  this->name = "gazebo_material_" + boost::lexical_cast<std::string>(counter++);
  this->blendMode = REPLACE;
  this->shadeMode = GOURAUD;
  this->transparency = 0;
  this->shininess = 0;
  this->ambient.Set(1, 1, 1, 1);
  this->diffuse.Set(1, 1, 1, 1);
  this->specular.Set(0, 0, 0, 1);
  this->lighting = false;
  this->dstBlendFactor = this->srcBlendFactor = 1.0;
}

//////////////////////////////////////////////////
Material::Material(const Color &_clr)
{
  this->name = "gazebo_material_" + boost::lexical_cast<std::string>(counter++);
  this->blendMode = REPLACE;
  this->shadeMode = GOURAUD;
  this->transparency = 0;
  this->shininess = 0;
  this->ambient = _clr;
  this->diffuse = _clr;
  this->lighting = false;
}

//////////////////////////////////////////////////
Material::~Material()
{
}

//////////////////////////////////////////////////
std::string Material::GetName() const
{
  return this->name;
}

//////////////////////////////////////////////////
void Material::SetTextureImage(const std::string &_tex)
{
  this->texImage = _tex;
}

//////////////////////////////////////////////////
void Material::SetTextureImage(const std::string &_tex,
                               const std::string &_resourcePath)
{
  this->texImage = _resourcePath + "/" + _tex;
}

//////////////////////////////////////////////////
std::string Material::GetTextureImage() const
{
  return this->texImage;
}

//////////////////////////////////////////////////
void Material::SetAmbient(const Color &_clr)
{
  this->ambient = _clr;
}

//////////////////////////////////////////////////
Color Material::GetAmbient() const
{
  return this->ambient;
}

//////////////////////////////////////////////////
void Material::SetDiffuse(const Color &_clr)
{
  this->diffuse = _clr;
  this->lighting = true;
}

//////////////////////////////////////////////////
Color Material::GetDiffuse() const
{
  return this->diffuse;
}

//////////////////////////////////////////////////
void Material::SetSpecular(const Color &_clr)
{
  this->specular = _clr;
  this->lighting = true;
}

//////////////////////////////////////////////////
Color Material::GetSpecular() const
{
  return this->specular;
}

//////////////////////////////////////////////////
void Material::SetEmissive(const Color &_clr)
{
  this->emissive = _clr;
}

//////////////////////////////////////////////////
Color Material::GetEmissive() const
{
  return this->emissive;
}

//////////////////////////////////////////////////
void Material::SetTransparency(double _t)
{
  this->transparency = std::min(_t, 1.0);
  this->transparency = std::max(this->transparency, 0.0);
  this->lighting = true;
}

//////////////////////////////////////////////////
double Material::GetTransparency() const
{
  return this->transparency;
}

//////////////////////////////////////////////////
void Material::SetShininess(double _s)
{
  this->shininess = _s;
  this->lighting = true;
}

//////////////////////////////////////////////////
double Material::GetShininess() const
{
  return this->shininess;
}

//////////////////////////////////////////////////
void Material::SetBlendFactors(double _srcFactor, double _dstFactor)
{
  this->srcBlendFactor = _srcFactor;
  this->dstBlendFactor = _dstFactor;
}

//////////////////////////////////////////////////
void Material::GetBlendFactors(double &_srcFactor, double &_dstFactor)
{
  _srcFactor = this->srcBlendFactor;
  _dstFactor = this->dstBlendFactor;
}


//////////////////////////////////////////////////
void Material::SetBlendMode(BlendMode _b)
{
  this->blendMode = _b;
}

//////////////////////////////////////////////////
Material::BlendMode Material::GetBlendMode() const
{
  return this->blendMode;
}

//////////////////////////////////////////////////
void Material::SetShadeMode(ShadeMode _s)
{
  this->shadeMode = _s;
}

//////////////////////////////////////////////////
Material::ShadeMode Material::GetShadeMode() const
{
  return this->shadeMode;
}

//////////////////////////////////////////////////
void Material::SetPointSize(double _size)
{
  this->pointSize = _size;
}

//////////////////////////////////////////////////
double Material::GetPointSize() const
{
  return this->pointSize;
}
//////////////////////////////////////////////////
void Material::SetDepthWrite(bool _value)
{
  this->depthWrite = _value;
}

//////////////////////////////////////////////////
bool Material::GetDepthWrite() const
{
  return this->depthWrite;
}

//////////////////////////////////////////////////
void Material::SetLighting(bool _value)
{
  this->lighting = _value;
}

//////////////////////////////////////////////////
bool Material::GetLighting() const
{
  return this->lighting;
}
