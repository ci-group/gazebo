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
#include <google/protobuf/descriptor.h>
#include "transport/IOManager.hh"

#include "Master.hh"

#include "gazebo_config.h"

using namespace gazebo;

Master::Master()
  : connection(new transport::Connection())
{
  this->stop = false;
  this->runThread = NULL;

  this->connectionMutex = new boost::recursive_mutex();
  this->msgsMutex = new boost::recursive_mutex();
}

Master::~Master()
{
  this->Fini();

  delete this->connectionMutex;
  this->connectionMutex = NULL;

  delete this->msgsMutex;
  this->msgsMutex = NULL;

  delete this->runThread;
  this->runThread = NULL;

  this->publishers.clear();
  this->subscribers.clear();
  this->connections.clear();

  this->connection->Shutdown();
  delete this->connection;
  this->connection = NULL;
}

void Master::Init(uint16_t _port)
{
  try
  {
    this->connection->Listen(_port, boost::bind(&Master::OnAccept, this, _1));
  }
  catch(std::exception &e)
  {
    gzthrow("Unable to start server[" << e.what() << "]\n");
  }
}

//////////////////////////////////////////////////
void Master::OnAccept(const transport::ConnectionPtr &new_connection)
{
  // Send the gazebo version string
  msgs::String versionMsg;
  versionMsg.set_data(std::string("gazebo ") + GAZEBO_VERSION);
  new_connection->EnqueueMsg(msgs::Package("version_init", versionMsg), true);

  // Send all the current topic namespaces
  msgs::String_V namespacesMsg;
  std::list<std::string>::iterator iter;
  for (iter = this->worldNames.begin();
       iter != this->worldNames.end(); ++iter)
  {
    namespacesMsg.add_data(*iter);
  }
  new_connection->EnqueueMsg(msgs::Package("topic_namepaces_init",
                              namespacesMsg), true);

  // Send all the publishers
  msgs::Publishers publishersMsg;
  PubList::iterator pubiter;
  for (pubiter = this->publishers.begin();
       pubiter != this->publishers.end(); ++pubiter)
  {
    msgs::Publish *pub = publishersMsg.add_publisher();
    pub->CopyFrom(pubiter->first);
  }
  new_connection->EnqueueMsg(
      msgs::Package("publishers_init", publishersMsg), true);


  // Add the connection to our list
  this->connectionMutex->lock();
  int index = this->connections.size();

  this->connections[index] = new_connection;

  // Start reading from the connection
  new_connection->AsyncRead(
      boost::bind(&Master::OnRead, this, index, _1));
  this->connectionMutex->unlock();
}

//////////////////////////////////////////////////
void Master::OnRead(const unsigned int _connectionIndex,
                    const std::string &_data)
{
  if (this->stop)
    return;

  if (!this->connections[_connectionIndex] ||
      !this->connections[_connectionIndex]->IsOpen())
    return;

  // Get the connection
  transport::ConnectionPtr conn = this->connections[_connectionIndex];

  // Read the next message
  if (conn && conn->IsOpen())
    conn->AsyncRead(boost::bind(&Master::OnRead, this, _connectionIndex, _1));

  // Store the message if it's not empty
  if (!_data.empty())
  {
    this->msgsMutex->lock();
    this->msgs.push_back(std::make_pair(_connectionIndex, _data));
    this->msgsMutex->unlock();
  }
  else
  {
    gzerr << "Master got empty data message from["
          << conn->GetRemotePort() << "]\n";
  }
}

