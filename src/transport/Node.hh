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

#ifndef NODE_HH
#define NODE_HH

#include <boost/enable_shared_from_this.hpp>
#include <map>
#include <list>
#include <string>
#include <vector>

#include "transport/TransportTypes.hh"
#include "transport/TopicManager.hh"

namespace gazebo
{
  namespace transport
  {
    /// \addtogroup gazebo_transport
    /// \{
    /// \brief A node can advertise and subscribe topics, publish on
    ///        advertised topics and listen to subscribed topics.
    class Node : public boost::enable_shared_from_this<Node>
    {
      /// \brief Constructor
      public: Node();

      /// \brief Destructor
      public: virtual ~Node();

      /// \brief Init the node
      /// \param space Set the global namespace of all topics. If left
      ///              blank, the topic will initialize to the first
      ///              namespace on the Master
      public: void Init(const std::string &_space ="");

      public: void Fini();

      /// \brief Get the topic namespace for this node
      /// \return The namespace
      public: std::string GetTopicNamespace() const;

      /// \brief Decode a topic name
      public: std::string DecodeTopicName(const std::string &topic);

      /// \brief Encode a topic name
      public: std::string EncodeTopicName(const std::string &topic);

      /// \brief Get the unique ID of the node
      public: unsigned int GetId() const;

      /// \brief Process all publishers, which has each publisher send it's
      /// most recent message over the wire. This is for internal use only
      public: void ProcessPublishers();
      public: void ProcessIncoming();

      /// \brief Adverise a topic
      template<typename M>
      transport::PublisherPtr Advertise(const std::string &topic,
                                        unsigned int _queueLimit = 10,
                                        bool _latch = false)
      {
        std::string decodedTopic = this->DecodeTopicName(topic);
        PublisherPtr publisher =
          transport::TopicManager::Instance()->Advertise<M>(
              decodedTopic, _queueLimit, _latch);

        this->publisherMutex->lock();
        this->publishers.push_back(publisher);
        this->publishersEnd = this->publishers.end();
        this->publisherMutex->unlock();

        return publisher;
      }

      /// \brief Subscribe to a topic, and return data on the callback
      template<typename M, typename T>
      SubscriberPtr Subscribe(const std::string &topic,
          void(T::*fp)(const boost::shared_ptr<M const> &), T *obj,
          bool _latching = false)
      {
        SubscribeOptions ops;
        std::string decodedTopic = this->DecodeTopicName(topic);
        ops.template Init<M>(decodedTopic, shared_from_this(), _latching);

        this->incomingMutex->lock();
        this->callbacks[decodedTopic].push_back(CallbackHelperPtr(
              new CallbackHelperT<M>(boost::bind(fp, obj, _1))));
        this->incomingMutex->unlock();

        return transport::TopicManager::Instance()->Subscribe(ops);
      }

      /// \brief Subscribe to a topic, and return data on the callback
      template<typename M>
      SubscriberPtr Subscribe(const std::string &topic,
          void(*fp)(const boost::shared_ptr<M const> &), bool _latching = false)
      {
        SubscribeOptions ops;
        std::string decodedTopic = this->DecodeTopicName(topic);
        ops.template Init<M>(decodedTopic, shared_from_this(), _latching);

        this->incomingMutex->lock();
        this->callbacks[decodedTopic].push_back(
            CallbackHelperPtr(new CallbackHelperT<M>(fp)));
        this->incomingMutex->unlock();

        return transport::TopicManager::Instance()->Subscribe(ops);
      }

      public: bool HandleData(const std::string &_topic,
                              const std::string &_msg);

      /// \brief Get the message type for a topic
      public: std::string GetMsgType(const std::string &_topic) const;

      private: std::string topicNamespace;
      private: std::vector<PublisherPtr> publishers;
      private: std::vector<PublisherPtr>::iterator publishersIter;
      private: std::vector<PublisherPtr>::iterator publishersEnd;
      private: static unsigned int idCounter;
      private: unsigned int id;

      private: typedef std::list<CallbackHelperPtr> Callback_L;
      private: typedef std::map<std::string, Callback_L> Callback_M;
      private: Callback_M callbacks;
      private: std::map<std::string, std::list<std::string> > incomingMsgs;
      private: boost::recursive_mutex *publisherMutex;
      private: boost::recursive_mutex *incomingMutex;
    };
    /// \}
  }
}
#endif


