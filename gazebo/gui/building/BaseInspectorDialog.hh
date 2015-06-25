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

#ifndef _GAZEBO_BASE_INSPECTOR_DIALOG_HH_
#define _GAZEBO_BASE_INSPECTOR_DIALOG_HH_

#include <string>
#include <vector>
#include "gazebo/gui/qt.h"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace gui
  {
    /// \addtogroup gazebo_gui
    /// \{

    /// \class BaseInspectorDialog BaseInspectorDialog.hh
    /// \brief Base Dialog for a specific inspector dialog.
    class GZ_GUI_BUILDING_VISIBLE BaseInspectorDialog : public QDialog
    {
      Q_OBJECT

      /// \brief Constructor
      public: BaseInspectorDialog(QWidget *_parent);

      /// \brief Destructor
      public: ~BaseInspectorDialog();

      /// \brief initiate color combo box.
      public: void InitColorComboBox();

      /// \brief initiate texture combo box.
      public: void InitTextureComboBox();

      /// \brief get the color.
      /// \return color.
      public: QColor GetColor() const;

      /// \brief get the texture.
      /// \return texture.
      public: QString GetTexture() const;

      /// \brief set the color of the wall.
      /// \param[in] _color color.
      public: void SetColor(const QColor _color);

      /// \brief set the texture of the wall.
      /// \param[in] _texture texture.
      public: void SetTexture(const QString _texture);

      /// \brief combo box for selecting the color.
      public: QComboBox *colorComboBox;

      /// \brief Vector of color options.
      public: std::vector<QColor> colorList;

      /// \brief Combo box for selecting the texture.
      public: QComboBox *textureComboBox;

      /// \brief Vector of texture options.
      public: std::vector<QString> textureList;
    };
    /// \}
  }
}

#endif
