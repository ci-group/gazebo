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


#include "msgs/msgs.hh"
#include "transport/Node.hh"
#include "transport/Publication.hh"
#include "transport/TopicManager.hh"

using namespace gazebo;
using namespace transport;

//////////////////////////////////////////////////
TopicManager::TopicManager()
{
  this->pauseIncoming = false;
  this->advertisedTopicsEnd = this->advertisedTopics.end();
}

//////////////////////////////////////////////////
TopicManager::~TopicManager()
{
}

//////////////////////////////////////////////////
void TopicManager::Init()
{
  this->advertisedTopics.clear();
  this->advertisedTopicsEnd = this->advertisedTopics.end();
  this->subscribedNodes.clear();
  this->nodes.clear();
}

//////////////////////////////////////////////////
void TopicManager::Fini()
{
  // These two lines make sure that pending messages get sent out
  this->ProcessNodes(true);
  ConnectionManager::Instance()->RunUpdate();

  PublicationPtr_M::iterator iter;
  for (iter = this->advertisedTopics.begin();
       iter != this->advertisedTopics.end(); ++iter)
  {
    this->Unadvertise(iter->first);
  }

  this->advertisedTopics.clear();
  this->advertisedTopicsEnd = this->advertisedTopics.end();
  this->subscribedNodes.clear();
  this->nodes.clear();
}

//////////////////////////////////////////////////
void TopicManager::AddNode(NodePtr _node)
{
  boost::recursive_mutex::scoped_lock lock(this->nodeMutex);
  this->nodes.push_back(_node);
}

//////////////////////////////////////////////////
void TopicManager::RemoveNode(unsigned int _id)
{
  std::vector<NodePtr>::iterator iter;
  boost::recursive_mutex::scoped_lock lock(this->nodeMutex);

  for (iter = this->nodes.begin(); iter != this->nodes.end(); ++iter)
  {
    if ((*iter)->GetId() == _id)
    {
      // Remove the node from all publications.
      for (PublicationPtr_M::iterator piter = this->advertisedTopics.begin();
           piter != this->advertisedTopics.end(); ++piter)
      {
        piter->second->RemoveSubscription(*iter);
      }

      // Remove the node from all subscriptions.
      for (SubNodeMap::iterator siter = this->subscribedNodes.begin();
           siter != this->subscribedNodes.end(); ++siter)
      {
        std::list<NodePtr>::iterator subIter = siter->second.begin();
        while (subIter != siter->second.end())
        {
          if (*subIter == *iter)
            siter->second.erase(subIter++);
          else
            ++subIter;
        }
      }

      this->nodes.erase(iter);
      break;
    }
  }
}

//////////////////////////////////////////////////
void TopicManager::ProcessNodes(bool _onlyOut)
{
  std::vector<NodePtr>::iterator iter;
  int s;

  {
    boost::recursive_mutex::scoped_lock lock(this->nodeMutex);
    // store as size might change (spawning)
    s = this->nodes.size();
  }

  for (int i = 0; i < s; i ++)
  {
    this->nodes[i]->ProcessPublishers();
  }

  if (!this->pauseIncoming && !_onlyOut)
  {
    {
      boost::recursive_mutex::scoped_lock lock(this->nodeMutex);
      s = this->nodes.size();
    }

    for (int i = 0; i < s; i ++)
    {
      this->nodes[i]->ProcessIncoming();
      if (this->pauseIncoming)
        break;
    }
  }
}

//////////////////////////////////////////////////
void TopicManager::Publish(const std::string &_topic,
                           const google::protobuf::Message &_message,
                           const boost::function<void()> &_cb)
{
  PublicationPtr pub = this->FindPublication(_topic);
  PublicationPtr dbgPub = this->FindPublication(_topic+"/__dbg");

  if (pub)
    pub->Publish(_message, _cb);

  if (dbgPub && dbgPub->GetCallbackCount() > 0)
  {
    msgs::GzString dbgMsg;
    dbgMsg.set_data(_message.DebugString());
    dbgPub->Publish(dbgMsg);
  }
}

