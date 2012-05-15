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
/* Desc: Base class for all models.
 * Author: Nathan Koenig and Andrew Howard
 * Date: 8 May 2003
 */

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <float.h>

#include <boost/thread/recursive_mutex.hpp>
#include <sstream>

#include "common/KeyFrame.hh"
#include "common/Animation.hh"
#include "common/Plugin.hh"
#include "common/Events.hh"
#include "common/Exception.hh"
#include "common/Console.hh"
#include "common/CommonTypes.hh"

#include "physics/Gripper.hh"
#include "physics/Joint.hh"
#include "physics/JointController.hh"
#include "physics/Link.hh"
#include "physics/World.hh"
#include "physics/PhysicsEngine.hh"
#include "physics/Model.hh"
#include "physics/Contact.hh"

#include "transport/Node.hh"

using namespace gazebo;
using namespace physics;

class LinkUpdate_TBB
{
  public: LinkUpdate_TBB(Link_V *_bodies) : bodies(_bodies) {}
  public: void operator() (const tbb::blocked_range<size_t> &_r) const
  {
    for (size_t i = _r.begin(); i != _r.end(); i++)
    {
      (*this->bodies)[i]->Update();
    }
  }

  private: Link_V *bodies;
};


//////////////////////////////////////////////////
Model::Model(BasePtr _parent)
  : Entity(_parent)
{
  this->AddType(MODEL);
  this->updateMutex = new boost::recursive_mutex();
  this->jointController = NULL;
}

//////////////////////////////////////////////////
Model::~Model()
{
  delete this->updateMutex;
  delete this->jointController;
}

//////////////////////////////////////////////////
void Model::Load(sdf::ElementPtr _sdf)
{
  Entity::Load(_sdf);

  this->jointPub = this->node->Advertise<msgs::Joint>("~/joint");

  this->SetStatic(this->sdf->GetValueBool("static"));
  this->sdf->GetAttribute("static")->SetUpdateFunc(
      boost::bind(&Entity::IsStatic, this));

  // TODO: check for duplicate model, and raise an error
  // BasePtr dup = Base::GetByName(this->GetScopedName());

  // Load the bodies
  if (_sdf->HasElement("link"))
  {
    sdf::ElementPtr linkElem = _sdf->GetElement("link");
    bool first = true;
    while (linkElem)
    {
      // Create a new link
      LinkPtr link = this->GetWorld()->GetPhysicsEngine()->CreateLink(
          boost::shared_static_cast<Model>(shared_from_this()));

      // FIXME: canonical link is hardcoded to the first link.
      //        warn users for now, need  to add parsing of
      //        the canonical tag in sdf
      if (first)
      {
        link->SetCanonicalLink(true);
        this->canonicalLink = link;
        first = false;
      }

      // Load the link using the config node. This also loads all of the
      // bodies collisionetries
      link->Load(linkElem);
      linkElem = linkElem->GetNextElement("link");
    }
  }

  // Load the joints
  if (_sdf->HasElement("joint"))
  {
    sdf::ElementPtr jointElem = _sdf->GetElement("joint");
    while (jointElem)
    {
      this->LoadJoint(jointElem);
      jointElem = jointElem->GetNextElement("joint");
    }
  }

  // Load the plugins
  if (_sdf->HasElement("plugin"))
  {
    sdf::ElementPtr pluginElem = _sdf->GetElement("plugin");
    while (pluginElem)
    {
      this->LoadPlugin(pluginElem);
      pluginElem = pluginElem->GetNextElement("plugin");
    }
  }

  if (_sdf->HasElement("gripper"))
  {
    sdf::ElementPtr gripperElem = _sdf->GetElement("gripper");
    while (gripperElem)
    {
      this->LoadGripper(gripperElem);
      gripperElem = gripperElem->GetNextElement("gripper");
    }
  }
}

