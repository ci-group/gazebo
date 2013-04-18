/*
 * Copyright 2011 Nate Koenig
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

#include "gazebo/msgs/msgs.hh"
#include "gazebo/transport/Transport.hh"
#include "gazebo/gui/Actions.hh"
#include "gazebo/gui/Gui.hh"
#include "gazebo/gui/MainWindow.hh"
#include "gazebo/gui/TimePanel.hh"
#include "gazebo/gui/MainWindow_TEST.hh"

bool g_gotSetWireframe = false;
void OnRequest(ConstRequestPtr &_msg)
{
  if (_msg->request() == "set_wireframe")
    g_gotSetWireframe = true;
}

/////////////////////////////////////////////////
void MainWindow_TEST::Wireframe()
{
  this->resMaxPercentChange = 4.0;
  this->shareMaxPercentChange = 2.0;

  this->Load("empty.world", false, false, true);
  gazebo::transport::NodePtr node;
  gazebo::transport::SubscriberPtr sub;

  node = gazebo::transport::NodePtr(new gazebo::transport::Node());
  node->Init();
  sub = node->Subscribe("~/request", &OnRequest, this);

  // Create the main window.
  gazebo::gui::MainWindow *mainWindow = new gazebo::gui::MainWindow();
  QVERIFY(mainWindow != NULL);
  mainWindow->Load();
  mainWindow->Init();
  mainWindow->show();

  // Process some events, and draw the screen
  for (unsigned int i = 0; i < 10; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Get the user camera, and tell it to save frames
  gazebo::rendering::UserCameraPtr cam = gazebo::gui::get_active_camera();
  if (!cam)
    return;

  cam->SetCaptureData(true);

  // Process some events, and draw the screen
  for (unsigned int i = 0; i < 10; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Get the image data
  const unsigned char *image = cam->GetImageData();
  unsigned int height = cam->GetImageHeight();
  unsigned int width = cam->GetImageWidth();
  unsigned int depth = 3;

  // Calculate the average color.
  unsigned int sum = 0;
  for (unsigned int y = 0; y < height; ++y)
  {
    for (unsigned int x = 0; x < width*depth; ++x)
    {
      unsigned int a = image[(y*width*depth)+x];
      sum += a;
    }
  }
  double avgPreWireframe = static_cast<double>(sum) / (height*width*depth);

  // Trigger the wireframe request.
  gazebo::gui::g_viewWireframeAct->trigger();

  // Redraw the screen
  for (unsigned int i = 0; i < 10; ++i)
  {
    gazebo::common::Time::MSleep(30);
    QCoreApplication::processEvents();
    mainWindow->repaint();
  }

  // Get the new image data, and calculate the new average color
  image = cam->GetImageData();
  sum = 0;
  for (unsigned int y = 0; y < height; ++y)
  {
    for (unsigned int x = 0; x < width*depth; ++x)
    {
      unsigned int a = image[(y*width*depth)+x];
      sum += a;
    }
  }
  double avgPostWireframe = static_cast<double>(sum) / (height*width*depth);

  // Make sure the request was set.
  QVERIFY(g_gotSetWireframe);

  gzdbg << "AvgPrewireframe [" << avgPreWireframe
        << "] AvgPostWireframe[" << avgPostWireframe << "]\n";

  // Removing the grey ground plane should change the image.
  QVERIFY(avgPreWireframe != avgPostWireframe);

  mainWindow->hide();
  delete mainWindow;
}

// Generate a main function for the test
QTEST_MAIN(MainWindow_TEST)
