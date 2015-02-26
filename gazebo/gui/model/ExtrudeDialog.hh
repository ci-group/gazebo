/*
 * Copyright (C) 2015 Open Source Robotics Foundation
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

#ifndef _GAZEBO_EXTRUDE_DIALOG_HH_
#define _GAZEBO_EXTRUDE_DIALOG_HH_

#include "gazebo/gui/qt.h"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace gui
  {
    class ExtrudeDialogPrivate;

    /// \addtogroup gazebo_gui
    /// \{

    /// \class ExtrudeDialog ExtrudeDialog.hh gui/gui.hh
    /// \brief Dialog for saving to file.
    class GAZEBO_VISIBLE ExtrudeDialog : public QDialog
    {
      Q_OBJECT

      /// \brief Constructor.
      /// \param[in] _parent Parent QWidget.
      public: ExtrudeDialog(std::string _filename, QWidget *_parent = 0);

      /// \brief Destructor.
      public: ~ExtrudeDialog();

      /// \internal
      /// \brief Pointer to private data.
      private: ExtrudeDialogPrivate *dataPtr;
    };
    /// \}
  }
}

#endif
