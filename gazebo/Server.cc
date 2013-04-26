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
#include <stdio.h>
#include <signal.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "gazebo/gazebo.hh"
#include "gazebo/transport/transport.hh"

#include "gazebo/util/LogRecord.hh"
#include "gazebo/util/LogPlay.hh"
#include "gazebo/common/ModelDatabase.hh"
#include "gazebo/common/Timer.hh"
#include "gazebo/common/Exception.hh"
#include "gazebo/common/Plugin.hh"
#include "gazebo/common/Common.hh"

#include "gazebo/sdf/sdf.hh"

#include "gazebo/sensors/Sensors.hh"

#include "gazebo/physics/PhysicsFactory.hh"
#include "gazebo/physics/Physics.hh"
#include "gazebo/physics/World.hh"
#include "gazebo/physics/Base.hh"

#include "gazebo/Master.hh"
#include "gazebo/Server.hh"

using namespace gazebo;

bool Server::stop = true;

/////////////////////////////////////////////////
Server::Server()
{
  this->receiveMutex = new boost::mutex();
  gazebo::print_version();

  if (signal(SIGINT, Server::SigInt) == SIG_ERR)
    std::cerr << "signal(2) failed while setting up for SIGINT" << std::endl;
}

/////////////////////////////////////////////////
Server::~Server()
{
  fflush(stdout);
  delete this->receiveMutex;
  delete this->master;
}

/////////////////////////////////////////////////
void Server::PrintUsage()
{
  std::cerr << "Run the Gazebo server.\n\n"
    << "Usage: gzserver [options] <world_file>\n\n";
}

/////////////////////////////////////////////////
bool Server::ParseArgs(int argc, char **argv)
{
  // save a copy of argc and argv for consumption by system plugins
  this->systemPluginsArgc = argc;
  this->systemPluginsArgv = new char*[argc];
  for (int i = 0; i < argc; ++i)
  {
    int argv_len = strlen(argv[i]);
    this->systemPluginsArgv[i] = new char[argv_len];
    for (int j = 0; j < argv_len; ++j)
      this->systemPluginsArgv[i][j] = argv[i][j];
  }

  po::options_description v_desc("Allowed options");
  v_desc.add_options()
    ("help,h", "Produce this help message.")
    ("pause,u", "Start the server in a paused state.")
    ("physics,e", po::value<std::string>(),
     "Specify a physics engine (ode|bullet).")
    ("play,p", po::value<std::string>(), "Play a log file.")
    ("record,r", "Record state data.")
    ("record_path", po::value<std::string>()->default_value(""),
     "Aboslute path in which to store state data")
    ("seed",  po::value<double>(),
     "Start with a given random number seed.")
    ("iters",  po::value<unsigned int>(),
     "Number of iterations to simulate.")
    ("server-plugin,s", po::value<std::vector<std::string> >(),
     "Load a plugin.");

  po::options_description h_desc("Hidden options");
  h_desc.add_options()
    ("world_file", po::value<std::string>(), "SDF world to load.");

  h_desc.add_options()
    ("pass_through", po::value<std::vector<std::string> >(),
     "not used, passed through to system plugins.");

  po::options_description desc("Allowed options");
  desc.add(v_desc).add(h_desc);

  po::positional_options_description p_desc;
  p_desc.add("world_file", 1).add("pass_through", -1);

  try
  {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(
          p_desc).allow_unregistered().run(), this->vm);

    po::notify(this->vm);
  }
  catch(boost::exception &_e)
  {
    std::cerr << "Error. Invalid arguments\n";
    // NOTE: boost::diagnostic_information(_e) breaks lucid
    // std::cerr << boost::diagnostic_information(_e) << "\n";
    return false;
  }

  // Set the random number seed if present on the command line.
  if (this->vm.count("seed"))
  {
    try
    {
      math::Rand::SetSeed(this->vm["seed"].as<double>());
    }
    catch(boost::bad_any_cast &_e)
    {
      gzerr << "Unable to set random number seed. Must supply a number.\n";
    }
  }

  if (this->vm.count("help"))
  {
    this->PrintUsage();
    std::cerr << v_desc << "\n";
    return false;
  }

  /// Load all the plugins specified on the command line
  if (this->vm.count("server-plugin"))
  {
    std::vector<std::string> pp =
      this->vm["server-plugin"].as<std::vector<std::string> >();

    for (std::vector<std::string>::iterator iter = pp.begin();
         iter != pp.end(); ++iter)
    {
      gazebo::add_plugin(*iter);
    }
  }

  // Set the parameter to record a log file
  if (this->vm.count("record") ||
      !this->vm["record_path"].as<std::string>().empty())
  {
    this->params["record"] = this->vm["record_path"].as<std::string>();
  }

  if (this->vm.count("iters"))
  {
    try
    {
      this->params["iterations"] = boost::lexical_cast<std::string>(
          this->vm["iters"].as<unsigned int>());
    }
    catch(...)
    {
      this->params["iterations"] = "0";
      gzerr << "Unable to set iterations of [" <<
        this->vm["iters"].as<unsigned int>() << "]\n";
    }
  }

  if (this->vm.count("pause"))
    this->params["pause"] = "true";
  else
    this->params["pause"] = "false";

  // The following "if" block must be processed directly before
  // this->ProcessPrarams.
  //
  // Set the parameter to playback a log file. The log file contains the
  // world description, so don't try to reead the world file from the
  // command line.
  if (this->vm.count("play"))
  {
    // Load the log file
    util::LogPlay::Instance()->Open(this->vm["play"].as<std::string>());

    gzmsg << "\nLog playback:\n"
      << "  Log Version: "
      << util::LogPlay::Instance()->GetLogVersion() << "\n"
      << "  Gazebo Version: "
      << util::LogPlay::Instance()->GetGazeboVersion() << "\n"
      << "  Random Seed: "
      << util::LogPlay::Instance()->GetRandSeed() << "\n";

    // Get the SDF world description from the log file
    std::string sdfString;
    util::LogPlay::Instance()->Step(sdfString);

    // Load the server
    if (!this->LoadString(sdfString))
      return false;
  }
  else
  {
    // Get the world file name from the command line, or use "empty.world"
    // if no world file is specified.
    std::string configFilename = "worlds/empty.world";
    if (this->vm.count("world_file"))
      configFilename = this->vm["world_file"].as<std::string>();

    // Get the physics engine name specified from the command line, or use ""
    // if no physics engine is specified.
    std::string physics;
    if (this->vm.count("physics"))
      physics = this->vm["physics"].as<std::string>();

    // Load the server
    if (!this->LoadFile(configFilename, physics))
      return false;
  }

  this->ProcessParams();
  this->Init();

  return true;
}