//////////////////////////////////////////////////
void Master::ProcessMessage(const unsigned int _connectionIndex,
                            const std::string &_data)
{
  if (!this->connections[_connectionIndex] ||
      !this->connections[_connectionIndex]->IsOpen())
    return;

  transport::ConnectionPtr conn = this->connections[_connectionIndex];

  msgs::Packet packet;
  packet.ParseFromString(_data);

  if (packet.type() == "register_topic_namespace")
  {
    msgs::String worldNameMsg;
    worldNameMsg.ParseFromString(packet.serialized_data());

    std::list<std::string>::iterator iter;
    iter = std::find(this->worldNames.begin(), this->worldNames.end(),
        worldNameMsg.data());
    if (iter == this->worldNames.end())
    {
      this->worldNames.push_back(worldNameMsg.data());

      this->connectionMutex->lock();
      Connection_M::iterator iter2;
      for (iter2 = this->connections.begin();
          iter2 != this->connections.end(); ++iter2)
      {
        iter2->second->EnqueueMsg(
            msgs::Package("topic_namespace_add", worldNameMsg));
      }
      this->connectionMutex->unlock();
    }
  }
  else if (packet.type() == "advertise")
  {
    msgs::Publish pub;
    pub.ParseFromString(packet.serialized_data());

    this->connectionMutex->lock();
    Connection_M::iterator iter2;
    for (iter2 = this->connections.begin();
         iter2 != this->connections.end(); ++iter2)
    {
      iter2->second->EnqueueMsg(msgs::Package("publisher_add", pub));
    }
    this->connectionMutex->unlock();

    this->publishers.push_back(std::make_pair(pub, conn));

    SubList::iterator iter;

    // Find all subscribers of the topic
    for (iter = this->subscribers.begin();
        iter != this->subscribers.end(); ++iter)
    {
      if (iter->first.topic() == pub.topic())
      {
        iter->second->EnqueueMsg(msgs::Package("publisher_update", pub));
      }
    }
  }
  else if (packet.type() == "unadvertise")
  {
    msgs::Publish pub;
    pub.ParseFromString(packet.serialized_data());
    this->RemovePublisher(pub);
  }
  else if (packet.type() == "unsubscribe")
  {
    msgs::Subscribe sub;
    sub.ParseFromString(packet.serialized_data());
    this->RemoveSubscriber(sub);
  }
  else if (packet.type() == "subscribe")
  {
    msgs::Subscribe sub;
    sub.ParseFromString(packet.serialized_data());

    this->subscribers.push_back(std::make_pair(sub, conn));

    PubList::iterator iter;

    // Find all publishers of the topic
    for (iter = this->publishers.begin();
        iter != this->publishers.end(); ++iter)
    {
      if (iter->first.topic() == sub.topic())
      {
        conn->EnqueueMsg(msgs::Package("publisher_update", iter->first));
      }
    }
  }
  else if (packet.type() == "request")
  {
    msgs::Request req;
    req.ParseFromString(packet.serialized_data());

    if (req.request() == "get_publishers")
    {
      msgs::Publishers msg;
      PubList::iterator iter;
      for (iter = this->publishers.begin();
          iter != this->publishers.end(); ++iter)
      {
        msgs::Publish *pub = msg.add_publisher();
        pub->CopyFrom(iter->first);
      }
      conn->EnqueueMsg(msgs::Package("publisher_list", msg), true);
    }
    else if (req.request() == "topic_info")
    {
      msgs::Publish pub = this->GetPublisher(req.data());
      msgs::TopicInfo ti;
      ti.set_msg_type(pub.msg_type());

      PubList::iterator piter;
      SubList::iterator siter;

      // Find all publishers of the topic
      for (piter = this->publishers.begin();
          piter != this->publishers.end(); ++piter)
      {
        if (piter->first.topic() == req.data())
        {
          msgs::Publish *pubPtr = ti.add_publisher();
          pubPtr->CopyFrom(piter->first);
        }
      }

      // Find all subscribers of the topic
      for (siter = this->subscribers.begin();
          siter != this->subscribers.end(); ++siter)
      {
        if (siter->first.topic() == req.data())
        {
          msgs::Subscribe *sub = ti.add_subscriber();
          sub->CopyFrom(siter->first);
        }
      }

      conn->EnqueueMsg(msgs::Package("topic_info_response", ti));
    }
    else if (req.request() == "get_topic_namespaces")
    {
      msgs::String_V msg;
      std::list<std::string>::iterator iter;
      for (iter = this->worldNames.begin();
          iter != this->worldNames.end(); ++iter)
      {
        msg.add_data(*iter);
      }
      conn->EnqueueMsg(msgs::Package("get_topic_namespaces_response", msg));
    }
    else
    {
      gzerr << "Unknown request[" << req.request() << "]\n";
    }
  }
  else
    std::cerr << "Master Unknown message type[" << packet.type()
              << "] From[" << conn->GetRemotePort() << "]\n";
}

//////////////////////////////////////////////////
void Master::Run()
{
  while (!this->stop)
  {
    this->RunOnce();
    common::Time::MSleep(10);
  }
}

//////////////////////////////////////////////////
void Master::RunThread()
{
  this->runThread = new boost::thread(boost::bind(&Master::Run, this));
}

//////////////////////////////////////////////////
void Master::RunOnce()
{
  Connection_M::iterator iter;

  // Process the incoming message queue
  this->msgsMutex->lock();
  while (this->msgs.size() > 0)
  {
    this->ProcessMessage(this->msgs.front().first,
        this->msgs.front().second);
    this->msgs.pop_front();
  }
  this->msgsMutex->unlock();

  // Process all the connections
  this->connectionMutex->lock();
  for (iter = this->connections.begin();
       iter != this->connections.end();)
  {
    if (iter->second->IsOpen())
    {
      iter->second->ProcessWriteQueue();
      ++iter;
    }
    else
    {
      this->RemoveConnection(iter->first);
      ++iter;
    }
  }
  this->connectionMutex->unlock();
}

