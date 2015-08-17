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
#ifndef _GAZEBO_TOP_TOOLBAR_HH_
#define _GAZEBO_TOP_TOOLBAR_HH_

// #include <string>
// #include <vector>
// #include <sdf/sdf.hh>
//
#include "gazebo/gui/qt.h"
//#include "gazebo/common/Event.hh"
#include "gazebo/util/system.hh"

// class QLineEdit;
// class QLabel;
// class QFrame;
// class QHBoxLayout;

namespace gazebo
{
  namespace gui
  {
    /// \brief Toolbar on the top of the main window.
    class GAZEBO_VISIBLE TopToolbar : public QFrame
    {
      Q_OBJECT

      /// \brief Constructor.
      /// \param[in] _parent Parent widget.
      public: TopToolbar(QWidget *_parent = 0);

      /// \brief Destructor.
      public: virtual ~TopToolbar();

      /// \brief Get the toolbar.
      /// \return Toolbar
      public: QToolBar *GetToolbar() const;

      /// \brief Widget for the top toolbar
      private: QToolBar *toolbar;
    };
  }
}
#endif
