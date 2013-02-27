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
#include <math.h>

#include "gazebo/common/Exception.hh"
#include "gazebo/math/gzmath.hh"

#include "gazebo/transport/transport.hh"

#include "gazebo/rendering/Conversions.hh"
#include "gazebo/rendering/Heightmap.hh"
#include "gazebo/rendering/RenderEvents.hh"
#include "gazebo/rendering/Rendering.hh"
#include "gazebo/rendering/Visual.hh"
#include "gazebo/rendering/WindowManager.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/UserCamera.hh"
#include "gazebo/rendering/OrbitViewController.hh"
#include "gazebo/rendering/FPSViewController.hh"

#include "gazebo/gui/MouseEventHandler.hh"
#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/Gui.hh"
#include "gazebo/gui/ModelRightMenu.hh"
#include "gazebo/gui/GuiEvents.hh"
#include "gazebo/gui/GLWidget.hh"

using namespace gazebo;
using namespace gui;

extern bool g_fullscreen;
extern ModelRightMenu *g_modelRightMenu;

/////////////////////////////////////////////////
GLWidget::GLWidget(QWidget *_parent)
  : QWidget(_parent)
{
  this->setObjectName("GLWidget");
  this->state = "select";

  this->setFocusPolicy(Qt::StrongFocus);

  this->windowId = -1;

  setAttribute(Qt::WA_OpaquePaintEvent, true);
  setAttribute(Qt::WA_PaintOnScreen, true);
//  setMinimumSize(320, 240);

  this->renderFrame = new QFrame;
  this->renderFrame->setFrameShape(QFrame::NoFrame);
  this->renderFrame->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Expanding);
  this->renderFrame->show();
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(this->renderFrame);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  this->setLayout(mainLayout);

  this->connections.push_back(
      rendering::Events::ConnectCreateScene(
        boost::bind(&GLWidget::OnCreateScene, this, _1)));

  this->connections.push_back(
      rendering::Events::ConnectRemoveScene(
        boost::bind(&GLWidget::OnRemoveScene, this, _1)));

  this->connections.push_back(
      gui::Events::ConnectMoveMode(
        boost::bind(&GLWidget::OnMoveMode, this, _1)));

  this->connections.push_back(
      gui::Events::ConnectCreateEntity(
        boost::bind(&GLWidget::OnCreateEntity, this, _1, _2)));

  this->connections.push_back(
      gui::Events::ConnectFPS(
        boost::bind(&GLWidget::OnFPS, this)));

  this->connections.push_back(
      gui::Events::ConnectOrbit(
        boost::bind(&GLWidget::OnOrbit, this)));

  this->connections.push_back(
      gui::Events::ConnectManipMode(
        boost::bind(&GLWidget::OnManipMode, this, _1)));

  this->connections.push_back(
     event::Events::ConnectSetSelectedEntity(
       boost::bind(&GLWidget::OnSetSelectedEntity, this, _1, _2)));

  this->renderFrame->setMouseTracking(true);
  this->setMouseTracking(true);

  this->entityMaker = NULL;

  this->node = transport::NodePtr(new transport::Node());
  this->node->Init();
  this->modelPub = this->node->Advertise<msgs::Model>("~/model/modify");
  this->lightPub = this->node->Advertise<msgs::Light>("~/light");

  this->factoryPub = this->node->Advertise<msgs::Factory>("~/factory");
  this->selectionSub = this->node->Subscribe("~/selection",
      &GLWidget::OnSelectionMsg, this);
  this->requestSub = this->node->Subscribe("~/request",
      &GLWidget::OnRequest, this);

  this->installEventFilter(this);
  this->keyModifiers = 0;

  this->selectedVis.reset();
  this->mouseMoveVis.reset();

  connect(g_terrainRaiseAct, SIGNAL(triggered()), this,
      SLOT(OnRaiseTerrain()));

  MouseEventHandler::Instance()->AddFilter("glwidget",
      boost::bind(&GLWidget::OnMousePress, this, _1));
}

/////////////////////////////////////////////////
GLWidget::~GLWidget()
{
  this->connections.clear();
  this->node.reset();
  this->modelPub.reset();
  this->lightPub.reset();
  this->selectionSub.reset();

  this->userCamera.reset();
}

/////////////////////////////////////////////////
bool GLWidget::eventFilter(QObject * /*_obj*/, QEvent *_event)
{
  if (_event->type() == QEvent::Enter)
  {
    this->setFocus(Qt::OtherFocusReason);
    return true;
  }

  return false;
}