/////////////////////////////////////////////////
bool Server::GetInitialized() const
{
  return !this->stop && !transport::is_stopped();
}

/////////////////////////////////////////////////
bool Server::LoadFile(const std::string &_filename,
                      const std::string &_physics)
{
  // Quick test for a valid file
  FILE *test = fopen(common::find_file(_filename).c_str(), "r");
  if (!test)
  {
    gzerr << "Could not open file[" << _filename << "]\n";
    return false;
  }
  fclose(test);

  // Load the world file
  sdf::SDFPtr sdf(new sdf::SDF);
  if (!sdf::init(sdf))
  {
    gzerr << "Unable to initialize sdf\n";
    return false;
  }

  if (!sdf::readFile(_filename, sdf))
  {
    gzerr << "Unable to read sdf file[" << _filename << "]\n";
    return false;
  }

  return this->LoadImpl(sdf->root, _physics);
}

/////////////////////////////////////////////////
bool Server::LoadString(const std::string &_sdfString)
{
  // Load the world file
  sdf::SDFPtr sdf(new sdf::SDF);
  if (!sdf::init(sdf))
  {
    gzerr << "Unable to initialize sdf\n";
    return false;
  }

  if (!sdf::readString(_sdfString, sdf))
  {
    gzerr << "Unable to read SDF string[" << _sdfString << "]\n";
    return false;
  }

  return this->LoadImpl(sdf->root);
}

/////////////////////////////////////////////////
bool Server::LoadImpl(sdf::ElementPtr _elem,
                      const std::string &_physics)
{
  std::string host = "";
  unsigned int port = 0;

  gazebo::transport::get_master_uri(host, port);

  this->master = new gazebo::Master();
  this->master->Init(port);
  this->master->RunThread();


  // Load gazebo
  gazebo::load(this->systemPluginsArgc, this->systemPluginsArgv);

  /// Load the sensors library
  sensors::load();

  /// Load the physics library
  physics::load();

  // If a physics engine is specified,
  if (_physics.length())
  {
    // Check if physics engine name is valid
    // This must be done after physics::load();
    if (!physics::PhysicsFactory::IsRegistered(_physics))
    {
      gzerr << "Unregistered physics engine [" << _physics
            << "], the default will be used instead.\n";
    }
    // Try inserting physics engine name if one is given
    else if (_elem->HasElement("world") &&
             _elem->GetElement("world")->HasElement("physics"))
    {
      _elem->GetElement("world")->GetElement("physics")
           ->GetAttribute("type")->Set(_physics);
    }
    else
    {
      gzerr << "Cannot set physics engine: <world> does not have <physics>\n";
    }
  }

  sdf::ElementPtr worldElem = _elem->GetElement("world");
  if (worldElem)
  {
    physics::WorldPtr world = physics::create_world();

    // Create the world
    try
    {
      physics::load_world(world, worldElem);
    }
    catch(common::Exception &e)
    {
      gzthrow("Failed to load the World\n"  << e);
    }
  }

  this->node = transport::NodePtr(new transport::Node());
  this->node->Init("/gazebo");
  this->serverSub = this->node->Subscribe("/gazebo/server/control",
                                          &Server::OnControl, this);

  this->worldModPub =
    this->node->Advertise<msgs::WorldModify>("/gazebo/world/modify");

  // Run the gazebo, starts a new thread
  gazebo::run();

  return true;
}

/////////////////////////////////////////////////
void Server::Init()
{
  // Make sure the model database has started.
  common::ModelDatabase::Instance()->Start();

  gazebo::init();

  sensors::init();

  physics::init_worlds();
  this->stop = false;
}

