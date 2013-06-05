/*
 * Copyright 2012 Open Source Robotics Foundation
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
#include "gazebo/transport/Publisher.hh"

#include "gazebo/common/Time.hh"

#include "gazebo/physics/World.hh"
#include "gazebo/physics/Collision.hh"
#include "gazebo/physics/Contact.hh"
#include "gazebo/physics/ContactManager.hh"

using namespace gazebo;
using namespace physics;

/////////////////////////////////////////////////
ContactManager::ContactManager()
{
  this->contactIndex = 0;
}

/////////////////////////////////////////////////
ContactManager::~ContactManager()
{
  this->Clear();
  this->node.reset();
  this->contactPub.reset();

  boost::unordered_map<std::string, ContactPublisher *>::iterator iter;
  for (iter = this->customContactPublishers.begin();
      iter != this->customContactPublishers.end(); ++iter)
  {
    if (iter->second)
    {
      iter->second->collisions.clear();
      iter->second->collisionNames.clear();
      iter->second->publisher.reset();
      delete iter->second;
      iter->second = NULL;
    }
  }
  this->customContactPublishers.clear();
}

/////////////////////////////////////////////////
void ContactManager::Init(WorldPtr _world)
{
  this->world = _world;

  this->node = transport::NodePtr(new transport::Node());
  this->node->Init(this->world->GetName());

  this->contactPub =
    this->node->Advertise<msgs::Contacts>("~/physics/contacts");
}

/////////////////////////////////////////////////
Contact *ContactManager::NewContact(Collision *_collision1,
                                    Collision *_collision2,
                                    const common::Time &_time)
{
  Contact *result = NULL;

  if (!_collision1 || !_collision2)
    return result;

  // If no one is listening to the default topic, or there are no
  // custom contact publishers then don't create any contact information.
  // This is a signal to the Physics engine that it can skip the extra
  // processing necessary to get back contact information.

  std::vector<ContactPublisher *> publishers;
  boost::unordered_map<std::string, ContactPublisher *>::iterator iter;
  for (iter = this->customContactPublishers.begin();
      iter != this->customContactPublishers.end(); ++iter)
  {
    // A model can simply be loaded later, so convert ones that are not yet
    // found
    if (!iter->second->collisionNames.empty())
    {
      std::vector<std::string>::iterator it;
      for (it = iter->second->collisionNames.begin();
          it != iter->second->collisionNames.end();)
      {
        Collision *col = boost::dynamic_pointer_cast<Collision>(
            this->world->GetByName(*it)).get();
        if (!col)
        {
          ++it;
          continue;
        }
        else
          it = iter->second->collisionNames.erase(it);
        iter->second->collisions.insert(col);
      }
    }

    if (iter->second->collisions.find(_collision1) !=
        iter->second->collisions.end() ||
        iter->second->collisions.find(_collision2) !=
        iter->second->collisions.end())
    {
      publishers.push_back(iter->second);
    }
  }

  if (this->contactPub->HasConnections() || !publishers.empty())
  {
    // Get or create a contact feedback object.
    if (this->contactIndex < this->contacts.size())
      result = this->contacts[this->contactIndex++];
    else
    {
      result = new Contact();
      this->contacts.push_back(result);
      this->contactIndex = this->contacts.size();
    }
    for (unsigned int i = 0; i < publishers.size(); ++i)
    {
      publishers[i]->contacts.push_back(result);
    }
  }

  if (!result)
    return result;

  result->count = 0;
  result->collision1 = _collision1->GetScopedName();
  result->collision2 = _collision2->GetScopedName();
  result->collisionPtr1 = _collision1;
  result->collisionPtr2 = _collision2;
  result->time = _time;
  result->world = this->world;

  return result;
}

/////////////////////////////////////////////////
unsigned int ContactManager::GetContactCount() const
{
  return this->contactIndex;
}

/////////////////////////////////////////////////
Contact *ContactManager::GetContact(unsigned int _index) const
{
  if (_index < this->contactIndex)
    return this->contacts[_index];
  else
    return NULL;
}

/////////////////////////////////////////////////
const std::vector<Contact*> &ContactManager::GetContacts() const
{
  return this->contacts;
}

/////////////////////////////////////////////////
void ContactManager::ResetCount()
{
  this->contactIndex = 0;
}

/////////////////////////////////////////////////
void ContactManager::Clear()
{
  // Delete all the contacts.
  for (unsigned int i = 0; i < this->contacts.size(); ++i)
    delete this->contacts[i];

  this->contacts.clear();

  boost::unordered_map<std::string, ContactPublisher *>::iterator iter;
  for (iter = this->customContactPublishers.begin();
      iter != this->customContactPublishers.end(); ++iter)
    iter->second->contacts.clear();

  // Reset the contact count to zero.
  this->contactIndex = 0;
}

/////////////////////////////////////////////////
void ContactManager::PublishContacts()
{
//  if (this->contacts.size() == 0)
//    return;

  if (!this->contactPub)
  {
    gzerr << "ContactManager has not been initialized. "
          << "Unable to publish contacts.\n";
    return;
  }

  // publish to default topic, ~/physics/contacts
  msgs::Contacts msg;
  for (unsigned int i = 0; i < this->contactIndex; ++i)
  {
    if (this->contacts[i]->count == 0)
      continue;

    msgs::Contact *contactMsg = msg.add_contact();
    this->contacts[i]->FillMsg(*contactMsg);
  }

  msgs::Set(msg.mutable_time(), this->world->GetSimTime());
  // this->contactPub->Publish(msg);

  // publish to other custom topics
  boost::unordered_map<std::string, ContactPublisher *>::iterator iter;
  for (iter = this->customContactPublishers.begin();
      iter != this->customContactPublishers.end(); ++iter)
  {
    ContactPublisher *contactPublisher = iter->second;
    msgs::Contacts msg2;
    for (unsigned int j = 0;
        j < contactPublisher->contacts.size(); ++j)
    {
      if (contactPublisher->contacts[j]->count == 0)
        continue;

      msgs::Contact *contactMsg = msg2.add_contact();
      contactPublisher->contacts[j]->FillMsg(*contactMsg);
    }
    msgs::Set(msg2.mutable_time(), this->world->GetSimTime());
    contactPublisher->publisher->Publish(msg2);
    contactPublisher->contacts.clear();
  }
}

/////////////////////////////////////////////////
std::string ContactManager::CreateFilter(const std::string &_name,
    const std::vector<std::string> &_collisions)
{
  if (_collisions.empty())
    return "";

  std::string name = _name;
  boost::replace_all(name, "::", "/");

  if (this->customContactPublishers.find(name) !=
    this->customContactPublishers.end())
  {
    gzerr << "Filter with the same name already exists! Aborting" << std::endl;
    return "";
  }

  // Contact sensors make use of this filter
  std::string topic = "~/" + name + "/contacts";

  transport::PublisherPtr pub =
    this->node->Advertise<msgs::Contacts>(topic);

  ContactPublisher *contactPublisher = new ContactPublisher;
  contactPublisher->publisher = pub;
  contactPublisher->collisionNames = _collisions;
  // convert collision names to pointers
  std::vector<std::string>::iterator iter;
  for (iter = contactPublisher->collisionNames.begin();
      iter != contactPublisher->collisionNames.end();)
  {
    Collision *col = boost::dynamic_pointer_cast<Collision>(
       this->world->GetByName(*iter)).get();
    if (!col)
    {
      ++iter;
      continue;
    }
    else
      iter = contactPublisher->collisionNames.erase(iter);
    contactPublisher->collisions.insert(col);
  }

  this->customContactPublishers[name] = contactPublisher;

  return topic;
}