/////////////////////////////////////////////////
void GLWidget::showEvent(QShowEvent *_event)
{
  QApplication::flush();
  this->windowId = rendering::WindowManager::Instance()->CreateWindow(
      this->GetOgreHandle(), this->width(), this->height());

  QWidget::showEvent(_event);

  if (this->userCamera)
    rendering::WindowManager::Instance()->SetCamera(this->windowId,
                                                    this->userCamera);
  this->setFocus();
}

/////////////////////////////////////////////////
void GLWidget::enterEvent(QEvent * /*_event*/)
{
}

/////////////////////////////////////////////////
void GLWidget::moveEvent(QMoveEvent *_e)
{
  QWidget::moveEvent(_e);

  if (_e->isAccepted() && this->windowId >= 0)
  {
    rendering::WindowManager::Instance()->Moved(this->windowId);
  }
}

/////////////////////////////////////////////////
void GLWidget::paintEvent(QPaintEvent *_e)
{
  rendering::UserCameraPtr cam = gui::get_active_camera();
  if (cam && cam->IsInitialized())
  {
    event::Events::preRender();

    // Tell all the cameras to render
    event::Events::render();

    event::Events::postRender();
  }
  _e->accept();
}

/////////////////////////////////////////////////
void GLWidget::resizeEvent(QResizeEvent *_e)
{
  if (!this->scene)
    return;

  if (this->windowId >= 0)
  {
    rendering::WindowManager::Instance()->Resize(this->windowId,
        _e->size().width(), _e->size().height());
    this->userCamera->Resize(_e->size().width(), _e->size().height());
  }
}

/////////////////////////////////////////////////
void GLWidget::keyPressEvent(QKeyEvent *_event)
{
  if (!this->scene)
    return;

  if (_event->isAutoRepeat())
    return;

  this->keyText = _event->text().toStdString();
  this->keyModifiers = _event->modifiers();

  // Toggle full screen
  if (_event->key() == Qt::Key_F11)
  {
    g_fullscreen = !g_fullscreen;
    gui::Events::fullScreen(g_fullscreen);
  }

  // Trigger a model delete if the Delete key was pressed, and a model
  // is currently selected.
  if (_event->key() == Qt::Key_Delete && this->selectedVis)
    g_deleteAct->Signal(this->selectedVis->GetName());

  if (_event->key() == Qt::Key_Escape)
    event::Events::setSelectedEntity("", "normal");

  this->mouseEvent.control =
    this->keyModifiers & Qt::ControlModifier ? true : false;
  this->mouseEvent.shift =
    this->keyModifiers & Qt::ShiftModifier ? true : false;
  this->mouseEvent.alt =
    this->keyModifiers & Qt::AltModifier ? true : false;

  this->userCamera->HandleKeyPressEvent(this->keyText);
}

/////////////////////////////////////////////////
void GLWidget::keyReleaseEvent(QKeyEvent *_event)
{
  if (!this->scene)
    return;

  if (_event->isAutoRepeat())
    return;

  this->keyText = "";

  this->keyModifiers = _event->modifiers();

  if (this->keyModifiers & Qt::ControlModifier &&
      _event->key() == Qt::Key_Z)
  {
    this->PopHistory();
  }

  this->mouseEvent.control =
    this->keyModifiers & Qt::ControlModifier ? true : false;
  this->mouseEvent.shift =
    this->keyModifiers & Qt::ShiftModifier ? true : false;
  this->mouseEvent.alt =
    this->keyModifiers & Qt::AltModifier ? true : false;

  // Reset the mouse move info when the user hits keys.
  if (this->state == "translate" || this->state == "rotate")
  {
    if (this->keyText == "x" || this->keyText == "y" || this->keyText == "z")
    {
      this->mouseEvent.pressPos = this->mouseEvent.pos;
      if (this->mouseMoveVis)
        this->mouseMoveVisStartPose = this->mouseMoveVis->GetWorldPose();
    }
  }

  this->userCamera->HandleKeyReleaseEvent(_event->text().toStdString());
}

/////////////////////////////////////////////////
void GLWidget::mouseDoubleClickEvent(QMouseEvent * /*_event*/)
{
  rendering::VisualPtr vis = this->userCamera->GetVisual(this->mouseEvent.pos);
  if (vis)
  {
    if (vis->IsPlane())
    {
      math::Pose pose, camPose;
      camPose = this->userCamera->GetWorldPose();
      if (this->scene->GetFirstContact(this->userCamera,
                                   this->mouseEvent.pos, pose.pos))
      {
        this->userCamera->SetFocalPoint(pose.pos);

        math::Vector3 dir = pose.pos - camPose.pos;
        pose.pos = camPose.pos + (dir * 0.8);

        pose.rot = this->userCamera->GetWorldRotation();
        this->userCamera->MoveToPosition(pose, 0.5);
      }
    }
    else
    {
      this->userCamera->MoveToVisual(vis);
    }
  }
}

