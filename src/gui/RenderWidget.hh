/*
 * Copyright 2011 Nate Koenig & Andrew Howard
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
#ifndef RENDER_WIDGET_HH
#define RENDER_WIDGET_HH

#include <string>
#include <vector>

#include "gui/qt.h"
#include "common/Event.hh"

class QLineEdit;
class QLabel;
class QFrame;
class QHBoxLayout;

namespace gazebo
{
  class GLWidget;

  namespace gui
  {
    class RenderWidget : public QWidget
    {
      Q_OBJECT
      public: RenderWidget(QWidget *_parent = 0);
      public: virtual ~RenderWidget();

      public: void RemoveScene(const std::string &_name);
      public: void CreateScene(const std::string &_name);

      private slots: virtual void update();

      private: void OnFullScreen(bool &_value);

      private: QHBoxLayout *bottomBarLayout;
      private: GLWidget *glWidget;
      private: QFrame *mainFrame;
      private: QLabel *xyzLabel;
      private: QLineEdit *xPosEdit;
      private: QLineEdit *yPosEdit;
      private: QLineEdit *zPosEdit;

      private: QLabel *rpyLabel;
      private: QLineEdit *rollEdit;
      private: QLineEdit *pitchEdit;
      private: QLineEdit *yawEdit;
      private: QLineEdit *fpsEdit;
      private: QLineEdit *trianglesEdit;

      private: std::vector<event::ConnectionPtr> connections;

      private: bool clear;
      private: std::string clearName;

      private: bool create;
      private: std::string createName;
      private: QTimer *timer;
    };
  }
}
#endif


