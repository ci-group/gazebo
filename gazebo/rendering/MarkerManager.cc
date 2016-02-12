/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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
#include "gazebo/transport/Node.hh"
#include "gazebo/transport/Subscriber.hh"
#include "gazebo/common/Events.hh"

#include "gazebo/rendering/RenderingIface.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/MarkerVisual.hh"
#include "gazebo/rendering/MarkerManagerPrivate.hh"
#include "gazebo/rendering/MarkerManager.hh"

using namespace gazebo;
using namespace rendering;

/////////////////////////////////////////////////
MarkerManager::MarkerManager()
  : dataPtr(new MarkerManagerPrivate)
{
}

/////////////////////////////////////////////////
MarkerManager::~MarkerManager()
{
  this->dataPtr->node->Fini();
  delete this->dataPtr;
}

/////////////////////////////////////////////////
bool MarkerManager::Init(ScenePtr _scene)
{
  if (!_scene)
  {
    gzerr << "Scene pointer is invalid\n";
    return false;
  }

  this->dataPtr->node = transport::NodePtr(new transport::Node());
  this->dataPtr->node->Init();
  this->dataPtr->scene = _scene;

  // Subscribe to the marker topic
  this->dataPtr->markerSub = this->dataPtr->node->Subscribe("~/marker",
      &MarkerManager::OnMarkerMsg, this);

  // Process markers on PreRender
  this->dataPtr->preRenderConnection = event::Events::ConnectPreRender(
      std::bind(&MarkerManager::OnPreRender, this));

  return true;
}

/////////////////////////////////////////////////
void MarkerManager::OnPreRender()
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  // Process the marker messages.
  for (auto markerIter = this->dataPtr->markerMsgs.begin();
       markerIter != this->dataPtr->markerMsgs.end();)
  {
    if (this->ProcessMarkerMsg(**markerIter))
      this->dataPtr->markerMsgs.erase(markerIter++);
    else
      ++markerIter;
  }

  // Erase any markers that have a lifetime.
  for (auto mit = this->dataPtr->markers.begin();
       mit != this->dataPtr->markers.end(); ++mit)
  {
    for (auto it = mit->second.cbegin(); it != mit->second.cend();)
    {
      // Erase a marker if it has a lifetime and it's expired.
      if (it->second->Lifetime() != common::Time::Zero &&
          it->second->Lifetime() <= this->dataPtr->scene->SimTime())
      {
        it = mit->second.erase(it);
      }
      else
        ++it;
    }

    // Erase a namespace if it's empty
    /*if (mit->second.empty())
      mit = this->dataPtr->markers.erase(mit);
    else
      ++mit;
      */
  }
}

//////////////////////////////////////////////////
bool MarkerManager::ProcessMarkerMsg(const msgs::Marker &_msg)
{
  // Get the namespace, if it exists
  std::string ns = "";
  if (_msg.has_ns())
    ns = _msg.ns();

  // Get the namespace that the marker belongs to
  Marker_M::iterator nsIter = this->dataPtr->markers.find(ns);

  // Add/modify a marker
  if (_msg.action() == msgs::Marker::ADD_MODIFY)
  {
    std::map<uint64_t, MarkerVisualPtr>::iterator markerIter;

    // Add the marker to an existing namespace, if the namespace exists.
    if (nsIter != this->dataPtr->markers.end() &&
        (markerIter = nsIter->second.find(_msg.id())) != nsIter->second.end())
    {
      markerIter->second->Load(_msg);
    }
    else
    {
      // Create the name for the marker
      std::string name = "__GZ_MARKER_VISUAL_" + ns + "_" +
        std::to_string(_msg.id());

      // Create the new marker
      MarkerVisualPtr marker(new MarkerVisual(name,
            this->dataPtr->scene->WorldVisual()));

      // Load the marker
      marker->Load(_msg);

      // Store the marker
      this->dataPtr->markers[ns][_msg.id()] = marker;
    }
  }
  else if (_msg.action() == msgs::Marker::DELETE_ALL)
  {
    if (nsIter != this->dataPtr->markers.end())
    {
      for (auto it = nsIter->second.begin(); it != nsIter->second.end(); ++it)
      {
        it->second->Fini();
        this->dataPtr->scene->RemoveVisual(it->second);
      }
      nsIter->second.clear();
    }
  }

  return true;
}

/////////////////////////////////////////////////
void MarkerManager::OnMarkerMsg(ConstMarkerPtr &_msg)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);
  this->dataPtr->markerMsgs.push_back(_msg);
}