/////////////////////////////////////////////////
void GLWidget::mousePressEvent(QMouseEvent *_event)
{
  if (!this->scene)
    return;

  this->mouseEvent.pressPos.Set(_event->pos().x(), _event->pos().y());
  this->mouseEvent.prevPos = this->mouseEvent.pressPos;

  /// Set the button which cause the press event
  if (_event->button() == Qt::LeftButton)
    this->mouseEvent.button = common::MouseEvent::LEFT;
  else if (_event->button() == Qt::RightButton)
    this->mouseEvent.button = common::MouseEvent::RIGHT;
  else if (_event->button() == Qt::MidButton)
    this->mouseEvent.button = common::MouseEvent::MIDDLE;

  this->mouseEvent.buttons = common::MouseEvent::NO_BUTTON;
  this->mouseEvent.type = common::MouseEvent::PRESS;

  this->mouseEvent.buttons |= _event->buttons() & Qt::LeftButton ?
    common::MouseEvent::LEFT : 0x0;
  this->mouseEvent.buttons |= _event->buttons() & Qt::RightButton ?
    common::MouseEvent::RIGHT : 0x0;
  this->mouseEvent.buttons |= _event->buttons() & Qt::MidButton ?
    common::MouseEvent::MIDDLE : 0x0;

  this->mouseEvent.dragging = false;

  // Process Mouse Events
  MouseEventHandler::Instance()->Handle(this->mouseEvent);
}

/////////////////////////////////////////////////
bool GLWidget::OnMousePress(const common::MouseEvent & /*_event*/)
{
  // gui::Events::mousePress(this->mouseEvent);

  if (this->state == "make_entity")
    this->OnMousePressMakeEntity();
  else if (this->state == "select")
    this->OnMousePressNormal();
  else if (this->state == "translate" || this->state == "rotate")
    this->OnMousePressTranslate();
  else if (this->state == "raise_terrain" || this->state == "lower_terrain")
    this->OnMousePressRaiseTerrain();

  return true;
}

/////////////////////////////////////////////////
void GLWidget::OnMousePressTranslate()
{
  rendering::VisualPtr vis = this->userCamera->GetVisual(this->mouseEvent.pos);

  if (vis && !vis->IsPlane() &&
      this->mouseEvent.button == common::MouseEvent::LEFT)
  {
    vis = vis->GetRootVisual();
    this->mouseMoveVisStartPose = vis->GetWorldPose();

    this->SetMouseMoveVisual(vis);

    event::Events::setSelectedEntity(this->mouseMoveVis->GetName(), "move");
    QApplication::setOverrideCursor(Qt::ClosedHandCursor);
  }
  else
    this->userCamera->HandleMouseEvent(this->mouseEvent);
}

/////////////////////////////////////////////////
void GLWidget::OnMousePressNormal()
{
  if (!this->userCamera)
    return;

  rendering::VisualPtr vis = this->userCamera->GetVisual(this->mouseEvent.pos);

  this->SetMouseMoveVisual(rendering::VisualPtr());

  this->userCamera->HandleMouseEvent(this->mouseEvent);
}

/////////////////////////////////////////////////
void GLWidget::OnMousePressMakeEntity()
{
  if (this->entityMaker)
    this->entityMaker->OnMousePush(this->mouseEvent);
}

/////////////////////////////////////////////////
void GLWidget::OnMouseMoveRaiseTerrain()
{
  rendering::Heightmap *heightmap = this->scene ?
    this->scene->GetHeightmap() : NULL;

  if (heightmap && this->mouseEvent.dragging &&
      this->mouseEvent.button == common::MouseEvent::LEFT)
  {
    if (this->mouseEvent.shift)
      heightmap->Lower(this->userCamera, this->mouseEvent.pos, 0.2, 0.05);
    else
      heightmap->Raise(this->userCamera, this->mouseEvent.pos, 0.2, 0.05);
  }
  else
    this->OnMouseMoveNormal();
}

/////////////////////////////////////////////////
void GLWidget::OnMousePressRaiseTerrain()
{
}

/////////////////////////////////////////////////
void GLWidget::OnRaiseTerrain()
{
  if (this->state != "raise_terrain")
    this->state = "raise_terrain";
  else
    this->state = "select";
}

