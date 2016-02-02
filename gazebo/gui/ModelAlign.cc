/*
 * Copyright (C) 2014-2016 Open Source Robotics Foundation
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

#ifdef _WIN32
  // Ensure that Winsock2.h is included before Windows.h, which can get
  // pulled in by anybody (e.g., Boost).
  #include <Winsock2.h>
#endif

#include "gazebo/transport/transport.hh"

#include "gazebo/rendering/RenderTypes.hh"
#include "gazebo/rendering/RenderEvents.hh"
#include "gazebo/rendering/RenderingIface.hh"
#include "gazebo/rendering/Visual.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/UserCamera.hh"
#include "gazebo/rendering/RayQuery.hh"

#include "gazebo/gui/qt.h"
#include "gazebo/gui/MouseEventHandler.hh"
#include "gazebo/gui/GuiIface.hh"
#include "gazebo/gui/GuiEvents.hh"

#include "gazebo/gui/ModelAlignPrivate.hh"
#include "gazebo/gui/ModelAlign.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
ModelAlign::ModelAlign()
  : dataPtr(new ModelAlignPrivate)
{
  this->dataPtr->initialized = false;
}

/////////////////////////////////////////////////
ModelAlign::~ModelAlign()
{
  this->Clear();
  delete this->dataPtr;
  this->dataPtr = NULL;
}

/////////////////////////////////////////////////
void ModelAlign::Clear()
{
  this->dataPtr->targetVis.reset();
  this->dataPtr->scene.reset();
  this->dataPtr->node.reset();
  this->dataPtr->userCmdPub.reset();
  this->dataPtr->selectedVisuals.clear();
  this->dataPtr->connections.clear();
  this->dataPtr->originalVisualPose.clear();
  this->dataPtr->initialized = false;
}

/////////////////////////////////////////////////
void ModelAlign::Init()
{
  if (this->dataPtr->initialized)
    return;

  rendering::UserCameraPtr cam = gui::get_active_camera();
  if (!cam)
  {
    this->dataPtr->scene = rendering::get_scene();
  }
  else
  {
    this->dataPtr->scene = cam->GetScene();
  }

  if (!this->dataPtr->scene)
  {
    gzerr << "Unable to initialize Model Align tool, scene is NULL"
        << std::endl;
  }

  this->dataPtr->node = transport::NodePtr(new transport::Node());
  this->dataPtr->node->Init();
  this->dataPtr->userCmdPub =
      this->dataPtr->node->Advertise<msgs::UserCmd>("~/user_cmd");

  this->dataPtr->initialized = true;
}

/////////////////////////////////////////////////
void ModelAlign::Transform(math::Box _bbox, math::Pose _worldPose,
    std::vector<math::Vector3> &_vertices)
{
  math::Vector3 center = _bbox.GetCenter();

  // Get the 8 corners of the bounding box.
  math::Vector3 v0 = center +
      math::Vector3(-_bbox.GetXLength()/2.0, _bbox.GetYLength()/2.0,
      _bbox.GetZLength()/2.0);
  math::Vector3 v1 = center +
      math::Vector3(_bbox.GetXLength()/2.0, _bbox.GetYLength()/2.0,
      _bbox.GetZLength()/2.0);
  math::Vector3 v2 = center +
      math::Vector3(-_bbox.GetXLength()/2.0, -_bbox.GetYLength()/2.0,
      _bbox.GetZLength()/2.0);
  math::Vector3 v3 = center +
      math::Vector3(_bbox.GetXLength()/2.0, -_bbox.GetYLength()/2.0,
      _bbox.GetZLength()/2.0);

  math::Vector3 v4 = center +
      math::Vector3(-_bbox.GetXLength()/2.0, _bbox.GetYLength()/2.0,
      -_bbox.GetZLength()/2.0);
  math::Vector3 v5 = center +
      math::Vector3(_bbox.GetXLength()/2.0, _bbox.GetYLength()/2.0,
      -_bbox.GetZLength()/2.0);
  math::Vector3 v6 = center +
      math::Vector3(-_bbox.GetXLength()/2.0, -_bbox.GetYLength()/2.0,
      -_bbox.GetZLength()/2.0);
  math::Vector3 v7 = center +
      math::Vector3(_bbox.GetXLength()/2.0, -_bbox.GetYLength()/2.0,
      -_bbox.GetZLength()/2.0);

  // Transform corners into world spacce.
  v0 = _worldPose.rot * v0 + _worldPose.pos;
  v1 = _worldPose.rot * v1 + _worldPose.pos;
  v2 = _worldPose.rot * v2 + _worldPose.pos;
  v3 = _worldPose.rot * v3 + _worldPose.pos;
  v4 = _worldPose.rot * v4 + _worldPose.pos;
  v5 = _worldPose.rot * v5 + _worldPose.pos;
  v6 = _worldPose.rot * v6 + _worldPose.pos;
  v7 = _worldPose.rot * v7 + _worldPose.pos;

  _vertices.clear();
  _vertices.push_back(v0);
  _vertices.push_back(v1);
  _vertices.push_back(v2);
  _vertices.push_back(v3);
  _vertices.push_back(v4);
  _vertices.push_back(v5);
  _vertices.push_back(v6);
  _vertices.push_back(v7);
}

/////////////////////////////////////////////////
void ModelAlign::GetMinMax(std::vector<math::Vector3> _vertices,
    math::Vector3 &_min, math::Vector3 &_max)
{
  if (_vertices.empty())
    return;

  _min = _vertices[0];
  _max = _vertices[0];

  // find min / max in world space;
  for (unsigned int i = 1; i < _vertices.size(); ++i)
  {
    math::Vector3 v = _vertices[i];
    if (_min.x > v.x)
      _min.x = v.x;
    if (_max.x < v.x)
      _max.x = v.x;
    if (_min.y > v.y)
      _min.y = v.y;
    if (_max.y < v.y)
      _max.y = v.y;
    if (_min.z > v.z)
      _min.z = v.z;
    if (_max.z < v.z)
      _max.z = v.z;
  }
}

/////////////////////////////////////////////////
void ModelAlign::AlignVisuals(std::vector<rendering::VisualPtr> _visuals,
    const std::string &_axis, const std::string &_config,
    const std::string &_target, const bool _publish, const bool _inverted)
{
  if (_config == "reset" || _publish)
  {
    std::map<rendering::VisualPtr, math::Pose>::iterator it =
        this->dataPtr->originalVisualPose.begin();
    for (it; it != this->dataPtr->originalVisualPose.end(); ++it)
    {
      if (it->first)
      {
        it->first->SetWorldPose(it->second);
        this->SetHighlighted(it->first, false);
      }
    }
    this->dataPtr->originalVisualPose.clear();
    if (this->dataPtr->scene)
      this->dataPtr->scene->SelectVisual("", "normal");
    if (!_publish)
      return;
  }

  this->dataPtr->selectedVisuals = _visuals;

  if (this->dataPtr->selectedVisuals.size() <= 1)
    return;

  unsigned int start = 0;
  unsigned int end = 0;
  if (_target == "first")
  {
    start = 1;
    end = this->dataPtr->selectedVisuals.size();
    this->dataPtr->targetVis = this->dataPtr->selectedVisuals.front();
  }
  else if (_target == "last")
  {
    start = 0;
    end = this->dataPtr->selectedVisuals.size()-1;
    this->dataPtr->targetVis = this->dataPtr->selectedVisuals.back();
  }

  math::Pose targetWorldPose = this->dataPtr->targetVis->GetWorldPose();
  math::Box targetBbox = this->dataPtr->targetVis->GetBoundingBox();
  targetBbox.min *= this->dataPtr->targetVis->GetScale();
  targetBbox.max *= this->dataPtr->targetVis->GetScale();

  std::vector<math::Vector3> targetVertices;
  this->Transform(targetBbox, targetWorldPose, targetVertices);

  math::Vector3 targetMin;
  math::Vector3 targetMax;
  this->GetMinMax(targetVertices, targetMin, targetMax);

  std::vector<rendering::VisualPtr> visualsToPublish;
  for (unsigned i = start; i < end; ++i)
  {
    rendering::VisualPtr vis = this->dataPtr->selectedVisuals[i];

    math::Pose worldPose = vis->GetWorldPose();
    math::Box bbox = vis->GetBoundingBox();
    bbox.min *= vis->GetScale();
    bbox.max *= vis->GetScale();

    std::vector<math::Vector3> vertices;
    this->Transform(bbox, worldPose, vertices);

    math::Vector3 min;
    math::Vector3 max;
    this->GetMinMax(vertices, min, max);

    math::Vector3 trans;
    if (_config == "center")
    {
      trans = (targetMin + (targetMax-targetMin)/2) - (min + (max-min)/2);
    }
    else
    {
      if (!_inverted)
      {
        if (_config == "min")
          trans = targetMin - min;
        else if (_config == "max")
          trans = targetMax - max;
      }
      else
      {
        if (_config == "min")
          trans = targetMin - max;
        else if (_config == "max")
          trans = targetMax - min;
      }
    }

    if (!_publish)
    {
      if (this->dataPtr->originalVisualPose.find(vis) ==
          this->dataPtr->originalVisualPose.end())
      {
        this->dataPtr->originalVisualPose[vis] = vis->GetWorldPose();
        this->SetHighlighted(vis, true);
      }
      // prevent the visual pose from being updated by the server
      if (this->dataPtr->scene)
        this->dataPtr->scene->SelectVisual(vis->GetName(), "move");
    }

    if (_axis == "x")
    {
      trans.y = trans.z = 0;
      vis->SetWorldPosition(vis->GetWorldPose().pos + trans);
    }
    else if (_axis == "y")
    {
      trans.x = trans.z = 0;
      vis->SetWorldPosition(vis->GetWorldPose().pos + trans);
    }
    else if (_axis == "z")
    {
      trans.x = trans.y = 0;
      vis->SetWorldPosition(vis->GetWorldPose().pos + trans);
    }

    if (_publish)
      visualsToPublish.push_back(vis);
  }
  // Register user command on server
  if (_publish)
  {
    msgs::UserCmd userCmdMsg;
    userCmdMsg.set_description(
        "Align to [" + this->dataPtr->targetVis->GetName() + "]");
    userCmdMsg.set_type(msgs::UserCmd::MOVING);

    for (const auto &vis : visualsToPublish)
    {
      // Only publish for models
      if (vis->GetType() == gazebo::rendering::Visual::VT_MODEL)
      {
        msgs::Model msg;

        auto id = gui::get_entity_id(vis->GetName());
        if (id)
          msg.set_id(id);

        msg.set_name(vis->GetName());
        msgs::Set(msg.mutable_pose(), vis->GetWorldPose().Ign());

        auto modelMsg = userCmdMsg.add_model();
        modelMsg->CopyFrom(msg);
      }
    }

    this->dataPtr->userCmdPub->Publish(userCmdMsg);
  }
}

/////////////////////////////////////////////////
void ModelAlign::SetHighlighted(rendering::VisualPtr _vis, bool _highlight)
{
  if (_vis->GetChildCount() != 0)
  {
    for (unsigned int j = 0; j < _vis->GetChildCount(); ++j)
    {
      this->SetHighlighted(_vis->GetChild(j), _highlight);
    }
  }
  else
  {
    // Highlighting increases transparency for opaque visuals (0 < t < 0.3) and
    // decreases transparency for semi-transparent visuals (0.3 < t < 1).
    // A visual will never become fully transparent (t = 1) when highlighted.
    if (_highlight)
    {
      _vis->SetTransparency((1.0 - _vis->GetTransparency()) * 0.5);
    }
    // The inverse operation restores the original transparency value.
    else
    {
      _vis->SetTransparency(std::abs(_vis->GetTransparency()*2.0-1.0));
    }
  }
}
