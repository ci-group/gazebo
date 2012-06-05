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

#include <signal.h>
#include <boost/program_options.hpp>
#include "gui/qt.h"
#include "gazebo.hh"

#include "common/Exception.hh"
#include "common/Console.hh"
#include "common/Plugin.hh"
#include "common/CommonTypes.hh"
#include "gui/MainWindow.hh"
#include "gui/ModelRightMenu.hh"
#include "gui/Gui.hh"

// These are needed by QT. They need to stay valid during the entire
// lifetime of the application, and argc > 0 and argv must contain one valid
// character string
int g_argc = 1;
char **g_argv;

namespace po = boost::program_options;
po::variables_map vm;

using namespace gazebo;

gui::ModelRightMenu *g_modelRightMenu = NULL;

std::string g_worldname = "default";

QApplication *g_app;
gui::MainWindow *g_main_win;
rendering::UserCameraPtr g_active_camera;
bool g_fullscreen = false;

//////////////////////////////////////////////////
void print_usage()
{
  fprintf(stderr, "Usage: gzclient [-h]\n");
  fprintf(stderr, "  -h            : Print this message.\n");
}

//////////////////////////////////////////////////
void signal_handler(int)
{
  gazebo::stop();
  gazebo::gui::stop();
}

//////////////////////////////////////////////////
bool parse_args(int _argc, char **_argv)
{
  if (signal(SIGINT, signal_handler) == SIG_ERR)
  {
    std::cerr << "signal(2) failed while setting up for SIGINT" << std::endl;
    return false;
  }

  gazebo::print_version();

  po::options_description v_desc("Allowed options");
  v_desc.add_options()
    ("help,h", "Produce this help message.")
    ("gui-plugin,g", po::value<std::vector<std::string> >(), "Load a plugin.");

  po::options_description desc("Allowed options");
  desc.add(v_desc);

  try
  {
    po::store(po::command_line_parser(_argc,
          _argv).options(desc).allow_unregistered().run(), vm);
    po::notify(vm);
  } catch(boost::exception &_e)
  {
    std::cerr << "Error. Gui Invalid arguments\n";
    return false;
  }

  if (vm.count("help"))
  {
    print_usage();
    std::cerr << v_desc << "\n";
    return false;
  }

  /// Load all the plugins specified on the command line
  if (vm.count("gui-plugin"))
  {
    std::vector<std::string> pp =
      vm["gui-plugin"].as<std::vector<std::string> >();

    for (std::vector<std::string>::iterator iter = pp.begin();
         iter != pp.end(); ++iter)
    {
      gazebo::add_plugin(*iter);
    }
  }

  return true;
}

namespace gazebo
{
  namespace gui
  {
    /////////////////////////////////////////////////
    void load()
    {
      g_modelRightMenu = new gui::ModelRightMenu();
      rendering::load();
      rendering::init();

      g_argv = new char*[g_argc];
      for (int i = 0; i < g_argc; i++)
      {
        g_argv[i] = new char[strlen("gazebo")];
        snprintf(g_argv[i], strlen("gazebo"), "gazebo");
      }

      g_app = new QApplication(g_argc, g_argv);

      g_main_win = new gui::MainWindow();

      g_main_win->Load();
      g_main_win->resize(1024, 768);
    }

    /////////////////////////////////////////////////
    void init()
    {
      g_main_win->show();
      g_main_win->Init();
    }

    /////////////////////////////////////////////////
    void fini()
    {
      gui::clear_active_camera();
      rendering::fini();
      fflush(stdout);
    }
  }
}

/////////////////////////////////////////////////
unsigned int gui::get_entity_id(const std::string &_name)
{
  return g_main_win->GetEntityId(_name);
}



/////////////////////////////////////////////////
bool gui::run(int _argc, char **_argv)
{
  if (!parse_args(_argc, _argv))
    return false;

  if (!gazebo::load())
    return false;

  if (!gazebo::init())
    return false;

  gazebo::run();

  gazebo::gui::load();
  gazebo::gui::init();

  g_app->exec();

  gazebo::fini();
  gazebo::gui::fini();
  return true;
}

/////////////////////////////////////////////////
void gui::stop()
{
  g_active_camera.reset();
  g_app->quit();
}



/////////////////////////////////////////////////
void gui::set_world(const std::string &_name)
{
  g_worldname = _name;
}

/////////////////////////////////////////////////
std::string gui::get_world()
{
  return g_worldname;
}

/////////////////////////////////////////////////
void gui::set_active_camera(rendering::UserCameraPtr _cam)
{
  g_active_camera = _cam;
}

/////////////////////////////////////////////////
void gui::clear_active_camera()
{
  g_active_camera.reset();
}

/////////////////////////////////////////////////
rendering::UserCameraPtr gui::get_active_camera()
{
  return g_active_camera;
}

/////////////////////////////////////////////////
bool gui::has_entity_name(const std::string &_name)
{
  return g_main_win->HasEntityName(_name);
}
