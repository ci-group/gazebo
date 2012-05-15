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
/* Desc: External interfaces for Gazebo
 * Author: Nate Koenig
 * Date: 03 Apr 2007
 */

#include <boost/thread/recursive_mutex.hpp>

#include "msgs/msgs.h"

#include "common/Events.hh"
#include "common/Console.hh"
#include "common/Animation.hh"
#include "common/KeyFrame.hh"

#include "transport/Publisher.hh"
#include "transport/Transport.hh"
#include "transport/Node.hh"

#include "physics/RayShape.hh"
#include "physics/Collision.hh"
#include "physics/Model.hh"
#include "physics/Link.hh"
#include "physics/World.hh"
#include "physics/PhysicsEngine.hh"
#include "physics/Entity.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
Entity::Entity(BasePtr _parent)
  : Base(_parent)
{
  this->isCanonicalLink = false;
  this->node = transport::NodePtr(new transport::Node());
  this->AddType(ENTITY);

  this->visualMsg = new msgs::Visual;
  this->poseMsg = new msgs::Pose;

  if (this->parent && this->parent->HasType(ENTITY))
  {
    this->parentEntity = boost::shared_dynamic_cast<Entity>(this->parent);
    this->SetStatic(this->parentEntity->IsStatic());
  }

  this->setWorldPoseFunc = &Entity::SetWorldPoseDefault;
}

//////////////////////////////////////////////////
Entity::~Entity()
{
  Base_V::iterator iter;

  // TODO: put this back in
  // this->GetWorld()->GetPhysicsEngine()->RemoveEntity(this);

  delete this->visualMsg;
  this->visualMsg = NULL;

  delete this->poseMsg;
  this->poseMsg = NULL;

  this->posePub.reset();
  this->visPub.reset();
  this->requestPub.reset();
  this->poseSub.reset();
  this->node.reset();
}

//////////////////////////////////////////////////
void Entity::Load(sdf::ElementPtr _sdf)
{
  Base::Load(_sdf);
  this->node->Init(this->GetWorld()->GetName());
  this->posePub = this->node->Advertise<msgs::Pose>("~/pose/info", 10);

  this->poseSub = this->node->Subscribe("~/pose/modify",
      &Entity::OnPoseMsg, this);
  this->visPub = this->node->Advertise<msgs::Visual>("~/visual", 10);
  this->requestPub = this->node->Advertise<msgs::Request>("~/request");

  this->visualMsg->set_name(this->GetScopedName());

  if (this->sdf->HasElement("origin"))
  {
    sdf::ElementPtr originElem = this->sdf->GetElement("origin");
    if (this->parent && this->parentEntity)
      this->worldPose = originElem->GetValuePose("pose") +
                        this->parentEntity->worldPose;
    else
      this->worldPose = originElem->GetValuePose("pose");

    this->initialRelativePose = originElem->GetValuePose("pose");
  }

  if (this->parent)
    this->visualMsg->set_parent_name(this->parent->GetScopedName());
  msgs::Set(this->visualMsg->mutable_pose(), this->GetRelativePose());

  this->visPub->Publish(*this->visualMsg);

  this->poseMsg->set_name(this->GetScopedName());

  if (this->HasType(Base::MODEL))
    this->setWorldPoseFunc = &Entity::SetWorldPoseModel;
  else if (this->IsCanonicalLink())
    this->setWorldPoseFunc = &Entity::SetWorldPoseCanonicalLink;
  else
    this->setWorldPoseFunc = &Entity::SetWorldPoseDefault;
}


//////////////////////////////////////////////////
void Entity::SetName(const std::string &_name)
{
  // TODO: if an entitie's _name is changed, then the old visual is never
  // removed. Should add in functionality to modify/update the visual
  Base::SetName(_name);
}

//////////////////////////////////////////////////
void Entity::SetStatic(const bool &_s)
{
  Base_V::iterator iter;

  this->isStatic = _s;

  for (iter = this->children.begin(); iter != this->childrenEnd; ++iter)
  {
    EntityPtr e = boost::shared_dynamic_cast<Entity>(*iter);
    if (e)
      e->SetStatic(_s);
  }
}

//////////////////////////////////////////////////
bool Entity::IsStatic() const
{
  return this->isStatic;
}

//////////////////////////////////////////////////
void Entity::SetInitialRelativePose(const math::Pose &_p)
{
  this->initialRelativePose = _p;
}

//////////////////////////////////////////////////
math::Box Entity::GetBoundingBox() const
{
  return math::Box(math::Vector3(0, 0, 0), math::Vector3(1, 1, 1));
}

