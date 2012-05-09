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

/* Desc: Base class shared by all classes in Gazebo.
 * Author: Nate Koenig
 * Date: 09 Sept. 2008
 */


#include "common/Console.hh"
#include "common/Exception.hh"
#include "physics/World.hh"
#include "physics/Base.hh"

using namespace gazebo;
using namespace physics;

unsigned int Base::idCounter = 0;

//////////////////////////////////////////////////
Base::Base(BasePtr _parent)
: parent(_parent)
{
  this->type = BASE;
  this->id = ++idCounter;
  this->saveable = true;
  this->selected = false;

  this->sdf.reset(new sdf::Element);
  this->sdf->AddAttribute("name", "string", "__default__", true);

  if (this->parent)
  {
    this->world = this->parent->GetWorld();
  }
}

//////////////////////////////////////////////////
Base::~Base()
{
  // remove self as a child of the parent
  if (this->parent)
    this->parent->RemoveChild(this->id);

  this->SetParent(BasePtr());

  for (Base_V::iterator iter = this->children.begin();
       iter != this->childrenEnd; ++iter)
  {
    if (*iter)
      (*iter)->SetParent(BasePtr());
  }
  this->children.clear();
  this->childrenEnd = this->children.end();
  this->sdf.reset();
}

//////////////////////////////////////////////////
void Base::Load(sdf::ElementPtr _sdf)
{
  this->sdf = _sdf;
  if (this->parent)
  {
    this->world = this->parent->GetWorld();
    this->parent->AddChild(shared_from_this());
  }
}

//////////////////////////////////////////////////
void Base::UpdateParameters(sdf::ElementPtr _sdf)
{
  this->sdf->Copy(_sdf);
}

//////////////////////////////////////////////////
void Base::Fini()
{
  Base_V::iterator iter;

  for (iter = this->children.begin(); iter != this->childrenEnd; ++iter)
    (*iter)->Fini();

  this->children.clear();
  this->childrenEnd = this->children.end();

  this->world.reset();
  this->parent.reset();
}

//////////////////////////////////////////////////
void Base::Reset()
{
  Base_V::iterator iter;
  for (iter = this->children.begin(); iter != this->childrenEnd; ++iter)
  {
    (*iter)->Reset();
  }
}

//////////////////////////////////////////////////
void Base::SetName(const std::string &_name)
{
  this->sdf->GetAttribute("name")->Set(_name);
}

//////////////////////////////////////////////////
std::string Base::GetName() const
{
  if (this->sdf->HasAttribute("name"))
    return this->sdf->GetValueString("name");
  else
    return std::string();
}

//////////////////////////////////////////////////
unsigned int Base::GetId() const
{
  return this->id;
}

//////////////////////////////////////////////////
void Base::SetSaveable(bool _v)
{
  this->saveable = _v;
}

//////////////////////////////////////////////////
bool Base::GetSaveable() const
{
  return this->saveable;
}

//////////////////////////////////////////////////
int Base::GetParentId() const
{
  return this->parent == NULL ? 0 : this->parent->GetId();
}

//////////////////////////////////////////////////
void Base::SetParent(BasePtr _parent)
{
  this->parent = _parent;
}

//////////////////////////////////////////////////
BasePtr Base::GetParent() const
{
  return this->parent;
}

//////////////////////////////////////////////////
void Base::AddChild(BasePtr _child)
{
  if (_child == NULL)
    gzthrow("Cannot add a null _child to an entity");

  // Add this _child to our list
  this->children.push_back(_child);
  this->childrenEnd = this->children.end();
}

//////////////////////////////////////////////////
void Base::RemoveChild(unsigned int _id)
{
  Base_V::iterator iter;
  for (iter = this->children.begin(); iter != this->childrenEnd; ++iter)
  {
    if ((*iter)->GetId() == _id)
    {
      (*iter)->Fini();
      this->children.erase(iter);
      break;
    }
  }
  this->childrenEnd = this->children.end();
}

//////////////////////////////////////////////////
unsigned int Base::GetChildCount() const
{
  return this->children.size();
}

//////////////////////////////////////////////////
void Base::AddType(Base::EntityType _t)
{
  this->type = this->type | (unsigned int)_t;
}

