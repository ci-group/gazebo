/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
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

#include "gazebo/common/Assert.hh"
#include "gazebo/common/Exception.hh"
#include "gazebo/math/gzmath.hh"

#include "gazebo/transport/transport.hh"

#include "gazebo/rendering/Conversions.hh"
#include "gazebo/rendering/Heightmap.hh"
#include "gazebo/rendering/RenderEvents.hh"
#include "gazebo/rendering/RenderingIface.hh"
#include "gazebo/rendering/Visual.hh"
#include "gazebo/rendering/WindowManager.hh"
#include "gazebo/rendering/RenderEngine.hh"
#include "gazebo/rendering/Scene.hh"
#include "gazebo/rendering/UserCamera.hh"
#include "gazebo/rendering/OrbitViewController.hh"
#include "gazebo/rendering/FPSViewController.hh"
#include "gazebo/rendering/SelectionObj.hh"

#include "gazebo/gui/ModelManipulator.hh"
#include "gazebo/gui/MouseEventHandler.hh"
#include "gazebo/gui/KeyEventHandler.hh"
#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/GuiIface.hh"
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
  this->sceneCreated = false;
  this->copyEntityName = "";

  this->setFocusPolicy(Qt::StrongFocus);

  this->windowId = -1;

  setAttribute(Qt::WA_OpaquePaintEvent, true);
  setAttribute(Qt::WA_PaintOnScreen, true);

  this->renderFrame = new QFrame;
  this->renderFrame->setFrameShape(QFrame::NoFrame);
  this->renderFrame->setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Expanding);
  this->renderFrame->setContentsMargins(0, 0, 0, 0);
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

  this->factoryPub = this->node->Advertise<msgs::Factory>("~/factory");
  this->selectionSub = this->node->Subscribe("~/selection",
      &GLWidget::OnSelectionMsg, this);
  this->requestSub = this->node->Subscribe("~/request",
      &GLWidget::OnRequest, this);

  this->installEventFilter(this);
  this->keyModifiers = 0;

  this->selectedVis.reset();

  MouseEventHandler::Instance()->AddPressFilter("glwidget",
      boost::bind(&GLWidget::OnMousePress, this, _1));

  MouseEventHandler::Instance()->AddReleaseFilter("glwidget",
      boost::bind(&GLWidget::OnMouseRelease, this, _1));

  MouseEventHandler::Instance()->AddMoveFilter("glwidget",
      boost::bind(&GLWidget::OnMouseMove, this, _1));

  MouseEventHandler::Instance()->AddDoubleClickFilter("glwidget",
      boost::bind(&GLWidget::OnMouseDoubleClick, this, _1));

  connect(g_copyAct, SIGNAL(triggered()), this, SLOT(OnCopy()));
  connect(g_pasteAct, SIGNAL(triggered()), this, SLOT(OnPaste()));
}

/////////////////////////////////////////////////
GLWidget::~GLWidget()
{
  MouseEventHandler::Instance()->RemovePressFilter("glwidget");
  MouseEventHandler::Instance()->RemoveReleaseFilter("glwidget");
  MouseEventHandler::Instance()->RemoveMoveFilter("glwidget");
  MouseEventHandler::Instance()->RemoveDoubleClickFilter("glwidget");

  this->connections.clear();
  this->node.reset();
  this->modelPub.reset();
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

  if (this->windowId < 0)
  {
    this->windowId = rendering::RenderEngine::Instance()->GetWindowManager()->
        CreateWindow(this->GetOgreHandle(), this->width(), this->height());
    if (this->userCamera)
    {
      rendering::RenderEngine::Instance()->GetWindowManager()->SetCamera(
        this->windowId, this->userCamera);
    }
  }

  QWidget::showEvent(_event);

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
    rendering::RenderEngine::Instance()->GetWindowManager()->Moved(
        this->windowId);
  }
}

/////////////////////////////////////////////////
void GLWidget::paintEvent(QPaintEvent *_e)
{
  // Timing may cause GLWidget to miss the OnCreateScene event. So, we check
  // here to make sure it's handled.
  if (!this->sceneCreated && rendering::get_scene())
    this->OnCreateScene(rendering::get_scene()->GetName());

  rendering::UserCameraPtr cam = gui::get_active_camera();
  if (cam && cam->GetInitialized())
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
    rendering::RenderEngine::Instance()->GetWindowManager()->Resize(
        this->windowId, _e->size().width(), _e->size().height());
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

  this->keyEvent.key = _event->key();

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
  {
    event::Events::setSelectedEntity("", "normal");
    if (this->state == "make_entity")
    {
      if (this->entityMaker)
        this->entityMaker->Stop();
    }
  }

  this->mouseEvent.control =
    this->keyModifiers & Qt::ControlModifier ? true : false;
  this->mouseEvent.shift =
    this->keyModifiers & Qt::ShiftModifier ? true : false;
  this->mouseEvent.alt =
    this->keyModifiers & Qt::AltModifier ? true : false;

  if (this->mouseEvent.control)
  {
    if (_event->key() == Qt::Key_C && this->selectedVis)
    {
      g_copyAct->trigger();
    }
    else if (_event->key() == Qt::Key_V && !this->copyEntityName.empty())
    {
      g_pasteAct->trigger();
    }
  }

  ModelManipulator::Instance()->OnKeyPressEvent(this->keyEvent);

  this->userCamera->HandleKeyPressEvent(this->keyText);

  // Process Key Events
  KeyEventHandler::Instance()->HandlePress(this->keyEvent);
}