//////////////////////////////////////////////////
void Entity::SetCanonicalLink(bool _value)
{
  this->isCanonicalLink = _value;
}

//////////////////////////////////////////////////
void Entity::SetAnimation(common::PoseAnimationPtr _anim)
{
  this->animationStartPose = this->worldPose;

  this->prevAnimationTime = this->world->GetSimTime();
  this->animation = _anim;
  this->onAnimationComplete.clear();
  this->animationConnection = event::Events::ConnectWorldUpdateStart(
      boost::bind(&Entity::UpdateAnimation, this));
}

//////////////////////////////////////////////////
void Entity::SetAnimation(const common::PoseAnimationPtr &_anim,
                          boost::function<void()> _onComplete)
{
  this->animationStartPose = this->worldPose;

  this->prevAnimationTime = this->world->GetSimTime();
  this->animation = _anim;
  this->onAnimationComplete = _onComplete;
  this->animationConnection = event::Events::ConnectWorldUpdateStart(
      boost::bind(&Entity::UpdateAnimation, this));
}

//////////////////////////////////////////////////
void Entity::StopAnimation()
{
  this->animation.reset();
  this->onAnimationComplete.clear();
  if (this->animationConnection)
  {
    event::Events::DisconnectWorldUpdateStart(this->animationConnection);
    this->animationConnection.reset();
  }
}

//////////////////////////////////////////////////
void Entity::PublishPose()
{
  if (this->posePub && this->posePub->HasConnections())
  {
    math::Pose relativePose = this->GetRelativePose();
    if (relativePose != msgs::Convert(*this->poseMsg))
    {
      msgs::Set(this->poseMsg, relativePose);
      this->posePub->Publish(*this->poseMsg);
    }
  }
}

//////////////////////////////////////////////////
math::Pose Entity::GetRelativePose() const
{
  if (this->IsCanonicalLink())
  {
    return this->initialRelativePose;
  }
  else if (this->parent && this->parentEntity)
  {
    return this->worldPose - this->parentEntity->GetWorldPose();
  }
  else
  {
    return this->worldPose;
  }
}

//////////////////////////////////////////////////
void Entity::SetRelativePose(const math::Pose &_pose, bool _notify)
{
  if (this->parent && this->parentEntity)
    this->SetWorldPose(_pose + this->parentEntity->GetWorldPose(), _notify);
  else
    this->SetWorldPose(_pose, _notify);
}

//////////////////////////////////////////////////
void Entity::SetWorldTwist(const math::Vector3 &_linear,
    const math::Vector3 &_angular, bool _updateChildren)
{
  if (this->HasType(LINK) || this->HasType(MODEL))
  {
    if (this->HasType(LINK))
    {
      Link* link = dynamic_cast<Link*>(this);
      link->SetLinearVel(_linear);
      link->SetAngularVel(_angular);
    }
    if (_updateChildren)
    {
      // force an update of all children
      for  (Base_V::iterator iter = this->children.begin();
            iter != this->childrenEnd; ++iter)
      {
        if ((*iter)->HasType(ENTITY))
        {
          EntityPtr entity = boost::shared_static_cast<Entity>(*iter);
          entity->SetWorldTwist(_linear, _angular, _updateChildren);
        }
      }
    }
  }
}

//////////////////////////////////////////////////
void Entity::SetWorldPoseModel(const math::Pose &_pose, bool _notify)
{
  math::Pose oldModelWorldPose = this->worldPose;

  // initialization: (no children?) set own worldPose
  this->worldPose = _pose;
  this->worldPose.Correct();

  // (OnPoseChange uses GetWorldPose)
  if (_notify)
    this->UpdatePhysicsPose(false);

  //
  // user deliberate setting: lock and update all children's wp
  //

  // force an update of all children
  // update all children pose, moving them with the model.
  for (Base_V::iterator iter = this->children.begin();
       iter != this->childrenEnd; ++iter)
  {
    if ((*iter)->HasType(ENTITY))
    {
      EntityPtr entity = boost::shared_static_cast<Entity>(*iter);

      if (entity->IsCanonicalLink())
      {
        entity->worldPose = (entity->initialRelativePose + _pose);
      }
      else
      {
        entity->worldPose = ((entity->worldPose - oldModelWorldPose) + _pose);
        entity->PublishPose();
      }

      if (_notify)
        entity->UpdatePhysicsPose(false);
    }
  }
}