//////////////////////////////////////////////////
void Model::Init()
{
  // Record the model's initial pose (for reseting)
  this->SetInitialRelativePose(this->GetWorldPose());

  this->SetRelativePose(this->GetWorldPose());

  // Initialize the bodies before the joints
  for (Base_V::iterator iter = this->children.begin();
       iter!= this->children.end(); ++iter)
  {
    if ((*iter)->HasType(Base::LINK))
      boost::shared_static_cast<Link>(*iter)->Init();
    else if ((*iter)->HasType(Base::MODEL))
      boost::shared_static_cast<Model>(*iter)->Init();
  }

  // Initialize the joints last.
  for (Joint_V::iterator iter = this->joints.begin();
       iter != this->joints.end(); ++iter)
  {
    (*iter)->Init();
  }

  for (std::vector<ModelPluginPtr>::iterator iter = this->plugins.begin();
       iter != this->plugins.end(); ++iter)
  {
    (*iter)->Init();
  }

  for (std::vector<Gripper*>::iterator iter = this->grippers.begin();
       iter != this->grippers.end(); ++iter)
  {
    (*iter)->Init();
  }
}


//////////////////////////////////////////////////
void Model::Update()
{
  this->updateMutex->lock();

  if (this->jointController)
    this->jointController->Update();

  if (this->jointAnimations.size() > 0)
  {
    common::NumericKeyFrame kf(0);
    std::map<std::string, double> jointPositions;
    std::map<std::string, common::NumericAnimationPtr>::iterator iter;
    iter = this->jointAnimations.begin();
    while (iter != this->jointAnimations.end())
    {
      iter->second->GetInterpolatedKeyFrame(kf);

      iter->second->AddTime(
          (this->world->GetSimTime() - this->prevAnimationTime).Double());

      if (iter->second->GetTime() < iter->second->GetLength())
      {
        iter->second->GetInterpolatedKeyFrame(kf);
        jointPositions[iter->first] = kf.GetValue();
        ++iter;
      }
      else
      {
        this->jointAnimations.erase(iter++);
      }
    }
    if (!jointPositions.empty())
    {
      this->jointController->SetJointPositions(jointPositions);
    }
    else
    {
      if (this->onJointAnimationComplete)
        this->onJointAnimationComplete();
    }
    this->prevAnimationTime = this->world->GetSimTime();
  }

  this->updateMutex->unlock();
}

//////////////////////////////////////////////////
void Model::SetJointPositions(
    const std::map<std::string, double> &_jointPositions)
{
  if (this->jointController)
    this->jointController->SetJointPositions(_jointPositions);
}

//////////////////////////////////////////////////
void Model::RemoveChild(EntityPtr _child)
{
  Joint_V::iterator jiter;

  if (_child->HasType(LINK))
  {
    bool done = false;

    while (!done)
    {
      done = true;

      for (jiter = this->joints.begin(); jiter != this->joints.end(); ++jiter)
      {
        if (!(*jiter))
          continue;

        LinkPtr jlink0 = (*jiter)->GetJointLink(0);
        LinkPtr jlink1 = (*jiter)->GetJointLink(1);

        if (!jlink0 || !jlink1 || jlink0->GetName() == _child->GetName() ||
            jlink1->GetName() == _child->GetName() ||
            jlink0->GetName() == jlink1->GetName())
        {
          this->joints.erase(jiter);
          done = false;
          break;
        }
      }
    }
  }

  Entity::RemoveChild(_child->GetId());

  Base_V::iterator iter;
  for (iter = this->children.begin(); iter != this->children.end(); ++iter)
    if (*iter && (*iter)->HasType(LINK))
      boost::static_pointer_cast<Link>(*iter)->SetEnabled(true);
}

//////////////////////////////////////////////////
void Model::Fini()
{
  Entity::Fini();

  this->attachedModels.clear();
  this->joints.clear();
  this->plugins.clear();
  this->canonicalLink.reset();
}

