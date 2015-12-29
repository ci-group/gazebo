/*
 * Copyright (C) 2015-2016 Open Source Robotics Foundation
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

#include <ignition/math/MassMatrix3.hh>

#include "gazebo/math/Vector3.hh"
#include "gazebo/math/Quaternion.hh"
#include "gazebo/math/Pose.hh"

#include "gazebo/rendering/DynamicLines.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/InertiaVisualPrivate.hh"
#include "gazebo/rendering/InertiaVisual.hh"

using namespace gazebo;
using namespace rendering;

/////////////////////////////////////////////////
InertiaVisual::InertiaVisual(const std::string &_name, VisualPtr _vis)
  : Visual(*new InertiaVisualPrivate, _name, _vis, false)
{
  InertiaVisualPrivate *dPtr =
      reinterpret_cast<InertiaVisualPrivate *>(this->dataPtr);
  dPtr->type = VT_PHYSICS;
}

/////////////////////////////////////////////////
InertiaVisual::~InertiaVisual()
{
  InertiaVisualPrivate *dPtr =
      reinterpret_cast<InertiaVisualPrivate *>(this->dataPtr);
  if (dPtr && dPtr->sceneNode)
  {
    this->DestroyAllAttachedMovableObjects(dPtr->sceneNode);
    dPtr->sceneNode->removeAndDestroyAllChildren();
  }
}

/////////////////////////////////////////////////
void InertiaVisual::Fini()
{
  InertiaVisualPrivate *dPtr =
      reinterpret_cast<InertiaVisualPrivate *>(this->dataPtr);
  if (dPtr && dPtr->sceneNode)
  {
    this->DestroyAllAttachedMovableObjects(dPtr->sceneNode);
    dPtr->sceneNode->removeAndDestroyAllChildren();
  }
  Visual::Fini();
}

/////////////////////////////////////////////////
void InertiaVisual::Load(sdf::ElementPtr _elem)
{
  Visual::Load();
  math::Pose pose = _elem->Get<math::Pose>("origin");
  this->Load(pose);
}

/////////////////////////////////////////////////
void InertiaVisual::Load(ConstLinkPtr &_msg)
{
  Visual::Load();

  auto xyz = msgs::ConvertIgn(_msg->inertial().pose().position());
  auto q = msgs::ConvertIgn(_msg->inertial().pose().orientation());

  // Use ignition::math::MassMatrix3 to compute
  // equivalent box size and rotation
  ignition::math::MassMatrix3d m(_msg->inertial().mass(),
      ignition::math::Vector3d(_msg->inertial().ixx(),
                               _msg->inertial().iyy(),
                               _msg->inertial().izz()),
      ignition::math::Vector3d(_msg->inertial().ixy(),
                               _msg->inertial().ixz(),
                               _msg->inertial().iyz()));
  ignition::math::Vector3d boxScale;
  ignition::math::Quaterniond boxRot;
  if (!m.EquivalentBox(boxScale, boxRot))
  {
    // Invalid inertia, load with default scale
    gzlog << "The link " << _msg->name() << " has unrealistic inertia, "
          << "unable to visualize box of equivalent inertia." << std::endl;
    this->Load(math::Pose(xyz, q));
  }
  else
  {
    // Apply additional rotation by boxRot
    this->Load(math::Pose(xyz, q * boxRot), boxScale);
  }
}

/////////////////////////////////////////////////
void InertiaVisual::Load(const math::Pose &_pose,
    const math::Vector3 &_scale)
{
  InertiaVisualPrivate *dPtr =
      reinterpret_cast<InertiaVisualPrivate *>(this->dataPtr);

  // Inertia position indicator
  ignition::math::Vector3d p1(0, 0, -2*_scale.z);
  ignition::math::Vector3d p2(0, 0, 2*_scale.z);
  ignition::math::Vector3d p3(0, -2*_scale.y, 0);
  ignition::math::Vector3d p4(0, 2*_scale.y, 0);
  ignition::math::Vector3d p5(-2*_scale.x, 0, 0);
  ignition::math::Vector3d p6(2*_scale.x, 0, 0);
  p1 = _pose.rot.RotateVector(p1).Ign();
  p2 = _pose.rot.RotateVector(p2).Ign();
  p3 = _pose.rot.RotateVector(p3).Ign();
  p4 = _pose.rot.RotateVector(p4).Ign();
  p5 = _pose.rot.RotateVector(p5).Ign();
  p6 = _pose.rot.RotateVector(p6).Ign();
  p1 += _pose.pos.Ign();
  p2 += _pose.pos.Ign();
  p3 += _pose.pos.Ign();
  p4 += _pose.pos.Ign();
  p5 += _pose.pos.Ign();
  p6 += _pose.pos.Ign();

  dPtr->crossLines = this->CreateDynamicLine(rendering::RENDERING_LINE_LIST);
  dPtr->crossLines->setMaterial("Gazebo/Green");
  dPtr->crossLines->AddPoint(p1);
  dPtr->crossLines->AddPoint(p2);
  dPtr->crossLines->AddPoint(p3);
  dPtr->crossLines->AddPoint(p4);
  dPtr->crossLines->AddPoint(p5);
  dPtr->crossLines->AddPoint(p6);

  // Inertia indicator: equivalent box of uniform density
  this->InsertMesh("unit_box");

  Ogre::MovableObject *boxObj =
    (Ogre::MovableObject*)(dPtr->scene->OgreSceneManager()->createEntity(
          this->GetName()+"__BOX__", "unit_box"));
  boxObj->setVisibilityFlags(GZ_VISIBILITY_GUI);
  ((Ogre::Entity*)boxObj)->setMaterialName("__GAZEBO_TRANS_PURPLE_MATERIAL__");

  dPtr->boxNode =
      dPtr->sceneNode->createChildSceneNode(this->GetName() + "_BOX_");

  dPtr->boxNode->attachObject(boxObj);
  dPtr->boxNode->setScale(_scale.x, _scale.y, _scale.z);
  dPtr->boxNode->setPosition(_pose.pos.x, _pose.pos.y, _pose.pos.z);
  dPtr->boxNode->setOrientation(Ogre::Quaternion(_pose.rot.w, _pose.rot.x,
                                                 _pose.rot.y, _pose.rot.z));

  this->SetVisibilityFlags(GZ_VISIBILITY_GUI);
}

/////////////////////////////////////////////////
void InertiaVisual::DestroyAllAttachedMovableObjects(
        Ogre::SceneNode *_sceneNode)
{
  if (!_sceneNode)
    return;

  // Destroy all the attached objects
  Ogre::SceneNode::ObjectIterator itObject =
    _sceneNode->getAttachedObjectIterator();

  while (itObject.hasMoreElements())
  {
    Ogre::Entity *ent = static_cast<Ogre::Entity*>(itObject.getNext());
    if (ent->getMovableType() != DynamicLines::GetMovableType())
      this->dataPtr->scene->OgreSceneManager()->destroyEntity(ent);
    else
      delete ent;
  }

  // Recurse to child SceneNodes
  Ogre::SceneNode::ChildNodeIterator itChild = _sceneNode->getChildIterator();

  while (itChild.hasMoreElements())
  {
    Ogre::SceneNode* pChildNode =
        static_cast<Ogre::SceneNode*>(itChild.getNext());
    this->DestroyAllAttachedMovableObjects(pChildNode);
  }
}
