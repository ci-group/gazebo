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
#ifndef _CALLBACKHELPER_HH_
#define _CALLBACKHELPER_HH_

#include <google/protobuf/message.h>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <vector>
#include <string>

#include "common/Console.hh"
#include "msgs/msgs.hh"
#include "common/Exception.hh"

namespace gazebo
{
  namespace transport
  {
    /// \addtogroup gazebo_transport Transport
    /// \{

    /// \class CallbackHelper CallbackHelper.hh transport/transport.hh
    /// \brief A helper class to handle callbacks when messages arrive
    class CallbackHelper
    {
      /// \brief Constructor
      public: CallbackHelper() : latching(false) {}
      /// \brief Destructor
      public: virtual ~CallbackHelper() {}

      /// \brief Get the typename of the message that is handled
      /// \return String representation of the message type
      public: virtual std::string GetMsgType() const
              {
                return std::string();
              }

      /// \brief Process new incoming data
      /// \param[in] _newdata Incoming data to be processed
      /// \return true if successfully processed; false otherwise
      public: virtual bool HandleData(const std::string &_newdata) = 0;

      /// \brief Is the callback local?
      /// \return true if the callback is local, false if the callback
      ///         is tied to a remote connection
      public: virtual bool IsLocal() const = 0;

      /// \brief Is the callback latching?
      /// \return true if the callback is latching, false otherwise
      public: bool GetLatching() const
              {return this->latching;}
      protected: bool latching;
    };

    /// \brief boost shared pointer to transport::CallbackHelper
    typedef boost::shared_ptr<CallbackHelper> CallbackHelperPtr;


    /// \class CallbackHelperT CallbackHelper.hh transport/transport.hh
    /// \brief Callback helper Template
    template<class M>
    class CallbackHelperT : public CallbackHelper
    {
      /// \brief Constructor
      /// \param[in] _cb boost function to call on incoming messages
      public: CallbackHelperT(const boost::function<
                void (const boost::shared_ptr<M const> &)> &_cb) : callback(_cb)
              {
                // Just some code to make sure we have a google protobuf.
                /*M test;
                google::protobuf::Message *m;
                if ((m =dynamic_cast<google::protobuf::Message*>(&test))
                    == NULL)
                  gzthrow("Message type must be a google::protobuf type\n");
                  */
              }

      // documentation inherited
      public: std::string GetMsgType() const
              {
                M test;
                google::protobuf::Message *m;
                if ((m = dynamic_cast<google::protobuf::Message*>(&test))
                    == NULL)
                  gzthrow("Message type must be a google::protobuf type\n");
                return m->GetTypeName();
              }

      // documentation inherited
      public: virtual bool HandleData(const std::string &_newdata)
              {
                boost::shared_ptr<M> m(new M);
                m->ParseFromString(_newdata);
                this->callback(m);
                return true;
              }

      // documentation inherited
      public: virtual bool IsLocal() const
              {
                return true;
              }

      private: boost::function<void (const boost::shared_ptr<M const> &)>
               callback;
    };

    /// \class DebugCallbackHelper CallbackHelper.hh transport/transport.hh
    /// \brief CallbackHelper subclass with debug facilities
    class DebugCallbackHelper : public CallbackHelper
    {
      /// \brief Constructor
      /// \param[in] _cb boost function to call on incoming messages
      public: DebugCallbackHelper(
                  const boost::function<void (ConstGzStringPtr &)> &_cb)
              : callback(_cb)
              {
              }

      // documentation inherited
      public: std::string GetMsgType() const
              {
                msgs::GzString m;
                return m.GetTypeName();
              }

      // documentation inherited
      public: virtual bool HandleData(const std::string &_newdata)
              {
                msgs::Packet packet;
                packet.ParseFromString(_newdata);

                boost::shared_ptr<msgs::GzString> m(new msgs::GzString);
                m->ParseFromString(_newdata);
                this->callback(m);
                return true;
              }

      // documentation inherited
      public: virtual bool IsLocal() const
              {
                return true;
              }

      private: boost::function<void (boost::shared_ptr<msgs::GzString> &)>
               callback;
    };
    /// \}
  }
}
#endif