/////////////////////////////////////////////////
void GLWidget::wheelEvent(QWheelEvent *_event)
{
  if (!this->scene)
    return;

  this->mouseEvent.scroll.y = _event->delta() > 0 ? -1 : 1;
  this->mouseEvent.type = common::MouseEvent::SCROLL;
  this->mouseEvent.buttons |= _event->buttons() & Qt::LeftButton ?
    common::MouseEvent::LEFT : 0x0;
  this->mouseEvent.buttons |= _event->buttons() & Qt::RightButton ?
    common::MouseEvent::RIGHT : 0x0;
  this->mouseEvent.buttons |= _event->buttons() & Qt::MidButton ?
    common::MouseEvent::MIDDLE : 0x0;

  this->userCamera->HandleMouseEvent(this->mouseEvent);
}

/////////////////////////////////////////////////
void GLWidget::mouseMoveEvent(QMouseEvent *_event)
{
  if (!this->scene)
    return;

  this->setFocus(Qt::MouseFocusReason);

  this->mouseEvent.pos.Set(_event->pos().x(), _event->pos().y());
  this->mouseEvent.type = common::MouseEvent::MOVE;
  this->mouseEvent.buttons |= _event->buttons() & Qt::LeftButton ?
    common::MouseEvent::LEFT : 0x0;
  this->mouseEvent.buttons |= _event->buttons() & Qt::RightButton ?
    common::MouseEvent::RIGHT : 0x0;
  this->mouseEvent.buttons |= _event->buttons() & Qt::MidButton ?
    common::MouseEvent::MIDDLE : 0x0;

  if (_event->buttons())
    this->mouseEvent.dragging = true;
  else
    this->mouseEvent.dragging = false;

  // Update the view depending on the current GUI state
  if (this->state == "make_entity")
    this->OnMouseMoveMakeEntity();
  else if (this->state == "select")
    this->OnMouseMoveNormal();
  else if (this->state == "translate" || this->state == "rotate")
    this->OnMouseMoveTranslate();
  else if (this->state == "raise_terrain" || this->state == "lower_terrain")
    this->OnMouseMoveRaiseTerrain();

  this->mouseEvent.prevPos = this->mouseEvent.pos;
}

/////////////////////////////////////////////////
void GLWidget::OnMouseMoveMakeEntity()
{
  if (this->entityMaker)
  {
    if (this->mouseEvent.dragging)
      this->entityMaker->OnMouseDrag(this->mouseEvent);
    else
      this->entityMaker->OnMouseMove(this->mouseEvent);
  }
}

/////////////////////////////////////////////////
void GLWidget::SmartMoveVisual(rendering::VisualPtr _vis)
{
  if (!this->mouseEvent.dragging)
    return;

  // Get the point on the plane which correspoinds to the mouse
  math::Vector3 pp;

  // Rotate the visual using the middle mouse button
  if (this->mouseEvent.buttons == common::MouseEvent::MIDDLE)
  {
    math::Vector3 rpy = this->mouseMoveVisStartPose.rot.GetAsEuler();
    math::Vector2i delta = this->mouseEvent.pos - this->mouseEvent.pressPos;
    double yaw = (delta.x * 0.01) + rpy.z;
    if (!this->mouseEvent.shift)
    {
      double snap = rint(yaw / (M_PI * .25)) * (M_PI * 0.25);

      if (fabs(yaw - snap) < GZ_DTOR(10))
        yaw = snap;
    }

    _vis->SetWorldRotation(math::Quaternion(rpy.x, rpy.y, yaw));
  }
  else if (this->mouseEvent.buttons == common::MouseEvent::RIGHT)
  {
    math::Vector3 rpy = this->mouseMoveVisStartPose.rot.GetAsEuler();
    math::Vector2i delta = this->mouseEvent.pos - this->mouseEvent.pressPos;
    double pitch = (delta.y * 0.01) + rpy.y;
    if (!this->mouseEvent.shift)
    {
      double snap = rint(pitch / (M_PI * .25)) * (M_PI * 0.25);

      if (fabs(pitch - snap) < GZ_DTOR(10))
        pitch = snap;
    }

    _vis->SetWorldRotation(math::Quaternion(rpy.x, pitch, rpy.z));
  }
  else if (this->mouseEvent.buttons & common::MouseEvent::LEFT &&
           this->mouseEvent.buttons & common::MouseEvent::RIGHT)
  {
    math::Vector3 rpy = this->mouseMoveVisStartPose.rot.GetAsEuler();
    math::Vector2i delta = this->mouseEvent.pos - this->mouseEvent.pressPos;
    double roll = (delta.x * 0.01) + rpy.x;
    if (!this->mouseEvent.shift)
    {
      double snap = rint(roll / (M_PI * .25)) * (M_PI * 0.25);

      if (fabs(roll - snap) < GZ_DTOR(10))
        roll = snap;
    }

    _vis->SetWorldRotation(math::Quaternion(roll, rpy.y, rpy.z));
  }
  else
  {
    this->TranslateEntity(_vis);
  }
}

