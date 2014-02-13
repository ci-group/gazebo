/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
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
/* Desc: Axis Visualization Class
 * Author: Nate Koenig
 */

#include "gazebo/common/MeshManager.hh"

#include "gazebo/rendering/ogre_gazebo.h"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/ArrowVisual.hh"
#include "gazebo/rendering/AxisVisualPrivate.hh"
#include "gazebo/rendering/AxisVisual.hh"

using namespace gazebo;
using namespace rendering;

/////////////////////////////////////////////////
AxisVisual::AxisVisual(const std::string &_name, VisualPtr _vis)
  : Visual(*new AxisVisualPrivate, _name, _vis, false)
{
}

/////////////////////////////////////////////////
AxisVisual::~AxisVisual()
{
  AxisVisualPrivate *dPtr =
      reinterpret_cast<AxisVisualPrivate *>(this->dataPtr);

  dPtr->xAxis.reset();
  dPtr->yAxis.reset();
  dPtr->zAxis.reset();
}

/////////////////////////////////////////////////
void AxisVisual::Load()
{
  AxisVisualPrivate *dPtr =
      reinterpret_cast<AxisVisualPrivate *>(this->dataPtr);

  Visual::Load();

  dPtr->xAxis.reset(new ArrowVisual(this->GetName() +
      "_X_AXIS", shared_from_this()));
  dPtr->xAxis->Load();
  dPtr->xAxis->SetMaterial("__GAZEBO_TRANS_RED_MATERIAL__");

  dPtr->yAxis.reset(new ArrowVisual(this->GetName() +
      "_Y_AXIS", shared_from_this()));
  dPtr->yAxis->Load();
  dPtr->yAxis->SetMaterial("__GAZEBO_TRANS_GREEN_MATERIAL__");

  dPtr->zAxis.reset(new ArrowVisual(this->GetName() +
      "_Z_AXIS", shared_from_this()));
  dPtr->zAxis->Load();
  dPtr->zAxis->SetMaterial("__GAZEBO_TRANS_BLUE_MATERIAL__");

  dPtr->xAxis->SetRotation(
      math::Quaternion(math::Vector3(0, 1, 0), GZ_DTOR(90)));

  dPtr->yAxis->SetRotation(
      math::Quaternion(math::Vector3(1, 0, 0), GZ_DTOR(-90)));

  this->SetVisibilityFlags(GZ_VISIBILITY_GUI);
}

/////////////////////////////////////////////////
void AxisVisual::ScaleXAxis(const math::Vector3 &_scale)
{
  AxisVisualPrivate *dPtr =
      reinterpret_cast<AxisVisualPrivate *>(this->dataPtr);

  dPtr->xAxis->SetScale(_scale);
}

/////////////////////////////////////////////////
void AxisVisual::ScaleYAxis(const math::Vector3 &_scale)
{
  AxisVisualPrivate *dPtr =
      reinterpret_cast<AxisVisualPrivate *>(this->dataPtr);

  dPtr->yAxis->SetScale(_scale);
}

/////////////////////////////////////////////////
void AxisVisual::ScaleZAxis(const math::Vector3 &_scale)
{
  AxisVisualPrivate *dPtr =
      reinterpret_cast<AxisVisualPrivate *>(this->dataPtr);

  dPtr->zAxis->SetScale(_scale);
}

/////////////////////////////////////////////////
void AxisVisual::SetAxisMaterial(unsigned int _axis,
                                 const std::string &_material)
{
  AxisVisualPrivate *dPtr =
      reinterpret_cast<AxisVisualPrivate *>(this->dataPtr);

  switch (_axis)
  {
    case 0:
      dPtr->xAxis->SetMaterial(_material);
      break;
    case 1:
      dPtr->yAxis->SetMaterial(_material);
      break;
    case 2:
      dPtr->zAxis->SetMaterial(_material);
      break;
    default:
      gzerr << "Invlid axis index[" << _axis << "]\n";
      break;
  };
}

/////////////////////////////////////////////////
void AxisVisual::ShowRotation(unsigned int _axis)
{
  AxisVisualPrivate *dPtr =
      reinterpret_cast<AxisVisualPrivate *>(this->dataPtr);

  switch (_axis)
  {
    case 0:
      dPtr->xAxis->ShowRotation();
      break;
    case 1:
      dPtr->yAxis->ShowRotation();
      break;
    case 2:
      dPtr->zAxis->ShowRotation();
      break;
    default:
      gzerr << "Invlid axis index[" << _axis << "]\n";
      break;
  };
}