//////////////////////////////////////////////////
void Model::UpdateParameters(sdf::ElementPtr _sdf)
{
  Entity::UpdateParameters(_sdf);

  if (_sdf->HasElement("link"))
  {
    sdf::ElementPtr linkElem = _sdf->GetElement("link");
    while (linkElem)
    {
      LinkPtr link = boost::shared_dynamic_cast<Link>(
          this->GetChild(linkElem->GetValueString("name")));
      link->UpdateParameters(linkElem);
      linkElem = linkElem->GetNextElement("link");
    }
  }
  /*

  if (_sdf->HasElement("joint"))
  {
    sdf::ElementPtr jointElem = _sdf->GetElement("joint");
    while (jointElem)
    {
      JointPtr joint = boost::shared_dynamic_cast<Joint>(this->GetChild(jointElem->GetValueString("name")));
      joint->UpdateParameters(jointElem);
      jointElem = jointElem->GetNextElement("joint");
    }
  }
  */
}

//////////////////////////////////////////////////
const sdf::ElementPtr Model::GetSDF()
{
  return Entity::GetSDF();
}

//////////////////////////////////////////////////
void Model::Reset()
{
  Entity::Reset();

  for (std::vector<ModelPluginPtr>::iterator iter = this->plugins.begin();
       iter != this->plugins.end(); ++iter)
  {
    (*iter)->Reset();
  }

  for (Joint_V::iterator jiter = this->joints.begin();
       jiter!= this->joints.end(); ++jiter)
  {
    (*jiter)->Reset();
  }
}

//////////////////////////////////////////////////
void Model::SetLinearVel(const math::Vector3 &_vel)
{
  for (Base_V::iterator iter = this->children.begin();
      iter != this->children.end(); ++iter)
  {
    if (*iter && (*iter)->HasType(LINK))
    {
      LinkPtr link = boost::shared_static_cast<Link>(*iter);
      link->SetEnabled(true);
      link->SetLinearVel(_vel);
    }
  }
}

//////////////////////////////////////////////////
void Model::SetAngularVel(const math::Vector3 &_vel)
{
  Base_V::iterator iter;

  for (iter = this->children.begin(); iter != this->children.end(); ++iter)
  {
    if (*iter && (*iter)->HasType(LINK))
    {
      LinkPtr link = boost::shared_static_cast<Link>(*iter);
      link->SetEnabled(true);
      link->SetAngularVel(_vel);
    }
  }
}

//////////////////////////////////////////////////
void Model::SetLinearAccel(const math::Vector3 &_accel)
{
  Base_V::iterator iter;

  for (iter = this->children.begin(); iter != this->children.end(); ++iter)
  {
    if (*iter && (*iter)->HasType(LINK))
    {
      LinkPtr link = boost::shared_static_cast<Link>(*iter);
      link->SetEnabled(true);
      link->SetLinearAccel(_accel);
    }
  }
}

//////////////////////////////////////////////////
void Model::SetAngularAccel(const math::Vector3 &_accel)
{
  Base_V::iterator iter;

  for (iter = this->children.begin(); iter != this->children.end(); ++iter)
  {
    if (*iter && (*iter)->HasType(LINK))
    {
      LinkPtr link = boost::shared_static_cast<Link>(*iter);
      link->SetEnabled(true);
      link->SetAngularAccel(_accel);
    }
  }
}

//////////////////////////////////////////////////
math::Vector3 Model::GetRelativeLinearVel() const
{
  if (this->GetLink("canonical"))
    return this->GetLink("canonical")->GetRelativeLinearVel();
  else
    return math::Vector3(0, 0, 0);
}

//////////////////////////////////////////////////
math::Vector3 Model::GetWorldLinearVel() const
{
  if (this->GetLink("canonical"))
    return this->GetLink("canonical")->GetWorldLinearVel();
  else
    return math::Vector3(0, 0, 0);
}

//////////////////////////////////////////////////
math::Vector3 Model::GetRelativeAngularVel() const
{
  if (this->GetLink("canonical"))
    return this->GetLink("canonical")->GetRelativeAngularVel();
  else
    return math::Vector3(0, 0, 0);
}