/////////////////////////////////////////////////
void GLWidget::OnMouseMoveTranslate()
{
  if (this->mouseEvent.dragging)
  {
    if (this->mouseMoveVis &&
        this->mouseEvent.button == common::MouseEvent::LEFT)
    {
      if (this->state == "translate")
        this->TranslateEntity(this->mouseMoveVis);
      else if (this->state == "rotate")
        this->RotateEntity(this->mouseMoveVis);
    }
    else
      this->userCamera->HandleMouseEvent(this->mouseEvent);
  }
  else
  {
    rendering::VisualPtr vis = this->userCamera->GetVisual(
        this->mouseEvent.pos);

    if (vis && !vis->IsPlane())
      QApplication::setOverrideCursor(Qt::OpenHandCursor);
    else
      QApplication::setOverrideCursor(Qt::ArrowCursor);
    this->userCamera->HandleMouseEvent(this->mouseEvent);
  }
}

/////////////////////////////////////////////////
void GLWidget::OnMouseMoveNormal()
{
  if (!this->userCamera)
    return;

  rendering::VisualPtr vis = this->userCamera->GetVisual(this->mouseEvent.pos);

  if (vis && !vis->IsPlane())
    QApplication::setOverrideCursor(Qt::PointingHandCursor);
  else
    QApplication::setOverrideCursor(Qt::ArrowCursor);

  this->userCamera->HandleMouseEvent(this->mouseEvent);
}

/////////////////////////////////////////////////
void GLWidget::mouseReleaseEvent(QMouseEvent *_event)
{
  if (!this->scene)
    return;

  this->mouseEvent.pos.Set(_event->pos().x(), _event->pos().y());
  this->mouseEvent.prevPos = this->mouseEvent.pos;

  if (_event->button() == Qt::LeftButton)
    this->mouseEvent.button = common::MouseEvent::LEFT;
  else if (_event->button() == Qt::RightButton)
    this->mouseEvent.button = common::MouseEvent::RIGHT;
  else if (_event->button() == Qt::MidButton)
    this->mouseEvent.button = common::MouseEvent::MIDDLE;

  this->mouseEvent.buttons = common::MouseEvent::NO_BUTTON;
  this->mouseEvent.type = common::MouseEvent::RELEASE;

  this->mouseEvent.buttons |= _event->buttons() & Qt::LeftButton ?
    common::MouseEvent::LEFT : 0x0;

  this->mouseEvent.buttons |= _event->buttons() & Qt::RightButton ?
    common::MouseEvent::RIGHT : 0x0;

  this->mouseEvent.buttons |= _event->buttons() & Qt::MidButton ?
    common::MouseEvent::MIDDLE : 0x0;

  gui::Events::mouseRelease(this->mouseEvent);
  emit clicked();

  if (this->state == "make_entity")
    this->OnMouseReleaseMakeEntity();
  else if (this->state == "select")
    this->OnMouseReleaseNormal();
  else if (this->state == "translate" || this->state == "rotate")
    this->OnMouseReleaseTranslate();
}

//////////////////////////////////////////////////
void GLWidget::OnMouseReleaseMakeEntity()
{
  if (this->entityMaker)
    this->entityMaker->OnMouseRelease(this->mouseEvent);
}

//////////////////////////////////////////////////
void GLWidget::OnMouseReleaseTranslate()
{
  if (this->mouseEvent.dragging)
  {
    // If we were dragging a visual around, then publish its new pose to the
    // server
    if (this->mouseMoveVis)
    {
      this->PublishVisualPose(this->mouseMoveVis);
      this->SetMouseMoveVisual(rendering::VisualPtr());
      QApplication::setOverrideCursor(Qt::OpenHandCursor);
    }
    this->SetSelectedVisual(rendering::VisualPtr());
    event::Events::setSelectedEntity("", "normal");
  }

  this->userCamera->HandleMouseEvent(this->mouseEvent);
}

//////////////////////////////////////////////////
void GLWidget::OnMouseReleaseNormal()
{
  if (!this->userCamera)
    return;

  if (!this->mouseEvent.dragging)
  {
    rendering::VisualPtr vis =
      this->userCamera->GetVisual(this->mouseEvent.pos);
    if (vis)
    {
      if (this->mouseEvent.button == common::MouseEvent::RIGHT)
      {
        g_modelRightMenu->Run(vis->GetName(), QCursor::pos());
      }
      else if (this->mouseEvent.button == common::MouseEvent::LEFT)
      {
        vis = vis->GetRootVisual();
        this->SetSelectedVisual(vis);
        event::Events::setSelectedEntity(vis->GetName(), "normal");
      }
    }
    else
      this->SetSelectedVisual(rendering::VisualPtr());
  }

  this->userCamera->HandleMouseEvent(this->mouseEvent);
}