//////////////////////////////////////////////////
PublicationPtr TopicManager::FindPublication(const std::string &_topic)
{
  PublicationPtr_M::iterator iter = this->advertisedTopics.find(_topic);
  if (iter != this->advertisedTopicsEnd)
    return iter->second;
  else
    return PublicationPtr();
}

//////////////////////////////////////////////////
SubscriberPtr TopicManager::Subscribe(const SubscribeOptions &_ops)
{
  // Create a subscription (essentially a callback that gets
  // fired every time a Publish occurs on the corresponding
  // topic
  this->subscribedNodes[_ops.GetTopic()].push_back(_ops.GetNode());

  // The object that gets returned to the caller of this
  // function
  SubscriberPtr sub(new Subscriber(_ops.GetTopic(), _ops.GetNode()));

  // Find a current publication
  PublicationPtr pub = this->FindPublication(_ops.GetTopic());

  // If the publication exits, just add the subscription to it
  if (pub)
    pub->AddSubscription(_ops.GetNode());

  // Use this to find other remote publishers
  ConnectionManager::Instance()->Subscribe(_ops.GetTopic(), _ops.GetMsgType(),
                                           _ops.GetLatching());
  return sub;
}

//////////////////////////////////////////////////
void TopicManager::Unsubscribe(const std::string &_topic,
                               const NodePtr &_node)
{
  boost::mutex::scoped_lock lock(this->subscriberMutex);

  PublicationPtr publication = this->FindPublication(_topic);

  if (publication)
    publication->RemoveSubscription(_node);

  std::cout << "Unsub Topic[" << _topic << "]\n";
  ConnectionManager::Instance()->Unsubscribe(_topic,
      _node->GetMsgType(_topic));

  this->subscribedNodes[_topic].remove(_node);
}

//////////////////////////////////////////////////
void TopicManager::ConnectPubToSub(const std::string &topic,
                                   const SubscriptionTransportPtr _sublink)
{
  PublicationPtr publication = this->FindPublication(topic);
  publication->AddSubscription(sublink);
}

//////////////////////////////////////////////////
void TopicManager::DisconnectPubFromSub(const std::string &topic,
    const std::string &host, unsigned int port)
{
  PublicationPtr publication = this->FindPublication(topic);
  publication->RemoveSubscription(host, port);
}

//////////////////////////////////////////////////
void TopicManager::DisconnectSubFromPub(const std::string &topic,
    const std::string &host, unsigned int port)
{
  PublicationPtr publication = this->FindPublication(topic);
  if (publication)
    publication->RemoveTransport(host, port);
}

//////////////////////////////////////////////////
void TopicManager::ConnectSubscribers(const std::string &_topic)
{
  SubNodeMap::iterator nodeIter = this->subscribedNodes.find(_topic);

  if (nodeIter != this->subscribedNodes.end())
  {
    PublicationPtr publication = this->FindPublication(_topic);
    if (!publication)
      return;

    // Add all of our subscriptions to the publication
    std::list<NodePtr>::iterator cbIter;
    for (cbIter = nodeIter->second.begin();
         cbIter != nodeIter->second.end(); ++cbIter)
    {
      publication->AddSubscription(*cbIter);
    }
  }
  else
  {
    // TODO: Properly handle this error
    gzerr << "Shouldn't get here topic[" << _topic << "]\n";
  }
}

