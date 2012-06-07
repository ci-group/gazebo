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

#include "SubscriptionTransport.hh"
#include "Publication.hh"
#include "Node.hh"

using namespace gazebo;
using namespace transport;

unsigned int Publication::idCounter = 0;

//////////////////////////////////////////////////
Publication::Publication(const std::string &_topic, const std::string &_msgType)
  : topic(_topic), msgType(_msgType), locallyAdvertised(false)
{
  this->id = idCounter++;
}

//////////////////////////////////////////////////
Publication::~Publication()
{
  this->publishers.clear();
}

//////////////////////////////////////////////////
void Publication::AddSubscription(const NodePtr &_node)
{
  std::list<NodePtr>::iterator iter;
  iter = std::find(this->nodes.begin(), this->nodes.end(), _node);
  if (iter == this->nodes.end())
  {
    this->nodes.push_back(_node);

    std::vector<PublisherPtr>::iterator pubIter;
    for (pubIter = this->publishers.begin(); pubIter != this->publishers.end();
         ++pubIter)
    {
      if ((*pubIter)->GetLatching() && !(*pubIter)->GetPrevMsg().empty())
      {
        _node->HandleData(this->topic, (*pubIter)->GetPrevMsg());
      }
    }
  }
}

//////////////////////////////////////////////////
void Publication::AddSubscription(const CallbackHelperPtr &_callback)
{
  std::list< CallbackHelperPtr >::iterator iter;
  iter = std::find(this->callbacks.begin(), this->callbacks.end(), _callback);

  if (iter == this->callbacks.end())
  {
    this->callbacks.push_back(_callback);

    std::vector<PublisherPtr>::iterator pubIter;
    for (pubIter = this->publishers.begin(); pubIter != this->publishers.end();
         ++pubIter)
    {
      if ((*pubIter)->GetLatching() || _callback->GetLatching())
      {
        _callback->HandleData((*pubIter)->GetPrevMsg());
      }
    }
  }
}

//////////////////////////////////////////////////
void Publication::AddTransport(const PublicationTransportPtr &_publink)
{
  bool add = true;

  // Find an existing publication transport
  std::list<PublicationTransportPtr>::iterator iter;
  for (iter = this->transports.begin(); iter != this->transports.end(); ++iter)
  {
    if ((*iter)->GetTopic() == _publink->GetTopic() &&
        (*iter)->GetMsgType() == _publink->GetMsgType() &&
        (*iter)->GetConnection()->GetRemoteURI() ==
        _publink->GetConnection()->GetRemoteURI())
    {
      add = false;
      break;
    }
  }

  // Don't add a duplicate transport
  if (add)
  {
    _publink->AddCallback(boost::bind(&Publication::LocalPublish, this, _1));
    this->transports.push_back(_publink);
  }
}

//////////////////////////////////////////////////
bool Publication::HasTransport(const std::string &_host, unsigned int _port)
{
  std::list<PublicationTransportPtr>::iterator iter;
  for (iter = this->transports.begin(); iter != this->transports.end(); ++iter)
  {
    if ((*iter)->GetConnection()->GetRemoteAddress() == _host &&
         (*iter)->GetConnection()->GetRemotePort() == _port)
    {
      return true;
    }
  }

  return false;
}

//////////////////////////////////////////////////
void Publication::RemoveTransport(const std::string &host_, unsigned int port_)
{
  std::list<PublicationTransportPtr>::iterator iter;
  iter = this->transports.begin();
  while (iter != this->transports.end())
  {
    if (!(*iter)->GetConnection()->IsOpen() ||
        ((*iter)->GetConnection()->GetRemoteAddress() == host_ &&
         (*iter)->GetConnection()->GetRemotePort() == port_))
    {
      (*iter)->Fini();
      this->transports.erase(iter++);
    }
    else
      ++iter;
  }
}