//////////////////////////////////////////////////
void GLWidget::ViewScene(rendering::ScenePtr _scene)
{
  if (_scene->GetUserCameraCount() == 0)
    this->userCamera = _scene->CreateUserCamera("rc_camera");
  else
    this->userCamera = _scene->GetUserCamera(0);

  gui::set_active_camera(this->userCamera);
  this->scene = _scene;

  math::Vector3 camPos(5, -5, 2);
  math::Vector3 lookAt(0, 0, 0);
  math::Vector3 delta = lookAt - camPos;

  double yaw = atan2(delta.y, delta.x);

  double pitch = atan2(-delta.z, sqrt(delta.x*delta.x + delta.y*delta.y));
  this->userCamera->SetWorldPose(math::Pose(camPos,
        math::Vector3(0, pitch, yaw)));

  if (this->windowId >= 0)
  {
    rendering::WindowManager::Instance()->SetCamera(this->windowId,
                                                    this->userCamera);
  }
}

/////////////////////////////////////////////////
rendering::ScenePtr GLWidget::GetScene() const
{
  return this->scene;
}

/////////////////////////////////////////////////
void GLWidget::Clear()
{
  gui::clear_active_camera();
  this->userCamera.reset();
  this->scene.reset();
  this->SetSelectedVisual(rendering::VisualPtr());
  this->SetMouseMoveVisual(rendering::VisualPtr());
  this->hoverVis.reset();
  this->keyModifiers = 0;
}


//////////////////////////////////////////////////
rendering::UserCameraPtr GLWidget::GetCamera() const
{
  return this->userCamera;
}

//////////////////////////////////////////////////
std::string GLWidget::GetOgreHandle() const
{
  std::string ogreHandle;

#ifdef WIN32
  ogreHandle = boost::lexical_cast<std::string>(this->winId());
#else
  QX11Info info = x11Info();
  QWidget *q_parent = dynamic_cast<QWidget*>(this->renderFrame);
  ogreHandle = boost::lexical_cast<std::string>(
      reinterpret_cast<uint64_t>(info.display()));
  ogreHandle += ":";
  ogreHandle += boost::lexical_cast<std::string>(
      static_cast<uint32_t>(info.screen()));
  ogreHandle += ":";
  assert(q_parent);
  ogreHandle += boost::lexical_cast<std::string>(
      static_cast<uint64_t>(q_parent->winId()));
#endif

  return ogreHandle;
}

/////////////////////////////////////////////////
void GLWidget::OnRemoveScene(const std::string &_name)
{
  if (this->scene && this->scene->GetName() == _name)
  {
    this->Clear();
  }
}

/////////////////////////////////////////////////
void GLWidget::OnCreateScene(const std::string &_name)
{
  this->hoverVis.reset();
  this->SetSelectedVisual(rendering::VisualPtr());
  this->SetMouseMoveVisual(rendering::VisualPtr());

  this->ViewScene(rendering::get_scene(_name));
}

/////////////////////////////////////////////////
void GLWidget::OnMoveMode(bool _mode)
{
  if (_mode)
  {
    this->entityMaker = NULL;
    this->state = "select";
  }
}

/////////////////////////////////////////////////
void GLWidget::OnCreateEntity(const std::string &_type,
                              const std::string &_data)
{
  this->ClearSelection();

  if (this->entityMaker)
    this->entityMaker->Stop();

  this->entityMaker = NULL;

  if (_type == "box")
  {
    this->boxMaker.Start(this->userCamera);
    if (this->modelMaker.InitFromSDFString(this->boxMaker.GetSDFString()))
      this->entityMaker = &this->modelMaker;
  }
  else if (_type == "sphere")
  {
    this->sphereMaker.Start(this->userCamera);
    if (this->modelMaker.InitFromSDFString(this->sphereMaker.GetSDFString()))
      this->entityMaker = &this->modelMaker;
  }
  else if (_type == "cylinder")
  {
    this->cylinderMaker.Start(this->userCamera);
    if (this->modelMaker.InitFromSDFString(this->cylinderMaker.GetSDFString()))
      this->entityMaker = &this->modelMaker;
  }
  else if (_type == "mesh" && !_data.empty())
  {
    this->meshMaker.Init(_data);
    this->entityMaker = &this->meshMaker;
  }
  else if (_type == "model" && !_data.empty())
  {
    if (this->modelMaker.InitFromFile(_data))
      this->entityMaker = &this->modelMaker;
  }
  else if (_type == "pointlight")
    this->entityMaker =  &this->pointLightMaker;
  else if (_type == "spotlight")
    this->entityMaker =  &this->spotLightMaker;
  else if (_type == "directionallight")
    this->entityMaker =  &this->directionalLightMaker;

  if (this->entityMaker)
  {
    gui::Events::manipMode("make_entity");
    // TODO: change the cursor to a cross
    this->entityMaker->Start(this->userCamera);
  }
  else
  {
    this->state = "select";
    // TODO: make sure cursor state stays at the default
  }
}

