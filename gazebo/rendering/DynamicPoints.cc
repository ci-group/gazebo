/*
 * Copyright 2013 Open Source Robotics Foundation
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
/* Desc: Dynamic point generator
 * Author: Carlos Agüero
 * Date: 30 July 2013
 */

#include <math.h>

#include <cmath>
#include <sstream>
#include "gazebo/rendering/ogre_gazebo.h"

#include "gazebo/common/Exception.hh"
#include "gazebo/rendering/DynamicPoints.hh"

using namespace gazebo;
using namespace rendering;

enum {POSITION_BINDING, TEXCOORD_BINDING};

/////////////////////////////////////////////////
DynamicPoints::DynamicPoints(RenderOpType opType)
{
  this->Init(opType, false);
  this->setCastShadows(false);
  this->dirty = true;
}

/////////////////////////////////////////////////
DynamicPoints::~DynamicPoints()
{
}

/////////////////////////////////////////////////
std::string DynamicPoints::GetMovableType()
{
  return "gazebo::DynamicPoints";
}

/////////////////////////////////////////////////
const Ogre::String &DynamicPoints::getMovableType() const
{
  static Ogre::String moveType = DynamicPoints::GetMovableType();
  return moveType;
}

/////////////////////////////////////////////////
void DynamicPoints::AddPoint(const math::Vector3 &pt)
{
  this->points.push_back(pt);
  this->dirty = true;
}

/////////////////////////////////////////////////
void DynamicPoints::AddPoint(double _x, double _y, double _z)
{
  this->AddPoint(math::Vector3(_x, _y, _z));
}

/////////////////////////////////////////////////
void DynamicPoints::SetPoint(unsigned int index, const math::Vector3 &value)
{
  if (index >= this->points.size())
  {
    std::ostringstream stream;
    stream << "Point index[" << index << "] is out of bounds[0-"
           << this->points.size()-1 << "]";
    gzthrow(stream.str());
  }

  this->points[index] = value;

  this->dirty = true;
}

/////////////////////////////////////////////////
const math::Vector3& DynamicPoints::GetPoint(unsigned int index) const
{
  if (index >= this->points.size())
  {
    gzthrow("Point index is out of bounds");
  }

  return this->points[index];
}

/////////////////////////////////////////////////
unsigned int DynamicPoints::GetPointCount() const
{
  return this->points.size();
}

/////////////////////////////////////////////////
void DynamicPoints::Clear()
{
  this->points.clear();
  this->dirty = true;
}

/////////////////////////////////////////////////
void DynamicPoints::Update()
{
  if (this->dirty && this->points.size() > 1)
    this->FillHardwareBuffers();
}

/////////////////////////////////////////////////
void DynamicPoints::CreateVertexDeclaration()
{
  Ogre::VertexDeclaration *decl =
    this->mRenderOp.vertexData->vertexDeclaration;

  decl->addElement(POSITION_BINDING, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
}

/////////////////////////////////////////////////
void DynamicPoints::FillHardwareBuffers()
{
  int size = this->points.size();
  this->PrepareHardwareBuffers(size, 0);

  if (!size)
  {
    this->mBox.setExtents(Ogre::Vector3::ZERO, Ogre::Vector3::ZERO);
    this->dirty = false;
  }

  Ogre::HardwareVertexBufferSharedPtr vbuf =
    this->mRenderOp.vertexData->vertexBufferBinding->getBuffer(0);

  Ogre::Real *prPos =
    static_cast<Ogre::Real*>(vbuf->lock(Ogre::HardwareBuffer::HBL_NORMAL));
  {
    for (int i = 0; i < size; i++)
    {
      *prPos++ = this->points[i].x;
      *prPos++ = this->points[i].y;
      *prPos++ = this->points[i].z;

      this->mBox.merge(Ogre::Vector3(this->points[i].x,
                                     this->points[i].y, this->points[i].z));
    }
  }
  vbuf->unlock();

  // need to update after mBox change, otherwise the points goes in and out
  // of scope based on old mBox
  this->getParentSceneNode()->needUpdate();

  this->dirty = false;
}