void Master::RemoveConnection(unsigned int _index)
{
  std::list< std::pair<unsigned int, std::string> >::iterator msgIter;
  Connection_M::iterator connIter;
  connIter = this->connections.find(_index);

  if (connIter == this->connections.end() || !connIter->second)
    return;

  // Remove all messages for this connection
  this->msgsMutex->lock();
  msgIter = this->msgs.begin();
  while (msgIter != this->msgs.end())
  {
    if ((*msgIter).first == _index)
      this->msgs.erase(msgIter++);
    else
      ++msgIter;
  }
  this->msgsMutex->unlock();

  // Remove all publishers for this connection
  bool done = false;
  while (!done)
  {
    done = true;
    PubList::iterator pubIter = this->publishers.begin();
    while (pubIter != this->publishers.end())
    {
      if ((*pubIter).second->id ==
          connIter->second->id)
      {
        this->RemovePublisher((*pubIter).first);
        done = false;
        break;
      }
      else
        ++pubIter;
    }
  }


  done = false;
  while (!done)
  {
    done = true;

    // Remove all subscribers for this connection
    SubList::iterator subIter = this->subscribers.begin();
    while (subIter != this->subscribers.end())
    {
      if ((*subIter).second->id == connIter->second->id)
      {
        this->RemoveSubscriber((*subIter).first);
        done = false;
        break;
      }
      else
        ++subIter;
    }
  }

  this->connections.erase(connIter);
}

void Master::RemovePublisher(const msgs::Publish _pub)
{
  this->connectionMutex->lock();
  Connection_M::iterator iter2;
  for (iter2 = this->connections.begin();
      iter2 != this->connections.end(); ++iter2)
  {
    iter2->second->EnqueueMsg(msgs::Package("publisher_del", _pub));
  }
  this->connectionMutex->unlock();

  SubList::iterator iter;
  // Find all subscribers of the topic
  for (iter = this->subscribers.begin();
      iter != this->subscribers.end(); ++iter)
  {
    if (iter->first.topic() == _pub.topic())
    {
      iter->second->EnqueueMsg(msgs::Package("unadvertise", _pub));
    }
  }

  PubList::iterator pubIter = this->publishers.begin();
  while (pubIter != this->publishers.end())
  {
    if (pubIter->first.topic() == _pub.topic() &&
        pubIter->first.host() == _pub.host() &&
        pubIter->first.port() == _pub.port())
    {
      pubIter = this->publishers.erase(pubIter);
    }
    else
      ++pubIter;
  }
}

void Master::RemoveSubscriber(const msgs::Subscribe _sub)
{
  // Find all publishers of the topic, and remove the subscriptions
  for (PubList::iterator iter = this->publishers.begin();
      iter != this->publishers.end(); ++iter)
  {
    if (iter->first.topic() == _sub.topic())
    {
      iter->second->EnqueueMsg(msgs::Package("unsubscribe", _sub));
    }
  }

  // Remove the subscribers from our list
  SubList::iterator subiter = this->subscribers.begin();
  while (subiter != this->subscribers.end())
  {
    if (subiter->first.topic() == _sub.topic() &&
        subiter->first.host() == _sub.host() &&
        subiter->first.port() == _sub.port())
    {
      subiter = this->subscribers.erase(subiter);
    }
    else
      ++subiter;
  }
}

//////////////////////////////////////////////////
void Master::Stop()
{
  this->stop = true;

  if (this->runThread)
  {
    this->runThread->join();
    delete this->runThread;
    this->runThread = NULL;
  }
}

//////////////////////////////////////////////////
void Master::Fini()
{
  this->Stop();
}

//////////////////////////////////////////////////
msgs::Publish Master::GetPublisher(const std::string &_topic)
{
  msgs::Publish msg;

  PubList::iterator iter;

  // Find all publishers of the topic
  for (iter = this->publishers.begin();
       iter != this->publishers.end(); ++iter)
  {
    if (iter->first.topic() == _topic)
    {
      msg = iter->first;
      break;
    }
  }

  return msg;
}

//////////////////////////////////////////////////
transport::ConnectionPtr Master::FindConnection(const std::string &_host,
                                                uint16_t _port)
{
  transport::ConnectionPtr conn;
  Connection_M::iterator iter;

  this->connectionMutex->lock();
  for (iter = this->connections.begin();
       iter != this->connections.end(); ++iter)
  {
    if (iter->second->GetRemoteAddress() == _host &&
        iter->second->GetRemotePort() == _port)
    {
      conn = iter->second;
      break;
    }
  }
  this->connectionMutex->unlock();

  return conn;
}