//////////////////////////////////////////////////
void Entity::SetWorldPoseCanonicalLink(const math::Pose &_pose, bool _notify)
{
  this->worldPose = _pose;
  this->worldPose.Correct();

  if (_notify)
    this->UpdatePhysicsPose(true);

  // also update parent model's pose
  if (this->parentEntity->HasType(MODEL))
  {
    this->parentEntity->worldPose.pos = _pose.pos -
      this->parentEntity->worldPose.rot.RotateVector(
          this->initialRelativePose.pos);
    this->parentEntity->worldPose.rot = _pose.rot *
      this->initialRelativePose.rot.GetInverse();

    this->parentEntity->worldPose.Correct();

    if (_notify)
      this->parentEntity->UpdatePhysicsPose(false);

    this->parentEntity->PublishPose();
  }
  else
    gzerr << "SWP for CB[" << this->GetName() << "] but parent["
      << this->parentEntity->GetName() << "] is not a MODEL!\n";
}

//////////////////////////////////////////////////
void Entity::SetWorldPoseDefault(const math::Pose &_pose, bool _notify)
{
  this->worldPose = _pose;
  this->worldPose.Correct();

  if (_notify)
    this->UpdatePhysicsPose(true);
}


//////////////////////////////////////////////////
//   The entity stores an initialRelativePose and dynamic worldPose
//   When calling SetWroldPose (SWP) or SetRelativePose on an entity
//   that is a Model (M), Canonical Body (CB) or Body (B), different
//   considerations need to be taken.
// Below is a table that summarizes the current code.
//  +----------------------------------------------------------+
//  |     |     M          |  CB             |  B              |
//  |----------------------------------------------------------|
//  |SWP  | Lock           | Lock            | Set BWP         |
//  |     | Update MWP     | Set CBWP        |                 |
//  |     | SWP Children   | SWP M = CB-CBRP |                 |
//  |----------------------------------------------------------|
//  |SRP  | WP = RP + PP   | WP = RP + PP    | WP = RP + PP    |
//  |----------------------------------------------------------|
//  |GWP  | return WP      | return WP       | return WP       |
//  |----------------------------------------------------------|
//  |GRP  | RP = WP - RP   | return CBRP     | RP = WP - RP    |
//  +----------------------------------------------------------+
//  Legends
//    M    - Model
//    CB   - Canonical Body
//    B    - Non-Canonical Body
//    *WP  - *WorldPose
//    *RP  - *RelativePose (relative to parent)
//    SWP  - SetWorldPose
//    GWP  - GetWorldPose
//    MWP  - Model World Pose
//    CBRP - Canonical Body Relative (to Model) Pose
//
void Entity::SetWorldPose(const math::Pose &_pose, bool _notify)
{
  this->GetWorld()->setWorldPoseMutex->lock();

  (*this.*setWorldPoseFunc)(_pose, _notify);

  this->GetWorld()->setWorldPoseMutex->unlock();

  this->PublishPose();
}

//////////////////////////////////////////////////
void Entity::UpdatePhysicsPose(bool _updateChildren)
{
  this->OnPoseChange();

  if (!this->HasType(COLLISION) && (_updateChildren || this->IsStatic()))
  {
    for (Base_V::iterator iter = this->children.begin();
         iter != this->childrenEnd; ++iter)
    {
      if ((*iter)->HasType(LINK))
        boost::shared_static_cast<Link>(*iter)->OnPoseChange();
      else if ((*iter)->HasType(COLLISION))
      {
        CollisionPtr coll = boost::shared_static_cast<Collision>(*iter);

        if (this->IsStatic())
        {
          coll->worldPose = this->worldPose + coll->GetRelativePose();
        }
        coll->OnPoseChange();
      }
      else
      {
        // Should never get here.
        gzthrow(std::string("Invalid type[") +
                boost::lexical_cast<std::string>((*iter)->GetType()) + "]");
      }
    }
  }
}

//////////////////////////////////////////////////
ModelPtr Entity::GetParentModel()
{
  BasePtr p;
  if (this->HasType(MODEL))
    return boost::shared_dynamic_cast<Model>(shared_from_this());

  p = this->parent;

  while (p->GetParent() && p->GetParent()->HasType(MODEL))
    p = p->GetParent();

  return boost::shared_dynamic_cast<Model>(p);
}

//////////////////////////////////////////////////
CollisionPtr Entity::GetChildCollision(const std::string &_name)
{
  BasePtr base = this->GetByName(_name);
  if (base)
    return boost::shared_dynamic_cast<Collision>(base);

  return CollisionPtr();
}

//////////////////////////////////////////////////
LinkPtr Entity::GetChildLink(const std::string &_name)
{
  BasePtr base = this->GetByName(_name);
  if (base)
    return boost::shared_dynamic_cast<Link>(base);

  return LinkPtr();
}