//////////////////////////////////////////////////
math::Vector3 Model::GetWorldAngularVel() const
{
  if (this->GetLink("canonical"))
    return this->GetLink("canonical")->GetWorldAngularVel();
  else
    return math::Vector3(0, 0, 0);
}


//////////////////////////////////////////////////
math::Vector3 Model::GetRelativeLinearAccel() const
{
  if (this->GetLink("canonical"))
    return this->GetLink("canonical")->GetRelativeLinearAccel();
  else
    return math::Vector3(0, 0, 0);
}

//////////////////////////////////////////////////
math::Vector3 Model::GetWorldLinearAccel() const
{
  if (this->GetLink("canonical"))
    return this->GetLink("canonical")->GetWorldLinearAccel();
  else
    return math::Vector3(0, 0, 0);
}

//////////////////////////////////////////////////
math::Vector3 Model::GetRelativeAngularAccel() const
{
  if (this->GetLink("canonical"))
    return this->GetLink("canonical")->GetRelativeAngularAccel();
  else
    return math::Vector3(0, 0, 0);
}

//////////////////////////////////////////////////
math::Vector3 Model::GetWorldAngularAccel() const
{
  if (this->GetLink("canonical"))
    return this->GetLink("canonical")->GetWorldAngularAccel();
  else
    return math::Vector3(0, 0, 0);
}

//////////////////////////////////////////////////
math::Box Model::GetBoundingBox() const
{
  math::Box box;
  Base_V::const_iterator iter;

  box.min.Set(FLT_MAX, FLT_MAX, FLT_MAX);
  box.max.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

  for (iter = this->children.begin(); iter!= this->children.end(); ++iter)
  {
    if (*iter && (*iter)->HasType(LINK))
    {
      math::Box linkBox;
      LinkPtr link = boost::shared_static_cast<Link>(*iter);
      linkBox = link->GetBoundingBox();
      box += linkBox;
    }
  }

  return box;
}

//////////////////////////////////////////////////
unsigned int Model::GetJointCount() const
{
  return this->joints.size();
}

//////////////////////////////////////////////////
JointPtr Model::GetJoint(unsigned int _index) const
{
  if (_index >= this->joints.size())
    gzthrow("Invalid joint _index[" << _index << "]\n");

  return this->joints[_index];
}

//////////////////////////////////////////////////
JointPtr Model::GetJoint(const std::string &_name)
{
  JointPtr result;
  Joint_V::iterator iter;

  for (iter = this->joints.begin(); iter != this->joints.end(); ++iter)
  {
    if ((*iter)->GetName() == _name)
    {
      result = (*iter);
      break;
    }
  }

  return result;
}

//////////////////////////////////////////////////
LinkPtr Model::GetLinkById(unsigned int _id) const
{
  return boost::shared_dynamic_cast<Link>(this->GetById(_id));
}

//////////////////////////////////////////////////
LinkPtr Model::GetLink(const std::string &_name) const
{
  Base_V::const_iterator biter;
  LinkPtr result;

  if (_name == "canonical")
  {
    result = this->canonicalLink;
  }
  else
  {
    for (biter = this->children.begin(); biter != this->children.end(); ++biter)
    {
      if ((*biter)->GetName() == _name)
      {
        result = boost::shared_dynamic_cast<Link>(*biter);
        break;
      }
    }
  }

  return result;
}

//////////////////////////////////////////////////
LinkPtr Model::GetLink(unsigned int _index) const
{
  LinkPtr link;
  if (_index <= this->GetChildCount())
    link = boost::shared_static_cast<Link>(this->GetChild(_index));
  else
    gzerr << "Index is out of range\n";

  return link;
}

