#include <QtGui>
#include <QX11Info>

#include <math.h>

#include "rendering/Rendering.hh"
#include "rendering/WindowManager.hh"
#include "rendering/Scene.hh"
#include "rendering/UserCamera.hh"
#include "rendering/OrbitViewController.hh"

#include "GLWidget.hh"

using namespace gazebo;
using namespace gui;

GLWidget::GLWidget(QWidget *parent)
  : QWidget(parent)
{
  this->windowId = -1;
  this->userCamera = NULL;

  setAttribute(Qt::WA_OpaquePaintEvent,true);
  setAttribute(Qt::WA_PaintOnScreen,true);
  setMinimumSize(320,240);

  this->renderFrame = new QFrame;
  this->renderFrame->setLineWidth(1);
  this->renderFrame->setFrameShadow(QFrame::Sunken);
  this->renderFrame->setFrameShape(QFrame::Box);
  this->renderFrame->show();

  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->addWidget(this->renderFrame);
  this->setLayout(mainLayout);
  this->layout()->setContentsMargins(0,0,0,0);

  QTimer *timer = new QTimer(this);
  connect( timer, SIGNAL(timeout()), this, SLOT(update()) );
  timer->start(33);
}

GLWidget::~GLWidget()
{
}

void GLWidget::showEvent(QShowEvent *event)
{
  // Load the Ogre rendering system
  if (!rendering::load(NULL))
    gzthrow("Failed to load the rendering engine");

  QApplication::flush();
  this->windowId = rendering::WindowManager::Instance()->CreateWindow(
      this->GetOgreHandle(), this->width(), this->height());

  QWidget::showEvent(event);
  if (!rendering::init(false))
    gzthrow("Failed to initialized the rendering engine\n");

  rendering::ScenePtr scene = rendering::create_scene("default");

  this->ViewScene( scene );
}

void GLWidget::moveEvent(QMoveEvent *e)
{
  QWidget::moveEvent(e);

  if(e->isAccepted() && this->windowId >= 0)
  {
    rendering::WindowManager::Instance()->Moved(this->windowId);
  }
}

void GLWidget::paintEvent(QPaintEvent *e)
{
  if (this->userCamera)
  {
    event::Events::preRenderSignal();

    // Tell all the cameras to render
    event::Events::renderSignal();

    event::Events::postRenderSignal();

    //this->userCamera->Render();
    //this->userCamera->PostRender();
  }
  e->accept();
}

void GLWidget::resizeEvent(QResizeEvent *e)
{
  if (this->windowId >= 0)
  {
    rendering::WindowManager::Instance()->Resize( this->windowId, 
        e->size().width(), e->size().height());
  }
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
  this->mouseEvent.pressPos.Set( event->pos().x(), event->pos().y() );
  this->mouseEvent.prevPos = this->mouseEvent.pressPos;

  this->mouseEvent.left = event->buttons() & Qt::LeftButton ? common::MouseEvent::DOWN : common::MouseEvent::UP;
  this->mouseEvent.right = event->buttons() & Qt::RightButton ? common::MouseEvent::DOWN : common::MouseEvent::UP;
  this->mouseEvent.middle = event->buttons() & Qt::MidButton ? common::MouseEvent::DOWN : common::MouseEvent::UP;
  this->mouseEvent.dragging = false;
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
  this->mouseEvent.scroll.y = event->delta() > 0 ? -1 : 1;
  this->mouseEvent.middle = common::MouseEvent::SCROLL;
  this->userCamera->HandleMouseEvent(this->mouseEvent);
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
  this->mouseEvent.pos.Set( event->pos().x(), event->pos().y() );
  this->mouseEvent.dragging = true;

  this->userCamera->HandleMouseEvent(this->mouseEvent);

  this->mouseEvent.prevPos = this->mouseEvent.pos;
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
  this->mouseEvent.pos.Set( event->pos().x(), event->pos().y() );
  this->mouseEvent.prevPos = this->mouseEvent.pos;

  this->mouseEvent.left = event->buttons() & Qt::LeftButton ? common::MouseEvent::DOWN : common::MouseEvent::UP;
  this->mouseEvent.right = event->buttons() & Qt::RightButton ? common::MouseEvent::DOWN : common::MouseEvent::UP;
  this->mouseEvent.middle = event->buttons() & Qt::MidButton ? common::MouseEvent::DOWN : common::MouseEvent::UP;

  emit clicked();
}

////////////////////////////////////////////////////////////////////////////////
/// Create the camera
void GLWidget::ViewScene(rendering::ScenePtr scene)
{
  if (scene->GetUserCameraCount() == 0)
    this->userCamera = scene->CreateUserCamera("rc_camera");
  else
    this->userCamera = scene->GetUserCamera(0);

  this->userCamera->SetWorldPosition( common::Vector3(-5,0,5) );
  this->userCamera->SetWorldRotation( common::Quatern::EulerToQuatern(0, DTOR(15), 0) );

  rendering::WindowManager::Instance()->SetCamera(this->windowId, this->userCamera);
}

////////////////////////////////////////////////////////////////////////////////
std::string GLWidget::GetOgreHandle() const
{
  std::string handle;

#ifdef WIN32
  handle = boost::lexical_cast<std::string>(this->winId());
#else
  QX11Info info = x11Info();
  QWidget *q_parent = dynamic_cast<QWidget*>(this->renderFrame);
  handle = boost::lexical_cast<std::string>((unsigned long)info.display());
  handle += ":";
  handle += boost::lexical_cast<std::string>((unsigned int)info.screen());
  handle += ":";
  assert(q_parent);
  handle += boost::lexical_cast<std::string>((unsigned long)q_parent->winId());
#endif


  return handle;
}
