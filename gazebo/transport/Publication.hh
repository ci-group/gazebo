/*
 * Copyright 2012 Nate Koenig
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

#ifndef _PUBLICATION_HH_
#define _PUBLICATION_HH_

#include <boost/shared_ptr.hpp>
#include <list>
#include <string>
#include <vector>

#include "transport/CallbackHelper.hh"
#include "transport/TransportTypes.hh"
#include "transport/PublicationTransport.hh"

namespace gazebo
{
  namespace transport
  {
    /// \addtogroup gazebo_transport
    /// \{

    /// \class Publication Publication.hh transport/transport.hh
    /// \brief A publication for a topic. This facilitates transport of
    /// messages
    class Publication
    {
      /// \brief Constructor
      /// \param[in] _topic The topic we're publishing
      /// \param[in] _msgType The type of the topic we're publishing
      public: Publication(const std::string &_topic,
                           const std::string &_msgType);

      /// \brief Destructor
      public: virtual ~Publication();

      /// \brief Get the type of message
      /// \return The type of message
      public: std::string GetMsgType() const;

      /// \brief Subscribe a callback to our topic
      /// \param[in] _callback The callback
      public: void AddSubscription(const CallbackHelperPtr &_callback);

      /// \brief Subscribe a node to our topic
      /// \param[in] _node The node
      public: void AddSubscription(const NodePtr &_node);

      /// \brief Unsubscribe a node from our topic
      /// \param[in] _node The node
      public: void RemoveSubscription(const NodePtr &_node);

      /// \brief Unsubscribe a a node by host/port from our topic
      /// \param[in] _host The node's hostname
      /// \param[in] _port The node's port
      public: void RemoveSubscription(const std::string &_host,
                                      unsigned int _port);

      /// \brief Remove a transport
      /// \param[in] _host The transport's hostname
      /// \param[in] _port The transport's port
      public: void RemoveTransport(const std::string &_host, unsigned
                                   int _port);

      /// \brief Get the number of transports
      /// \return The number of transports
      public: unsigned int GetTransportCount() const;

      /// \brief Get the number of callbacks
      /// \return The number of callbacks
      public: unsigned int GetCallbackCount() const;

      /// \brief Get the number of nodes
      /// \return The number of nodes
      public: unsigned int GetNodeCount() const;

      /// \brief Get the number of remote subscriptions
      /// \return The number of remote subscriptions
      public: unsigned int GetRemoteSubscriptionCount();

      /// \brief Was the topic has been advertised from this process?
      /// \return true if the topic has been advertised from this process,
      /// false otherwise
      public: bool GetLocallyAdvertised() const;

      /// \brief Set whether this topic has been advertised from this process
      /// \param[in] _value If true, the topic was locally advertise,
      /// otherwise it was not
      public: void SetLocallyAdvertised(bool _value);

      /// \brief Publish data to local subscribers (skip serialization)
      /// \param[in] _data The data to be published
      public: void LocalPublish(const std::string &_data);

      /// \brief Publish data to remote subscribers
      /// \param[in] _msg Message to be published
      /// \param[in] _cb If non-null, callback to be invoked after publishing
      /// is completed
      public: void Publish(const google::protobuf::Message &_msg,
                           const boost::function<void()> &_cb = NULL);

      /// \brief Add a transport
      /// \param[in] _publink Pointer to publication transport object to
      /// be added
      public: void AddTransport(const PublicationTransportPtr &_publink);

      /// \brief Does a given transport exist?
      /// \param[in] _host Hostname of the transport
      /// \param[in] _port Port of the transport
      /// \return true if the transport exists, false otherwise
      public: bool HasTransport(const std::string &_host, unsigned int _port);

      /// \brief Add a publisher
      /// \param[in,out] _pub Pointer to publisher object to be added
      public: void AddPublisher(PublisherPtr _pub);

      private: unsigned int id;
      private: static unsigned int idCounter;
      private: std::string topic;
      private: std::string msgType;

      /// \brief Remove nodes that receieve messages
      private: std::list<CallbackHelperPtr> callbacks;

      /// \brief Local nodes that recieve messages
      private: std::list<NodePtr> nodes;

      private: std::list<PublicationTransportPtr> transports;

      private: std::vector<PublisherPtr> publishers;

      private: bool locallyAdvertised;
    };
    /// \}
  }
}
#endif