//////////////////////////////////////////////////
void Publication::RemoveSubscription(const NodePtr &_node)
{
  std::list<NodePtr>::iterator iter;

  for (iter = this->nodes.begin(); iter != this->nodes.end(); ++iter)
  {
    if ((*iter)->GetId() == _node->GetId())
    {
      this->nodes.erase(iter);
      break;
    }
  }

  // If no more subscribers, then disconnect from all publishers
  if (this->nodes.size() == 0 && this->callbacks.size() == 0)
  {
    this->transports.clear();
  }
}

//////////////////////////////////////////////////
void Publication::RemoveSubscription(const std::string &host, unsigned int port)
{
  SubscriptionTransportPtr subptr;
  std::list< CallbackHelperPtr >::iterator iter;

  iter = this->callbacks.begin();
  while (iter != this->callbacks.end())
  {
    subptr = boost::shared_dynamic_cast<SubscriptionTransport>(*iter);
    if (!subptr || !subptr->GetConnection()->IsOpen() ||
        (subptr->GetConnection()->GetRemoteAddress() == host &&
         subptr->GetConnection()->GetRemotePort() == port))
    {
      this->callbacks.erase(iter++);
    }
    else
      ++iter;
  }

  // If no more subscribers, then disconnect from all publishers
  if (this->callbacks.size() == 0)
  {
    this->transports.clear();
  }
}

//////////////////////////////////////////////////
void Publication::LocalPublish(const std::string &data)
{
  std::list<NodePtr>::iterator iter;
  iter = this->nodes.begin();

  while (iter != this->nodes.end())
  {
    if ((*iter)->HandleData(this->topic, data))
      ++iter;
    else
      iter = this->nodes.erase(iter);
  }

  std::list< CallbackHelperPtr >::iterator cbIter;
  cbIter = this->callbacks.begin();
  while (cbIter != this->callbacks.end())
  {
    if ((*cbIter)->IsLocal())
    {
      if ((*cbIter)->HandleData(data))
        ++cbIter;
      else
        cbIter = this->callbacks.erase(cbIter);
    }
    else
      ++cbIter;
  }
}

//////////////////////////////////////////////////
void Publication::Publish(const google::protobuf::Message &_msg,
                          const boost::function<void()> &_cb)
{
  std::string data;
  _msg.SerializeToString(&data);

  std::list<NodePtr>::iterator iter;
  iter = this->nodes.begin();
  while (iter != this->nodes.end())
  {
    if ((*iter)->HandleData(this->topic, data))
      ++iter;
    else
      this->nodes.erase(iter++);
  }

  std::list<CallbackHelperPtr>::iterator cbIter;
  cbIter = this->callbacks.begin();

  while (cbIter != this->callbacks.end())
  {
    if ((*cbIter)->HandleData(data))
      ++cbIter;
    else
      this->callbacks.erase(cbIter++);
  }

  if (_cb)
    (_cb)();
}

//////////////////////////////////////////////////
std::string Publication::GetMsgType() const
{
  return this->msgType;
}

//////////////////////////////////////////////////
unsigned int Publication::GetTransportCount() const
{
  return this->transports.size();
}

//////////////////////////////////////////////////
unsigned int Publication::GetCallbackCount() const
{
  return this->callbacks.size();
}

//////////////////////////////////////////////////
unsigned int Publication::GetNodeCount() const
{
  return this->nodes.size();
}

//////////////////////////////////////////////////
unsigned int Publication::GetRemoteSubscriptionCount()
{
  unsigned int count = 0;

  std::list< CallbackHelperPtr >::iterator iter;
  for (iter = this->callbacks.begin(); iter != this->callbacks.end(); ++iter)
  {
    if (!(*iter)->IsLocal())
      count++;
  }

  return count;
}

//////////////////////////////////////////////////
bool Publication::GetLocallyAdvertised() const
{
  return this->locallyAdvertised;
}

//////////////////////////////////////////////////
void Publication::SetLocallyAdvertised(bool _value)
{
  this->locallyAdvertised = _value;
}

//////////////////////////////////////////////////
void Publication::AddPublisher(PublisherPtr _pub)
{
  this->publishers.push_back(_pub);
}
