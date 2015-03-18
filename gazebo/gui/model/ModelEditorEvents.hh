/*
 * Copyright (C) 2013-2015 Open Source Robotics Foundation
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
#ifndef _MODEL_EDITOR_EVENTS_HH_
#define _MODEL_EDITOR_EVENTS_HH_

#include <string>
#include "gazebo/math/Pose.hh"
#include "gazebo/common/Event.hh"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace gui
  {
    namespace model
    {
      class GAZEBO_VISIBLE Events
      {
        /// \brief Connect a boost::slot to the finish model signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectFinishModel(T _subscriber)
          { return finishModel.Connect(_subscriber); }

        /// \brief Disconnect a boost::slot to the finish model signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectFinishModel(
            event::ConnectionPtr _subscriber)
          { finishModel.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the save signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectSaveModelEditor(T _subscriber)
          { return saveModelEditor.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the save signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectSaveModelEditor(
            event::ConnectionPtr _subscriber)
          { saveModelEditor.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the save as signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectSaveAsModelEditor(T _subscriber)
          { return saveAsModelEditor.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the save as signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectSaveAsModelEditor(
            event::ConnectionPtr _subscriber)
          { saveAsModelEditor.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the new signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectNewModelEditor(T _subscriber)
          { return newModelEditor.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the new signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectNewModelEditor(
              event::ConnectionPtr _subscriber)
          { newModelEditor.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the exit signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectExitModelEditor(T _subscriber)
          { return exitModelEditor.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the exit signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectExitModelEditor(
            event::ConnectionPtr _subscriber)
          { exitModelEditor.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the model changed signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectModelChanged(T _subscriber)
          { return modelChanged.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the model changed signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectModelChanged(
            event::ConnectionPtr _subscriber)
          { modelChanged.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the name changed signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectModelNameChanged(T _subscriber)
          { return modelNameChanged.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the name changed signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectModelNameChanged(
            event::ConnectionPtr _subscriber)
          { modelNameChanged.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the model properties changed
        /// signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr
            ConnectModelPropertiesChanged(T _subscriber)
          { return modelPropertiesChanged.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the model properties changed
        /// signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectModelPropertiesChanged(
            event::ConnectionPtr _subscriber)
          { modelPropertiesChanged.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the save model signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectSaveModel(T _subscriber)
          { return saveModel.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the save model signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectSaveModel(
            event::ConnectionPtr _subscriber)
          { saveModel.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the new model signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectNewModel(T _subscriber)
          { return newModel.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the new model signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectNewModel(
            event::ConnectionPtr _subscriber)
          { newModel.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the link inserted signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectLinkInserted(T _subscriber)
          { return linkInserted.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the link inserted signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectLinkInserted(
            event::ConnectionPtr _subscriber)
          { linkInserted.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the joint inserted signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectJointInserted(T _subscriber)
          { return jointInserted.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the joint inserted signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectJointInserted(
            event::ConnectionPtr _subscriber)
          { jointInserted.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the link removed signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectLinkRemoved(T _subscriber)
          { return linkRemoved.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the link removed signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectLinkRemoved(
            event::ConnectionPtr _subscriber)
          { linkRemoved.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the joint removed signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectJointRemoved(T _subscriber)
          { return jointRemoved.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the joint removed signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectJointRemoved(
            event::ConnectionPtr _subscriber)
          { jointRemoved.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the open link inspector signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectOpenLinkInspector(T _subscriber)
          { return openLinkInspector.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the open link inspector
        /// signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectOpenLinkInspector(
            event::ConnectionPtr _subscriber)
          { openLinkInspector.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the open joint inspector signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectOpenJointInspector(T _subscriber)
          { return openJointInspector.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the open joint inspector
        /// signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectOpenJointInspector(
            event::ConnectionPtr _subscriber)
          { openJointInspector.Disconnect(_subscriber); }

        /// \brief Connect a Gazebo event to the joint name changed signal.
        /// \param[in] _subscriber the subscriber to this event
        /// \return a connection
        public: template<typename T>
            static event::ConnectionPtr ConnectJointNameChanged(T _subscriber)
          { return jointNameChanged.Connect(_subscriber); }

        /// \brief Disconnect a Gazebo event from the joint name changed signal.
        /// \param[in] _subscriber the subscriber to this event
        public: static void DisconnectJointNameChanged(
            event::ConnectionPtr _subscriber)
          { jointNameChanged.Disconnect(_subscriber); }

        /// \brief A model has been completed and uploaded onto the server.
        public: static event::EventT<void ()> finishModel;

        /// \brief Request to save the model.
        public: static event::EventT<bool ()> saveModelEditor;

        /// \brief Request to save the model as.
        public: static event::EventT<bool ()> saveAsModelEditor;

        /// \brief Request to start a new model.
        public: static event::EventT<void ()> newModelEditor;

        /// \brief Request to exit the editor.
        public: static event::EventT<void ()> exitModelEditor;

        /// \brief Model has been changed.
        public: static event::EventT<void ()> modelChanged;

        /// \brief Name was changed in the editor palette.
        public: static event::EventT<void (std::string)> modelNameChanged;

        /// \brief Notify that model properties have been changed.
        // The properties are: is_static, auto_disable, pose, name
        public: static event::EventT<void (bool, bool, const math::Pose &,
            const std::string &)> modelPropertiesChanged;

        /// \brief Notify that model has been saved.
        public: static event::EventT<void (std::string)> saveModel;

        /// \brief Notify that model has been newed.
        public: static event::EventT<void ()> newModel;

        /// \brief Notify that a link has been inserted.
        public: static event::EventT<void (std::string)> linkInserted;

        /// \brief Notify that a joint has been inserted. The first
        /// string is the joint's unique id and the second string is the
        /// joint name.
        public: static event::EventT<void (std::string, std::string)>
            jointInserted;

        /// \brief Notify that a link has been removed.
        public: static event::EventT<void (std::string)> linkRemoved;

        /// \brief Nitify that a joint has been removed.
        public: static event::EventT<void (std::string)> jointRemoved;

        /// \brief Request to open the link inspector.
        public: static event::EventT<void (std::string)> openLinkInspector;

        /// \brief Request to open the joint inspector.
        public: static event::EventT<void (std::string)> openJointInspector;

        /// \brief Notify that the joint name has been changed. The first
        /// string is the joint's unique id and the second string is the
        /// new joint name.
        public: static event::EventT<void (std::string, std::string)>
            jointNameChanged;
      };
    }
  }
}
#endif