//////////////////////////////////////////////////
void Entity::OnPoseMsg(ConstPosePtr &_msg)
{
  if (_msg->name() == this->GetScopedName())
  {
    math::Pose p = msgs::Convert(*_msg);
    this->SetWorldPose(p);
  }
}

//////////////////////////////////////////////////
void Entity::Fini()
{
  msgs::Request *msg = msgs::CreateRequest("entity_delete",
      this->GetScopedName());

  this->requestPub->Publish(*msg, true);

  this->parentEntity.reset();
  Base::Fini();

  this->connections.clear();
  this->node->Fini();
}

//////////////////////////////////////////////////
void Entity::Reset()
{
  this->GetWorld()->setWorldPoseMutex->lock();
  Base::Reset();

  if (this->HasType(Base::MODEL))
    this->SetWorldPose(this->initialRelativePose);
  else
    this->SetRelativePose(this->initialRelativePose);
  this->GetWorld()->setWorldPoseMutex->unlock();
}

//////////////////////////////////////////////////
void Entity::UpdateParameters(sdf::ElementPtr _sdf)
{
  Base::UpdateParameters(_sdf);

  math::Pose parentPose;
  if (this->parent && this->parentEntity)
    parentPose = this->parentEntity->worldPose;

  math::Pose newPose = _sdf->GetElement("origin")->GetValuePose("pose");
  if (newPose != this->GetRelativePose())
  {
    this->SetRelativePose(newPose);
  }
}

//////////////////////////////////////////////////
void Entity::UpdateAnimation()
{
  common::PoseKeyFrame kf(0);

  this->animation->AddTime(
      (this->world->GetSimTime() - this->prevAnimationTime).Double());
  this->animation->GetInterpolatedKeyFrame(kf);

  math::Pose offset;
  offset.pos = kf.GetTranslation();
  offset.rot = kf.GetRotation();

  this->SetWorldPose(offset);
  this->prevAnimationTime = this->world->GetSimTime();

  if (this->animation->GetLength() <= this->animation->GetTime())
  {
    event::Events::DisconnectWorldUpdateStart(this->animationConnection);
    this->animationConnection.reset();
    if (this->onAnimationComplete)
    {
      this->onAnimationComplete();
    }
  }
}

//////////////////////////////////////////////////
const math::Pose &Entity::GetDirtyPose() const
{
  return this->dirtyPose;
}

//////////////////////////////////////////////////
math::Box Entity::GetCollisionBoundingBox() const
{
  math::Box box;
  for (Base_V::const_iterator iter = this->children.begin();
       iter != this->children.end(); ++iter)
  {
    box += this->GetCollisionBoundingBoxHelper(*iter);
  }

  return box;
}

//////////////////////////////////////////////////
math::Box Entity::GetCollisionBoundingBoxHelper(BasePtr _base) const
{
  if (_base->HasType(COLLISION))
    return boost::shared_dynamic_cast<Collision>(_base)->GetBoundingBox();

  math::Box box;

  for (unsigned int i = 0; i < _base->GetChildCount(); i++)
  {
    box += this->GetCollisionBoundingBoxHelper(_base->GetChild(i));
  }

  return box;
}

//////////////////////////////////////////////////
void Entity::PlaceOnEntity(const std::string &_entityName)
{
  EntityPtr onEntity = this->GetWorld()->GetEntity(_entityName);
  math::Box box = this->GetCollisionBoundingBox();
  math::Box onBox = onEntity->GetCollisionBoundingBox();

  math::Pose p = onEntity->GetWorldPose();
  p.pos.z = onBox.max.z + box.GetZLength()*0.5;
  this->SetWorldPose(p);
}

//////////////////////////////////////////////////
void Entity::GetNearestEntityBelow(double &_distBelow,
                                   std::string &_entityName)
{
  RayShapePtr rayShape = boost::shared_dynamic_cast<RayShape>(
    this->GetWorld()->GetPhysicsEngine()->CreateShape("ray", CollisionPtr()));

  math::Box box = this->GetCollisionBoundingBox();
  math::Vector3 start = this->GetWorldPose().pos;
  math::Vector3 end = start;
  start.z = box.min.z - 0.00001;
  end.z -= 1000;
  rayShape->SetPoints(start, end);
  rayShape->GetIntersection(_distBelow, _entityName);
  _distBelow += 0.00001;
}

//////////////////////////////////////////////////
void Entity::PlaceOnNearestEntityBelow()
{
  double dist;
  std::string entityName;
  this->GetNearestEntityBelow(dist, entityName);
  if (dist > 0.0)
  {
    math::Pose p = this->GetWorldPose();
    p.pos.z -= dist;
    this->SetWorldPose(p);
  }
}