/////////////////////////////////////////////////
void GLWidget::keyReleaseEvent(QKeyEvent *_event)
{
  if (!this->scene)
    return;

  if (_event->isAutoRepeat())
    return;

  this->keyModifiers = _event->modifiers();

  if (this->keyModifiers & Qt::ControlModifier &&
      _event->key() == Qt::Key_Z)
  {
    this->PopHistory();
  }

  /// Switch between RTS modes
  if (this->keyModifiers == Qt::NoModifier && this->state != "make_entity")
  {
    if (_event->key() == Qt::Key_R)
      g_rotateAct->trigger();
    else if (_event->key() == Qt::Key_T)
      g_translateAct->trigger();
    else if (_event->key() == Qt::Key_S)
      g_scaleAct->trigger();
    else if (_event->key() == Qt::Key_Escape)
      g_arrowAct->trigger();
  }

  this->mouseEvent.control =
    this->keyModifiers & Qt::ControlModifier ? true : false;
  this->mouseEvent.shift =
    this->keyModifiers & Qt::ShiftModifier ? true : false;
  this->mouseEvent.alt =
    this->keyModifiers & Qt::AltModifier ? true : false;


  ModelManipulator::Instance()->OnKeyReleaseEvent(this->keyEvent);
  this->keyText = "";

  this->userCamera->HandleKeyReleaseEvent(_event->text().toStdString());

  // Process Key Events
  KeyEventHandler::Instance()->HandleRelease(this->keyEvent);
}

/////////////////////////////////////////////////
void GLWidget::mouseDoubleClickEvent(QMouseEvent *_event)
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
  MouseEventHandler::Instance()->HandleDoubleClick(this->mouseEvent);
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
  MouseEventHandler::Instance()->HandlePress(this->mouseEvent);
}

/////////////////////////////////////////////////
bool GLWidget::OnMousePress(const common::MouseEvent & /*_event*/)
{
  if (this->state == "make_entity")
    this->OnMousePressMakeEntity();
  else if (this->state == "select")
    this->OnMousePressNormal();
  else if (this->state == "translate" || this->state == "rotate"
      || this->state == "scale")
    ModelManipulator::Instance()->OnMousePressEvent(this->mouseEvent);

  return true;
}

/////////////////////////////////////////////////
bool GLWidget::OnMouseRelease(const common::MouseEvent & /*_event*/)
{
  if (this->state == "make_entity")
    this->OnMouseReleaseMakeEntity();
  else if (this->state == "select")
    this->OnMouseReleaseNormal();
  else if (this->state == "translate" || this->state == "rotate"
      || this->state == "scale")
    ModelManipulator::Instance()->OnMouseReleaseEvent(this->mouseEvent);

  return true;
}

/////////////////////////////////////////////////
bool GLWidget::OnMouseMove(const common::MouseEvent & /*_event*/)
{
  // Update the view depending on the current GUI state
  if (this->state == "make_entity")
    this->OnMouseMoveMakeEntity();
  else if (this->state == "select")
    this->OnMouseMoveNormal();
  else if (this->state == "translate" || this->state == "rotate"
      || this->state == "scale")
    ModelManipulator::Instance()->OnMouseMoveEvent(this->mouseEvent);

  return true;
}

/////////////////////////////////////////////////
bool GLWidget::OnMouseDoubleClick(const common::MouseEvent & /*_event*/)
{
  rendering::VisualPtr vis = this->userCamera->GetVisual(this->mouseEvent.pos);
  if (vis && gui::get_entity_id(vis->GetRootVisual()->GetName()))
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
  else
    return false;

  return true;
}

/////////////////////////////////////////////////
void GLWidget::OnMousePressNormal()
{
  if (!this->userCamera)
    return;

  rendering::VisualPtr vis = this->userCamera->GetVisual(this->mouseEvent.pos);

  this->userCamera->HandleMouseEvent(this->mouseEvent);
}

/////////////////////////////////////////////////
void GLWidget::OnMousePressMakeEntity()
{
  if (this->entityMaker)
    this->entityMaker->OnMousePush(this->mouseEvent);
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

  // Process Mouse Events
  MouseEventHandler::Instance()->HandleMove(this->mouseEvent);

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

  // Process Mouse Events
  MouseEventHandler::Instance()->HandleRelease(this->mouseEvent);

  emit clicked();
}

