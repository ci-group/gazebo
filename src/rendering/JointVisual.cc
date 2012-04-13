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
/* Desc: Joint Visualization Class
 * Author: Nate Koenig
 */

#include "rendering/ogre.h"
#include "rendering/DynamicLines.hh"
#include "rendering/Scene.hh"
#include "rendering/AxisVisual.hh"
#include "rendering/JointVisual.hh"

using namespace gazebo;
using namespace rendering;

/////////////////////////////////////////////////
JointVisual::JointVisual(const std::string &_name, VisualPtr _vis)
  : Visual(_name, _vis, false)
{
}

/////////////////////////////////////////////////
JointVisual::~JointVisual()
{
  this->axisVisual.reset();
}

/////////////////////////////////////////////////
void JointVisual::Load(ConstJointPtr &_msg)
{
  Visual::Load();

  this->axisVisual.reset(
      new AxisVisual(this->GetName() + "_AXIS", shared_from_this()));
  this->axisVisual->Load();

  this->SetWorldPosition(msgs::Convert(_msg->pose().position()));
  this->SetWorldRotation(msgs::Convert(_msg->pose().orientation()));

  if (math::equal(_msg->axis1().xyz().x(), 1))
    this->axisVisual->ShowRotation(0);

  if (math::equal(_msg->axis1().xyz().y(), 1))
    this->axisVisual->ShowRotation(1);

  if (math::equal(_msg->axis1().xyz().z(), 1))
    this->axisVisual->ShowRotation(2);
}
