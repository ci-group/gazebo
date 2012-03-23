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
#ifndef GL_WIDGET_HH
#define GL_WIDGET_HH

#include <string>
#include <vector>
#include <utility>
#include <list>

#include "gui/qt.h"
#include "rendering/RenderTypes.hh"

#include "transport/TransportTypes.hh"

#include "common/MouseEvent.hh"
#include "common/Event.hh"

#include "math/Pose.hh"

#include "msgs/msgs.h"

#include "gui/BoxMaker.hh"
#include "gui/SphereMaker.hh"
#include "gui/CylinderMaker.hh"
#include "gui/MeshMaker.hh"
#include "gui/ModelMaker.hh"
#include "gui/LightMaker.hh"

namespace gazebo
{
  namespace gui
  {
    class GLWidget : public QWidget
    {
      Q_OBJECT

      public: GLWidget(QWidget *_parent = 0);
      public: virtual ~GLWidget();

      public: void ViewScene(rendering::ScenePtr _scene);
      public: rendering::UserCameraPtr GetCamera() const;
      public: rendering::ScenePtr GetScene() const;

      public: void Clear();

      signals: void clicked();


      protected: virtual void moveEvent(QMoveEvent *_e);
      protected: virtual void paintEvent(QPaintEvent *_e);
      protected: virtual void resizeEvent(QResizeEvent *_e);
      protected: virtual void showEvent(QShowEvent *_e);
      protected: virtual void enterEvent(QEvent * event);


      protected: void keyPressEvent(QKeyEvent *_event);
      protected: void keyReleaseEvent(QKeyEvent *_event);
      protected: void wheelEvent(QWheelEvent *_event);
      protected: void mousePressEvent(QMouseEvent *_event);
      protected: void mouseMoveEvent(QMouseEvent *_event);
      protected: void mouseReleaseEvent(QMouseEvent *_event);

      private: std::string GetOgreHandle() const;

      private: void OnKeyReleaseRing(QKeyEvent *_event);

      private: void OnMouseMoveRing();
      private: void OnMouseMoveNormal();
      private: void OnMouseMoveMakeEntity();

      private: void OnMouseReleaseRing();
      private: void OnMouseReleaseNormal();
      private: void OnMouseReleaseMakeEntity();

      private: void OnMousePressRing();
      private: void OnMousePressNormal();
      private: void OnMousePressMakeEntity();

      private: void OnRequest(ConstRequestPtr &_msg);

      private: void SmartMoveVisual(rendering::VisualPtr _vis);

      private: void OnCreateScene(const std::string &_name);
      private: void OnRemoveScene(const std::string &_name);
      private: void OnMoveMode(bool _mode);
      private: void OnCreateEntity(const std::string &_type,
                                   const std::string &_data);
      private: void OnFPS();
      private: void OnOrbit();
      private: void OnManipMode(const std::string &_mode);
      private: void OnSetSelectedEntity(const std::string &_name);

      private: void RotateEntity(rendering::VisualPtr &_vis);
      private: void TranslateEntity(rendering::VisualPtr &_vis);

      private: void OnMouseMoveVisual(const std::string &_visualName);
      private: void OnSelectionMsg(ConstSelectionPtr &_msg);

      private: bool eventFilter(QObject *_obj, QEvent *_event);
      private: void PublishVisualPose(rendering::VisualPtr _vis);
      private: void ClearSelection();

      /// \brief Copy an object by name
      private: void Paste(const std::string &_object);

      private: void PushHistory(const std::string &_visName,
                                const math::Pose &_pose);
      private: void PopHistory();

      private: int windowId;

      private: rendering::UserCameraPtr userCamera;
      private: rendering::ScenePtr scene;
      private: QFrame *renderFrame;
      private: common::MouseEvent mouseEvent;

      private: std::vector<event::ConnectionPtr> connections;

      private: EntityMaker *entityMaker;
      private: BoxMaker boxMaker;
      private: SphereMaker sphereMaker;
      private: CylinderMaker cylinderMaker;
      private: MeshMaker meshMaker;
      private: ModelMaker modelMaker;
      private: PointLightMaker pointLightMaker;
      private: SpotLightMaker spotLightMaker;
      private: DirectionalLightMaker directionalLightMaker;

      private: rendering::VisualPtr hoverVis, mouseMoveVis;
      private: rendering::SelectionObj *selectionObj;
      private: unsigned int selectionId;
      private: std::string selectionMod;
      private: math::Pose selectionPoseOrig;

      private: transport::NodePtr node;
      private: transport::PublisherPtr modelPub, factoryPub;
      private: transport::SubscriberPtr selectionSub, requestSub;

      private: Qt::KeyboardModifiers keyModifiers;
      private: QPoint onShiftMousePos;
      private: int mouseOffset;
      private: math::Pose mouseMoveVisStartPose;

      private: std::string copiedObject;

      private: std::string state;

      private: std::list<std::pair<std::string, math::Pose> > moveHistory;
    };
  }
}

#endif