//////////////////////////////////////////////////
void GLWidget::OnMouseReleaseMakeEntity()
{
  if (this->entityMaker)
    this->entityMaker->OnMouseRelease(this->mouseEvent);
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
      vis = vis->GetRootVisual();
      this->SetSelectedVisual(vis);
      event::Events::setSelectedEntity(vis->GetName(), "normal");

      if (this->mouseEvent.button == common::MouseEvent::RIGHT)
      {
        g_modelRightMenu->Run(vis->GetName(), QCursor::pos());
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
  // The user camera name.
  std::string cameraBaseName = "gzclient_camera";
  std::string cameraName = cameraBaseName;

  transport::ConnectionPtr connection = transport::connectToMaster();
  if (connection)
  {
    std::string topicData;
    msgs::Packet packet;
    msgs::Request request;
    msgs::GzString_V topics;

    request.set_id(0);
    request.set_request("get_topics");
    connection->EnqueueMsg(msgs::Package("request", request), true);
    connection->Read(topicData);

    packet.ParseFromString(topicData);
    topics.ParseFromString(packet.serialized_data());

    std::string searchable;
    for (int i = 0; i < topics.data_size(); ++i)
      searchable += topics.data(i);

    int i = 0;
    while (searchable.find(cameraName) != std::string::npos)
    {
      cameraName = cameraBaseName + boost::lexical_cast<std::string>(++i);
    }
  }
  else
    gzerr << "Unable to connect to a running Gazebo master.\n";

  if (_scene->GetUserCameraCount() == 0)
    this->userCamera = _scene->CreateUserCamera(cameraName);
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
    rendering::RenderEngine::Instance()->GetWindowManager()->SetCamera(
        this->windowId, this->userCamera);
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

#if defined(WIN32) || defined(__APPLE__)
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
  GZ_ASSERT(q_parent, "q_parent is null");
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

  this->ViewScene(rendering::get_scene(_name));

  ModelManipulator::Instance()->Init();

  this->sceneCreated = true;
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
    g_copyAct->setEnabled(true);
  }
  else
  {
    g_copyAct->setEnabled(false);
  }
}

/////////////////////////////////////////////////
void GLWidget::OnManipMode(const std::string &_mode)
{
  this->state = _mode;

  if (this->selectedVis)
    ModelManipulator::Instance()->SetAttachedVisual(this->selectedVis);

  ModelManipulator::Instance()->SetManipulationMode(_mode);
}

/////////////////////////////////////////////////
void GLWidget::OnCopy()
{
  if (this->selectedVis)
    this->Copy(this->selectedVis->GetName());
}

/////////////////////////////////////////////////
void GLWidget::OnPaste()
{
  this->Paste(this->copyEntityName);
}

/////////////////////////////////////////////////
void GLWidget::Copy(const std::string &_name)
{
  this->copyEntityName = _name;
  g_pasteAct->setEnabled(true);
}

/////////////////////////////////////////////////
void GLWidget::Paste(const std::string &_name)
{
  if (!_name.empty())
  {
    bool isModel = false;
    bool isLight = false;
    if (scene->GetLight(_name))
      isLight = true;
    else if (scene->GetVisual(_name))
      isModel = true;

    if (isLight || isModel)
    {
      this->ClearSelection();
      if (this->entityMaker)
        this->entityMaker->Stop();

      if (isLight && this->lightMaker.InitFromLight(_name))
      {
        this->entityMaker = &this->lightMaker;
        this->entityMaker->Start(this->userCamera);
        // this makes the entity appear at the mouse cursor
        this->entityMaker->OnMouseMove(this->mouseEvent);
        gui::Events::manipMode("make_entity");
      }
      else if (isModel && this->modelMaker.InitFromModel(_name))
      {
        this->entityMaker = &this->modelMaker;
        this->entityMaker->Start(this->userCamera);
        // this makes the entity appear at the mouse cursor
        this->entityMaker->OnMouseMove(this->mouseEvent);
        gui::Events::manipMode("make_entity");
      }
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
  if (this->moveHistory.empty() ||
      this->moveHistory.back().first != _visName ||
      this->moveHistory.back().second != _pose)
  {
    this->moveHistory.push_back(std::make_pair(_visName, _pose));
  }
}

/////////////////////////////////////////////////
void GLWidget::PopHistory()
{
  if (!this->moveHistory.empty())
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
      this->selectedVis.reset();
      this->SetSelectedVisual(rendering::VisualPtr());
    }
    if (this->copyEntityName == _msg->data())
    {
      this->copyEntityName = "";
      g_pasteAct->setEnabled(false);
    }
  }
}