//////////////////////////////////////////////////
BasePtr Base::GetChild(unsigned int _i) const
{
  if (_i < this->children.size())
    return this->children[_i];

  return BasePtr();
}

//////////////////////////////////////////////////
BasePtr Base::GetChild(const std::string &_name)
{
  std::string fullName = this->GetScopedName() + "::" + _name;
  return this->GetByName(fullName);
}

//////////////////////////////////////////////////
void Base::RemoveChild(const std::string &_name)
{
  Base_V::iterator iter;

  for (iter = this->children.begin();
       iter != this->childrenEnd; ++iter)
  {
    if ((*iter)->GetScopedName() == _name)
      break;
  }

  if (iter != this->children.end())
  {
    (*iter)->Fini();
    this->children.erase(iter);
  }

  this->childrenEnd = this->children.end();
}

//////////////////////////////////////////////////
void Base::RemoveChildren()
{
  this->children.clear();
  this->childrenEnd = this->children.end();
}

//////////////////////////////////////////////////
BasePtr Base::GetById(unsigned int _id) const
{
  BasePtr result;
  Base_V::const_iterator biter;

  for (biter = this->children.begin(); biter != this->children.end(); ++biter)
  {
    if ((*biter)->GetId() == _id)
    {
      result = *biter;
      break;
    }
  }

  return result;
}

//////////////////////////////////////////////////
BasePtr Base::GetByName(const std::string &_name)
{
  if (this->GetScopedName() == _name || this->GetName() == _name)
    return shared_from_this();

  BasePtr result;
  Base_V::const_iterator iter;
  Base_V::const_iterator iterEnd = this->childrenEnd;

  for (iter =  this->children.begin();
       iter != iterEnd && result == NULL; ++iter)
    result = (*iter)->GetByName(_name);

  return result;
}

//////////////////////////////////////////////////
std::string Base::GetScopedName() const
{
  BasePtr p = this->parent;
  std::string scopedName = this->GetName();

  while (p)
  {
    if (p->GetParent())
      scopedName.insert(0, p->GetName()+"::");
    p = p->GetParent();
  }

  return scopedName;
}

//////////////////////////////////////////////////
std::string Base::GetCompleteScopedName() const
{
  BasePtr p = this->parent;
  std::string scopedName = this->GetName();

  while (p)
  {
    scopedName.insert(0, p->GetName()+"::");
    p = p->GetParent();
  }

  return scopedName;
}

//////////////////////////////////////////////////
bool Base::HasType(const Base::EntityType &_t) const
{
  return ((unsigned int)(_t & this->type) == (unsigned int)_t);
}

//////////////////////////////////////////////////
unsigned int Base::GetType() const
{
  return this->type;
}

//////////////////////////////////////////////////
void Base::Print(std::string _prefix)
{
  Base_V::iterator iter;
  gzmsg << _prefix << this->GetName() << "\n";

  _prefix += "  ";
  for (iter = this->children.begin(); iter != this->childrenEnd; ++iter)
    (*iter)->Print(_prefix);
}

//////////////////////////////////////////////////
bool Base::SetSelected(bool _s)
{
  this->selected = _s;

  Base_V::iterator iter;
  for (iter = this->children.begin(); iter != this->childrenEnd; ++iter)
    (*iter)->SetSelected(_s);

  return true;
}

//////////////////////////////////////////////////
bool Base::IsSelected() const
{
  return this->selected;
}

//////////////////////////////////////////////////
bool Base::operator ==(const Base &ent) const
{
  return ent.GetName() == this->GetName();
}

//////////////////////////////////////////////////
void Base::SetWorld(const WorldPtr &_newWorld)
{
  this->world = _newWorld;

  Base_V::iterator iter;
  for (iter = this->children.begin(); iter != this->childrenEnd; ++iter)
  {
    (*iter)->SetWorld(this->world);
  }
}

//////////////////////////////////////////////////
const WorldPtr &Base::GetWorld() const
{
  return this->world;
}

//////////////////////////////////////////////////
const sdf::ElementPtr Base::GetSDF()
{
  this->sdf->Update();
  return this->sdf;
}