//////////////////////////////////////////////////
void Model::LoadJoint(sdf::ElementPtr _sdf)
{
  JointPtr joint;

  std::string stype = _sdf->GetValueString("type");

  joint = this->GetWorld()->GetPhysicsEngine()->CreateJoint(stype);
  if (!joint)
    gzthrow("Unable to create joint of type[" + stype + "]\n");

  joint->SetModel(boost::shared_static_cast<Model>(shared_from_this()));

  // Load the joint
  joint->Load(_sdf);

  if (this->GetJoint(joint->GetName()) != NULL)
    gzthrow("can't have two joint with the same name");

  msgs::Joint msg;
  joint->FillJointMsg(msg);
  this->jointPub->Publish(msg);

  this->joints.push_back(joint);

  if (!this->jointController)
    this->jointController = new JointController(
        boost::shared_dynamic_cast<Model>(shared_from_this()));
  this->jointController->AddJoint(joint);
}

//////////////////////////////////////////////////
void Model::LoadGripper(sdf::ElementPtr _sdf)
{
  Gripper *gripper = new Gripper(
      boost::shared_static_cast<Model>(shared_from_this()));
  gripper->Load(_sdf);
  this->grippers.push_back(gripper);
}

//////////////////////////////////////////////////
void Model::LoadPlugin(sdf::ElementPtr _sdf)
{
  std::string name = _sdf->GetValueString("name");
  std::string filename = _sdf->GetValueString("filename");
  gazebo::ModelPluginPtr plugin = gazebo::ModelPlugin::Create(filename, name);
  if (plugin)
  {
    ModelPtr myself = boost::shared_static_cast<Model>(shared_from_this());
    plugin->Load(myself, _sdf);
    this->plugins.push_back(plugin);
  }
}

//////////////////////////////////////////////////
void Model::SetGravityMode(const bool &_v)
{
  Base_V::iterator iter;

  for (iter = this->children.begin(); iter!= this->children.end(); ++iter)
  {
    if (*iter && (*iter)->HasType(LINK))
    {
      boost::shared_static_cast<Link>(*iter)->SetGravityMode(_v);
    }
  }
}


//////////////////////////////////////////////////
void Model::SetCollideMode(const std::string &_m)
{
  Base_V::iterator iter;

  for (iter = this->children.begin(); iter!= this->children.end(); ++iter)
  {
    if (*iter && (*iter)->HasType(LINK))
    {
      boost::shared_static_cast<Link>(*iter)->SetCollideMode(_m);
    }
  }
}


//////////////////////////////////////////////////
void Model::SetLaserRetro(const float &_retro)
{
  Base_V::iterator iter;

  for (iter = this->children.begin(); iter!= this->children.end(); ++iter)
  {
    if (*iter && (*iter)->HasType(LINK))
    {
       boost::shared_static_cast<Link>(*iter)->SetLaserRetro(_retro);
    }
  }
}

//////////////////////////////////////////////////
void Model::FillModelMsg(msgs::Model &_msg)
{
  _msg.set_name(this->GetScopedName());
  _msg.set_is_static(this->IsStatic());
  _msg.mutable_pose()->CopyFrom(msgs::Convert(this->GetWorldPose()));
  _msg.set_id(this->GetId());

  msgs::Set(this->visualMsg->mutable_pose(), this->GetWorldPose());
  _msg.add_visual()->CopyFrom(*this->visualMsg);

  for (unsigned int j = 0; j < this->GetChildCount(); ++j)
  {
    if (this->GetChild(j)->HasType(Base::LINK))
    {
      LinkPtr link = boost::shared_dynamic_cast<Link>(this->GetChild(j));
      link->FillLinkMsg(*_msg.add_link());
    }
  }

  for (unsigned int j = 0; j < this->joints.size(); ++j)
    this->joints[j]->FillJointMsg(*_msg.add_joint());
}

