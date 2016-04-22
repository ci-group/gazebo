/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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

#ifndef GAZEBO_GUI_MODEL_MEUSERCMDMANAGER_HH_
#define GAZEBO_GUI_MODEL_MEUSERCMDMANAGER_HH_

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <sdf/sdf.hh>

#include "gazebo/gui/qt.h"
#include "gazebo/gui/UserCmdHistory.hh"
#include "gazebo/gui/model/ModelEditorTypes.hh"

#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace gui
  {
    class UserCmdHistory;

    // Forward declare private data.
    class MEUserCmdPrivate;
    class MEUserCmdManagerPrivate;

    /// \brief Class which represents a user command, which can be "undone"
    /// and "redone".
    class GZ_GUI_VISIBLE MEUserCmd
    {
      /// \enum CmdType
      /// \brief Types of user commands.
      public: enum CmdType
      {
        /// \brief Insert a link.
        INSERTING_LINK,

        /// \brief Delete a link.
        DELETING_LINK,

        /// \brief Insert a nested model.
        INSERTING_NESTED_MODEL,

        /// \brief Delete a nested model.
        DELETING_NESTED_MODEL,

        /// \brief Insert a joint.
        INSERTING_JOINT,

        /// \brief Delete a joint.
        DELETING_JOINT,

        /// \brief Move a link.
        MOVING_LINK,

        /// \brief Move a nested model.
        MOVING_NESTED_MODEL
      };

      /// \brief Constructor
      /// \param[in] _id Unique ID for this command
      /// \param[in] _description Description for the command, such as
      /// "Rotate box", "Delete sphere", etc.
      /// \param[in] _type Type of command, such as MOVING, DELETING, etc.
      public: MEUserCmd(const unsigned int _id, const std::string &_description,
          MEUserCmd::CmdType _type);

      /// \brief Destructor
      public: virtual ~MEUserCmd();

      /// \brief Undo this command.
      public: virtual void Undo();

      /// \brief Redo this command.
      public: virtual void Redo();

      /// \brief Return this command's unique ID.
      /// \return Unique ID
      public: unsigned int Id() const;

      /// \brief Return this command's description.
      /// \return Description
      public: std::string Description() const;

      /// \brief Set the SDF element relevant to this command.
      /// \param[in] _sdf SDF pointer.
      public: void SetSDF(sdf::ElementPtr _sdf);

      /// \brief Set the scoped name of the entity related to this command.
      /// \param[in] _name Fully scoped entity name.
      public: void SetScopedName(const std::string &_name);

      /// \brief Set the unique id of the joint related to this command.
      /// \param[in] _id Unique id of joint.
      public: void SetJointId(const std::string &_id);

      public: void SetPoseChange(const ignition::math::Pose3d &_before,
          const ignition::math::Pose3d &_after);

      /// \internal
      /// \brief Pointer to private data.
      protected: std::unique_ptr<MEUserCmdPrivate> dataPtr;
    };

    /// \brief Class which manages user commands in the model editor. It
    /// combines features of gui::UserCmdHistory and physics::UserCmdManager.
    class GZ_GUI_VISIBLE MEUserCmdManager : public UserCmdHistory
    {
      Q_OBJECT

      /// \brief Constructor
      public: MEUserCmdManager();

      /// \brief Destructor
      public: virtual ~MEUserCmdManager();

      /// \brief Reset commands.
      public: void Reset();

      /// \brief Register that a new command has been executed by the user.
      /// \param[in] _description Command description, to be displayed to the
      /// user.
      /// \param[in] _type Command type.
      /// \return Pointer to new command.
      public: MEUserCmdPtr NewCmd(const std::string &_description,
          const MEUserCmd::CmdType _type);

      /// \brief Qt callback to undo a command.
      /// \param[in] _action Action corresponding to the command.
      private slots: void OnUndoCommand(QAction *_action);

      /// \brief Qt callback to redo a command.
      /// \param[in] _action Action corresponding to the command.
      private slots: void OnRedoCommand(QAction *_action);

      /// \brief Whether there are commands for undo or not.
      /// \return True if there are.
      private: virtual bool HasUndo() const;

      /// \brief Whether there are commands for redo or not.
      /// \return True if there are.
      private: virtual bool HasRedo() const;

      /// \brief Get the list of commands.
      /// \return True if there are.
      private: virtual std::vector<std::pair<unsigned int, std::string>>
          Cmds(const bool _undo) const;

      /// \internal
      /// \brief Pointer to private data.
      private: std::unique_ptr<MEUserCmdManagerPrivate> dataPtr;
    };
  }
}
#endif