/////////////////////////////////////////////////
void GLWidget::OnFPS()
{
  this->userCamera->SetViewController(
      rendering::FPSViewController::GetTypeString());
}

/////////////////////////////////////////////////
void GLWidget::OnOrbit()
{
  this->userCamera->SetViewController(
      rendering::OrbitViewController::GetTypeString());
}

/////////////////////////////////////////////////
void GLWidget::RotateEntity(rendering::VisualPtr &_vis)
{
  math::Vector3 planeNorm, planeNorm2;
  math::Vector3 p1, p2;
  math::Vector3 a, b;
  math::Vector3 ray(0, 0, 0);

  math::Pose pose = _vis->GetPose();

  math::Vector2i diff = this->mouseEvent.pos - this->mouseEvent.pressPos;
  math::Vector3 rpy = this->mouseMoveVisStartPose.rot.GetAsEuler();

  math::Vector3 rpyAmt;

  if (this->keyText == "x" || this->keyText == "X")
    rpyAmt.x = 1.0;
  else if (this->keyText == "y" || this->keyText == "Y")
    rpyAmt.y = 1.0;
  else
    rpyAmt.z = 1.0;

  double amt = diff.y * 0.04;

  if (this->mouseEvent.shift)
    amt = rint(amt / (M_PI * 0.25)) * (M_PI * 0.25);

  rpy += rpyAmt * amt;

  _vis->SetRotation(math::Quaternion(rpy));
}

/////////////////////////////////////////////////
void GLWidget::TranslateEntity(rendering::VisualPtr &_vis)
{
  math::Pose pose = _vis->GetPose();

  math::Vector3 origin1, dir1, p1;
  math::Vector3 origin2, dir2, p2;

  // Cast two rays from the camera into the world
  this->userCamera->GetCameraToViewportRay(this->mouseEvent.pos.x,
      this->mouseEvent.pos.y, origin1, dir1);
  this->userCamera->GetCameraToViewportRay(this->mouseEvent.pressPos.x,
      this->mouseEvent.pressPos.y, origin2, dir2);

  math::Vector3 moveVector(0, 0, 0);
  math::Vector3 planeNorm(0, 0, 1);

  if (this->keyText == "z")
  {
    math::Vector2i diff = this->mouseEvent.pos - this->mouseEvent.pressPos;
    pose.pos.z = this->mouseMoveVisStartPose.pos.z + diff.y * -0.01;
    _vis->SetPose(pose);
    return;
  }
  else if (this->keyText == "x")
  {
    moveVector.x = 1;
  }
  else if (this->keyText == "y")
  {
    moveVector.y = 1;
  }
  else
    moveVector.Set(1, 1, 0);

  // Compute the distance from the camera to plane of translation
  double d = pose.pos.Dot(planeNorm);
  math::Plane plane(planeNorm, d);
  double dist1 = plane.Distance(origin1, dir1);
  double dist2 = plane.Distance(origin2, dir2);

  // Compute two points on the plane. The first point is the current
  // mouse position, the second is the previous mouse position
  p1 = origin1 + dir1 * dist1;
  p2 = origin2 + dir2 * dist2;

  moveVector *= p1 - p2;
  pose.pos = this->mouseMoveVisStartPose.pos + moveVector;

  if (this->mouseEvent.shift)
  {
    if (ceil(pose.pos.x) - pose.pos.x <= .4)
        pose.pos.x = ceil(pose.pos.x);
    else if (pose.pos.x - floor(pose.pos.x) <= .4)
      pose.pos.x = floor(pose.pos.x);

    if (ceil(pose.pos.y) - pose.pos.y <= .4)
        pose.pos.y = ceil(pose.pos.y);
    else if (pose.pos.y - floor(pose.pos.y) <= .4)
      pose.pos.y = floor(pose.pos.y);

    if (moveVector.z > 0.0)
    {
      if (ceil(pose.pos.z) - pose.pos.z <= .4)
        pose.pos.z = ceil(pose.pos.z);
      else if (pose.pos.z - floor(pose.pos.z) <= .4)
        pose.pos.z = floor(pose.pos.z);
    }
  }

  pose.pos.z = _vis->GetPose().pos.z;

  _vis->SetPose(pose);
}