/////////////////////////////////////////////////
void Server::SigInt(int)
{
  stop = true;
}

/////////////////////////////////////////////////
void Server::Stop()
{
  this->stop = true;
}

/////////////////////////////////////////////////
void Server::Fini()
{
  this->Stop();

  gazebo::fini();

  physics::fini();

  sensors::fini();

  if (this->master)
    this->master->Fini();
  delete this->master;
  this->master = NULL;

  // Cleanup model database.
  common::ModelDatabase::Instance()->Fini();
}

/////////////////////////////////////////////////
void Server::Run()
{
  if (this->stop)
    return;

  // Make sure the sensors are updated once before running the world.
  // This makes sure plugins get loaded properly.
  sensors::run_once(true);

  // Run the sensor threads
  sensors::run_threads();

  unsigned int iterations = 0;
  common::StrStr_M::iterator piter = this->params.find("iterations");
  if (piter != this->params.end())
  {
    try
    {
      iterations = boost::lexical_cast<unsigned int>(piter->second);
    }
    catch(...)
    {
      iterations = 0;
      gzerr << "Unable to cast iterations[" << piter->second << "] "
        << "to unsigned integer\n";
    }
  }

  // Run each world. Each world starts a new thread
  physics::run_worlds(iterations);

  // Update the sensors.
  while (!this->stop && physics::worlds_running())
  {
    this->ProcessControlMsgs();
    sensors::run_once();
    common::Time::MSleep(1);
  }

  // Stop all the worlds
  physics::stop_worlds();

  sensors::stop();

  // Stop gazebo
  gazebo::stop();

  // Stop the master
  this->master->Stop();
}

/////////////////////////////////////////////////
void Server::ProcessParams()
{
  common::StrStr_M::const_iterator iter;
  for (iter = this->params.begin(); iter != this->params.end(); ++iter)
  {
    if (iter->first == "pause")
    {
      bool p = false;
      try
      {
        p = boost::lexical_cast<bool>(iter->second);
      }
      catch(...)
      {
        // Unable to convert via lexical_cast, so try "true/false" string
        std::string str = iter->second;
        boost::to_lower(str);

        if (str == "true")
          p = true;
        else if (str == "false")
          p = false;
        else
          gzerr << "Invalid param value[" << iter->first << ":"
                << iter->second << "]\n";
      }

      physics::pause_worlds(p);
    }
    else if (iter->first == "record")
    {
      util::LogRecord::Instance()->Start("bz2", iter->second);
    }
  }
}

/////////////////////////////////////////////////
void Server::SetParams(const common::StrStr_M &_params)
{
  common::StrStr_M::const_iterator iter;
  for (iter = _params.begin(); iter != _params.end(); ++iter)
    this->params[iter->first] = iter->second;
}

/////////////////////////////////////////////////
void Server::OnControl(ConstServerControlPtr &_msg)
{
  boost::mutex::scoped_lock lock(*this->receiveMutex);
  this->controlMsgs.push_back(*_msg);
}

/////////////////////////////////////////////////
void Server::ProcessControlMsgs()
{
  std::list<msgs::ServerControl>::iterator iter;
  for (iter = this->controlMsgs.begin();
       iter != this->controlMsgs.end(); ++iter)
  {
    if ((*iter).has_save_world_name())
    {
      physics::WorldPtr world = physics::get_world((*iter).save_world_name());
      if ((*iter).has_save_filename())
        world->Save((*iter).save_filename());
      else
        gzerr << "No filename specified.\n";
    }
    else if ((*iter).has_new_world() && (*iter).new_world())
    {
      this->OpenWorld("worlds/empty.world");
    }
    else if ((*iter).has_open_filename())
    {
      this->OpenWorld((*iter).open_filename());
    }
  }
  this->controlMsgs.clear();
}

/////////////////////////////////////////////////
bool Server::OpenWorld(const std::string & /*_filename*/)
{
  gzerr << "Open World is not implemented\n";
  return false;
/*
  sdf::SDFPtr sdf(new sdf::SDF);
  if (!sdf::init(sdf))
  {
    gzerr << "Unable to initialize sdf\n";
    return false;
  }

  if (!sdf::readFile(_filename, sdf))
  {
    gzerr << "Unable to read sdf file[" << _filename << "]\n";
    return false;
  }

  msgs::WorldModify worldMsg;
  worldMsg.set_world_name("default");
  worldMsg.set_remove(true);
  this->worldModPub->Publish(worldMsg);

  physics::stop_worlds();

  physics::remove_worlds();

  sensors::remove_sensors();

  gazebo::transport::clear_buffers();

  sdf::ElementPtr worldElem = sdf->root->GetElement("world");

  physics::WorldPtr world = physics::create_world();

  physics::load_world(world, worldElem);

  physics::init_world(world);

  physics::run_world(world);

  worldMsg.set_world_name("default");
  worldMsg.set_remove(false);
  worldMsg.set_create(true);
  this->worldModPub->Publish(worldMsg);
  return true;
  */
}