//////////////////////////////////////////////////
void TopicManager::ConnectSubToPub(const msgs::Publish &_pub)
{
  boost::mutex::scoped_lock lock(this->subscriberMutex);
  this->UpdatePublications(_pub.topic(), _pub.msg_type());

  PublicationPtr publication = this->FindPublication(_pub.topic());

  if (publication && !publication->HasTransport(_pub.host(), _pub.port()))
  {
    std::cout << "ConnectSubToPub Topic[" << _pub.topic() << "]\n";
    // Connect to the remote publisher
    ConnectionPtr conn = ConnectionManager::Instance()->ConnectToRemoteHost(
        _pub.host(), _pub.port());

    if (conn)
    {
      // Create a transport link that will read from the connection, and
      // send data to a Publication.
      PublicationTransportPtr publink(new PublicationTransport(_pub.topic(),
            _pub.msg_type()));

      bool latched = false;
      SubNodeMap::iterator nodeIter = this->subscribedNodes.find(_pub.topic());

      // Find if any local node has a latched subscriber for the new topic
      // publication transport.
      if (nodeIter != this->subscribedNodes.end())
      {
        std::list<NodePtr>::iterator cbIter;
        for (cbIter = nodeIter->second.begin();
             cbIter != nodeIter->second.end() && !latched; ++cbIter)
        {
          latched = (*cbIter)->HasLatchedSubscriber(_pub.topic());
        }
      }

      publink->Init(conn, latched);

      publication->AddTransport(publink);
    }
  }

  this->ConnectSubscribers(_pub.topic());
}


//////////////////////////////////////////////////
PublicationPtr TopicManager::UpdatePublications(const std::string &topic,
                                                const std::string &msgType)
{
  // Find a current publication on this topic
  PublicationPtr pub = this->FindPublication(topic);

  if (pub)
  {
    if (msgType != pub->GetMsgType())
      gzthrow("Attempting to advertise on an existing topic with"
              " a conflicting message type\n");
  }
  else
  {
    PublicationPtr dbgPub;
    msgs::GzString tmp;

    pub = PublicationPtr(new Publication(topic, msgType));
    dbgPub = PublicationPtr(new Publication(topic+"/__dbg",
          tmp.GetTypeName()));
    this->advertisedTopics[topic] =  pub;
    this->advertisedTopics[topic+"/__dbg"] = dbgPub;
    this->advertisedTopicsEnd = this->advertisedTopics.end();
  }

  return pub;
}

//////////////////////////////////////////////////
void TopicManager::Unadvertise(const std::string &_topic)
{
  std::string t;

  for (int i = 0; i < 2; i ++)
  {
    if (i == 0)
      t = _topic;
    else
      t = _topic + "/__dbg";

    PublicationPtr publication = this->FindPublication(t);
    if (publication && publication->GetLocallyAdvertised() &&
        publication->GetTransportCount() == 0)
    {
      publication->SetLocallyAdvertised(false);
      ConnectionManager::Instance()->Unadvertise(t);
    }
  }
}

//////////////////////////////////////////////////
void TopicManager::RegisterTopicNamespace(const std::string &_name)
{
  ConnectionManager::Instance()->RegisterTopicNamespace(_name);
}

//////////////////////////////////////////////////
void TopicManager::GetTopicNamespaces(std::list<std::string> &_namespaces)
{
  ConnectionManager::Instance()->GetTopicNamespaces(_namespaces);
}

//////////////////////////////////////////////////
std::map<std::string, std::list<std::string> >
TopicManager::GetAdvertisedTopics() const
{
  std::map<std::string, std::list<std::string> > result;
  std::list<msgs::Publish> publishers;

  ConnectionManager::Instance()->GetAllPublishers(publishers);

  for (std::list<msgs::Publish>::iterator iter = publishers.begin();
      iter != publishers.end(); ++iter)
  {
    result[(*iter).msg_type()].push_back((*iter).topic());
  }

  return result;
}

//////////////////////////////////////////////////
void TopicManager::ClearBuffers()
{
  PublicationPtr_M::iterator iter;
  for (iter = this->advertisedTopics.begin();
       iter != this->advertisedTopics.end(); ++iter)
  {
  }
}

//////////////////////////////////////////////////
void TopicManager::PauseIncoming(bool _pause)
{
  this->pauseIncoming = _pause;
}