/////////////////////////////////////////////////
void GLWidget::OnSelectionMsg(ConstSelectionPtr &_msg)
{
  if (_msg->has_selected())
  {
    if (_msg->selected())
    {
      this->SetSelectedVisual(this->scene->GetVisual(_msg->name()));
    }
    else
    {
      this->SetSelectedVisual(rendering::VisualPtr());
      this->SetMouseMoveVisual(rendering::VisualPtr());
    }
  }
}

/////////////////////////////////////////////////
void GLWidget::SetSelectedVisual(rendering::VisualPtr _vis)
{
  if (this->selectedVis)
  {
    this->selectedVis->SetHighlighted(false);
  }

  this->selectedVis = _vis;

  if (this->selectedVis && !this->selectedVis->IsPlane())
  {
    this->selectedVis->SetHighlighted(true);
  }
}

/////////////////////////////////////////////////
void GLWidget::SetMouseMoveVisual(rendering::VisualPtr _vis)
{
  this->mouseMoveVis = _vis;
}

/////////////////////////////////////////////////
void GLWidget::OnManipMode(const std::string &_mode)
{
  this->state = _mode;
}

/////////////////////////////////////////////////
void GLWidget::Paste(const std::string &_object)
{
  if (!_object.empty())
  {
    this->ClearSelection();
    if (this->entityMaker)
      this->entityMaker->Stop();

    this->modelMaker.InitFromModel(_object);
    this->entityMaker = &this->modelMaker;
    this->entityMaker->Start(this->userCamera);
    gui::Events::manipMode("make_entity");
  }
}

/////////////////////////////////////////////////
void GLWidget::PublishVisualPose(rendering::VisualPtr _vis)
{
  if (_vis)
  {
    // Check to see if the visual is a model.
    if (gui::get_entity_id(_vis->GetName()))
    {
      msgs::Model msg;
      msg.set_id(gui::get_entity_id(_vis->GetName()));
      msg.set_name(_vis->GetName());

      msgs::Set(msg.mutable_pose(), _vis->GetWorldPose());
      this->modelPub->Publish(msg);
    }
    // Otherwise, check to see if the visual is a light
    else if (this->scene->GetLight(_vis->GetName()))
    {
      msgs::Light msg;
      msg.set_name(_vis->GetName());
      msgs::Set(msg.mutable_pose(), _vis->GetWorldPose());
      this->lightPub->Publish(msg);
    }
  }
}

/////////////////////////////////////////////////
void GLWidget::ClearSelection()
{
  if (this->hoverVis)
  {
    this->hoverVis->SetEmissive(common::Color(0, 0, 0));
    this->hoverVis.reset();
  }

  this->SetSelectedVisual(rendering::VisualPtr());

  this->scene->SelectVisual("", "normal");
}

/////////////////////////////////////////////////
void GLWidget::OnSetSelectedEntity(const std::string &_name,
                                   const std::string &_mode)

{
  std::map<std::string, unsigned int>::iterator iter;
  if (!_name.empty())
  {
    std::string name = _name;
    boost::replace_first(name, gui::get_world()+"::", "");

    this->SetSelectedVisual(this->scene->GetVisual(name));
    this->scene->SelectVisual(name, _mode);
  }
  else
  {
    this->SetSelectedVisual(rendering::VisualPtr());
    this->scene->SelectVisual("", _mode);
  }

  this->hoverVis.reset();
}

/////////////////////////////////////////////////
void GLWidget::PushHistory(const std::string &_visName, const math::Pose &_pose)
{
  if (this->moveHistory.size() == 0 ||
      this->moveHistory.back().first != _visName ||
      this->moveHistory.back().second != _pose)
  {
    this->moveHistory.push_back(std::make_pair(_visName, _pose));
  }
}

/////////////////////////////////////////////////
void GLWidget::PopHistory()
{
  if (this->moveHistory.size() > 0)
  {
    msgs::Model msg;
    msg.set_id(gui::get_entity_id(this->moveHistory.back().first));
    msg.set_name(this->moveHistory.back().first);

    msgs::Set(msg.mutable_pose(), this->moveHistory.back().second);
    this->scene->GetVisual(this->moveHistory.back().first)->SetWorldPose(
        this->moveHistory.back().second);

    this->modelPub->Publish(msg);

    this->moveHistory.pop_back();
  }
}

/////////////////////////////////////////////////
void GLWidget::OnRequest(ConstRequestPtr &_msg)
{
  if (_msg->request() == "entity_delete")
  {
    if (this->selectedVis && this->selectedVis->GetName() == _msg->data())
    {
      this->SetSelectedVisual(rendering::VisualPtr());
    }
    if (this->mouseMoveVis && this->mouseMoveVis->GetName() == _msg->data())
      this->SetMouseMoveVisual(rendering::VisualPtr());
  }
}