//////////////////////////////////////////////////
void Model::ProcessMsg(const msgs::Model &_msg)
{
  if (!(_msg.has_id() && _msg.id() == this->GetId()))
  {
    gzerr << "Incorrect ID[" << _msg.id() << " != " << this->GetId() << "]\n";
    return;
  }
  else if ((_msg.has_id() && _msg.id() != this->GetId()) &&
            _msg.name() != this->GetScopedName())
  {
    gzerr << "Incorrect name[" << _msg.name() << " != " << this->GetName()
      << "]\n";
    return;
  }

  this->SetName(this->world->StripWorldName(_msg.name()));
  if (_msg.has_pose())
    this->SetWorldPose(msgs::Convert(_msg.pose()));
  for (int i = 0; i < _msg.link_size(); i++)
  {
    LinkPtr link = this->GetLinkById(_msg.link(i).id());
    if (link)
      link->ProcessMsg(_msg.link(i));
  }

  if (_msg.has_is_static())
    this->SetStatic(_msg.is_static());
}

//////////////////////////////////////////////////
void Model::SetJointAnimation(
    const std::map<std::string, common::NumericAnimationPtr> _anims,
    boost::function<void()> _onComplete)
{
  this->updateMutex->lock();
  std::map<std::string, common::NumericAnimationPtr>::const_iterator iter;
  for (iter = _anims.begin(); iter != _anims.end(); ++iter)
  {
    this->jointAnimations[iter->first] = iter->second;
  }
  this->onJointAnimationComplete = _onComplete;
  this->prevAnimationTime = this->world->GetSimTime();
  this->updateMutex->unlock();
}

//////////////////////////////////////////////////
void Model::StopAnimation()
{
  this->updateMutex->lock();
  Entity::StopAnimation();
  this->onJointAnimationComplete.clear();
  this->jointAnimations.clear();
  this->updateMutex->unlock();
}

//////////////////////////////////////////////////
void Model::AttachStaticModel(ModelPtr &_model, math::Pose _offset)
{
  if (!_model->IsStatic())
  {
    gzerr << "AttachStaticModel requires a static model\n";
    return;
  }

  this->attachedModels.push_back(_model);
  this->attachedModelsOffset.push_back(_offset);
}

//////////////////////////////////////////////////
void Model::DetachStaticModel(const std::string &_modelName)
{
  for (unsigned int i = 0; i < this->attachedModels.size(); i++)
  {
    if (this->attachedModels[i]->GetName() == _modelName)
    {
      this->attachedModels.erase(this->attachedModels.begin()+i);
      this->attachedModelsOffset.erase(this->attachedModelsOffset.begin()+i);
      break;
    }
  }
}

//////////////////////////////////////////////////
void Model::OnPoseChange()
{
  math::Pose p;
  for (unsigned int i = 0; i < this->attachedModels.size(); i++)
  {
    p = this->GetWorldPose();
    p += this->attachedModelsOffset[i];
    this->attachedModels[i]->SetWorldPose(p, true);
  }
}

//////////////////////////////////////////////////
ModelState Model::GetState()
{
  return ModelState(boost::shared_static_cast<Model>(shared_from_this()));
}

//////////////////////////////////////////////////
void Model::SetState(const ModelState &_state)
{
  this->SetWorldPose(_state.GetPose(), true);

  for (unsigned int i = 0; i < _state.GetLinkStateCount(); ++i)
  {
    LinkState linkState = _state.GetLinkState(i);
    LinkPtr link = this->GetLink(linkState.GetName());
    if (link)
      link->SetState(linkState);
    else
      gzerr << "Unable to find link[" << linkState.GetName() << "]\n";
  }

  for (unsigned int i = 0; i < _state.GetJointStateCount(); ++i)
  {
    JointState jointState = _state.GetJointState(i);
    JointPtr joint = this->GetJoint(jointState.GetName());
    if (joint)
      joint->SetState(jointState);
    else
      gzerr << "Unable to find joint[" << jointState.GetName() << "]\n";
  }
}

/////////////////////////////////////////////////
void Model::SetEnabled(bool _enabled)
{
  Base_V::iterator iter;
  for (iter = this->children.begin(); iter != this->children.end(); ++iter)
    if (*iter && (*iter)->HasType(LINK))
      boost::static_pointer_cast<Link>(*iter)->SetEnabled(_enabled);
}
