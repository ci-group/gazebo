/*
 * Copyright 2012 Open Source Robotics Foundation
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

#ifndef _WINDOW_DOOR_INSPECTOR_DIALOG_HH_
#define _WINDOW_DOOR_INSPECTOR_DIALOG_HH_

#include "gui/qt.h"

namespace gazebo
{
  namespace gui
  {
    class WindowDoorInspectorDialog : public QDialog
    {
      Q_OBJECT

      public: enum type {WINDOW, DOOR};

      public: WindowDoorInspectorDialog(int _type = WINDOW,
          QWidget *_parent = 0);

      public: ~WindowDoorInspectorDialog();

      public: double GetWidth();

      public: double GetHeight();

      public: double GetDepth();

      public: QPointF GetPosition();

      public: double GetElevation();

      public: std::string GetType();

      public: void SetWidth(double _width);

      public: void SetHeight(double _height);

      public: void SetDepth(double _depth);

      public: void SetPosition(QPointF _pos);

      public: void SetElevation(double _elevation);

      public: void SetType(std::string _type);

      private slots: void OnCancel();

      private slots: void OnOK();

      private: QLabel* modelNameLabel;

      private: int modelType;

      private: std::string modelTypeStr;

      private: QDoubleSpinBox *widthSpinBox;

      private: QDoubleSpinBox *depthSpinBox;

      private: QDoubleSpinBox *heightSpinBox;

      private: QDoubleSpinBox *positionXSpinBox;

      private: QDoubleSpinBox *positionYSpinBox;

      private: QDoubleSpinBox *elevationSpinBox;

      private: QComboBox *typeComboBox;
   };
 }
}

#endif
