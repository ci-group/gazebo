/*
 * Copyright 2016 Open Source Robotics Foundation
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
#ifndef GAZEBO_GUI_SAVEDIALOG_HH_
#define GAZEBO_GUI_SAVEDIALOG_HH_

#include <string>
#include <memory>
#include "gazebo/gui/qt.h"

namespace gazebo
{
  namespace gui
  {
    class SaveDialogPrivate;

    /// \addtogroup gazebo_gui
    /// \{

    /// \class SaveVideoDialog SaveVideoDialog.hh
    /// \brief Dialog for saving to file.
    class SaveDialog : public QDialog
    {
      Q_OBJECT

      /// \brief Constructor
      /// \param[in] _parent Parent QWidget.
      public: SaveDialog(QWidget *_parent = 0);

      /// \brief Destructor
      public: ~SaveDialog();

      /// \brief Get name of file.
      /// \return The name of file.
      public: std::string SaveName() const;

      /// \brief Get the save location.
      /// \return Path of the save location.
      public: std::string SaveLocation() const;

      /// \brief Set the name to save as.
      /// \param[in] _name Name of file.
      public: void SetSaveName(const std::string &_name);

      /// \brief Set the save location.
      /// \param[in] _location Location to save to.
      public: void SetSaveLocation(const std::string &_location);

      /// \brief Set the message to be displayed.
      /// \param[in] _msg Message to be displayed.
      public: void SetMessage(const std::string &_msg);

      /// \brief Set the tile of the dialog.
      /// \param[in] _title Title of dialog.
      public: void SetTitle(const std::string &_title);

      /// \brief Qt event emitted showing the dialog
      protected: virtual void showEvent(QShowEvent *event);

      /// \brief Qt callback when the file directory browse button is pressed.
      private slots: void OnBrowse();

      /// \brief Qt callback when the Cancel button is pressed.
      private slots: void OnCancel();

      /// \brief Qt callback when the Save button is pressed.
      private slots: void OnSave();

      /// \brief Private data pointer
      private: std::unique_ptr<SaveDialogPrivate> dataPtr;
    };
    /// \}
  }
}

#endif
