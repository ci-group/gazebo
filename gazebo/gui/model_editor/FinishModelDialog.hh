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

#ifndef _FINISH_MODEL_DIALOG_HH_
#define _FINISH_MODEL_DIALOG_HH_

#include "gui/qt.h"

namespace gazebo
{
  namespace gui
  {
    class FinishModelDialog : public QDialog
    {
      Q_OBJECT

      public: FinishModelDialog(QWidget *_parent = 0);

      public: ~FinishModelDialog();

      public: std::string GetModelName();

      private slots: void OnBrowse();

      private slots: void OnCancel();

      private slots: void OnFinish();

      private: QLineEdit* modelNameLineEdit;

      private: QLineEdit* modelLocationLineEdit;
   };
 }
}

#endif
