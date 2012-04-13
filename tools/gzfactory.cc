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
#include <transport/transport.h>
#include <boost/program_options.hpp>
#include <fstream>
#include <string>

using namespace gazebo;
namespace po = boost::program_options;

/////////////////////////////////////////////////
void help()
{
  std::cerr << "This tool for spawning or deleting models into or from a "
            << "running Gazebo simulation.\n\n"
            << "  gzfactory <spawn|delete> [options]\n"
            << "    spawn   : Spawn new model. Must specify a SDF model file.\n"
            << "    delete  : Delete existing model. Must specify model name.\n"
            << "\n\n";
}

/////////////////////////////////////////////////
void Spawn(po::variables_map &vm)
{
  std::string filename, modelName;
  std::string worldName = "default";

  if (!vm.count("sdf"))
  {
    std::cerr << "Error: Missing filename.\n";
    return;
  }

  if (vm.count("world-name"))
    worldName = vm["world-name"].as<std::string>();

  if (vm.count("model-name"))
    modelName = vm["model-name"].as<std::string>();

  filename = vm["sdf"].as<std::string>();

  std::ifstream ifs(filename.c_str());
  if (!ifs)
  {
    std::cerr << "Error: Unable to open file[" << filename << "]\n";
    return;
  }

  boost::shared_ptr<sdf::SDF> sdf(new sdf::SDF());
  if (!sdf::init(sdf))
  {
    std::cerr << "ERROR: SDF parsing the xml failed" << std::endl;
    return;
  }

  if (!sdf::readFile(filename, sdf))
  {
    std::cerr << "Error: SDF parsing the xml failed\n";
    return;
  }

  sdf::ElementPtr modelElem = sdf->root->GetElement("model");
  sdf::ElementPtr modelPose = modelElem->GetOrCreateElement("origin");

  // Get/Set the model name
  if (modelName.empty())
    modelName = modelElem->GetValueString("name");
  else
    modelElem->GetAttribute("name")->SetFromString(modelName);

  math::Pose pose = modelPose->GetValuePose("pose");
  math::Vector3 rpy = pose.rot.GetAsEuler();
  if (vm.count("pose-x"))
    pose.pos.x = vm["pose-x"].as<double>();
  if (vm.count("pose-y"))
    pose.pos.y = vm["pose-y"].as<double>();
  if (vm.count("pose-z"))
    pose.pos.z = vm["pose-z"].as<double>();
  if (vm.count("pose-R"))
    rpy.x = vm["pose-R"].as<double>();
  if (vm.count("pose-P"))
    rpy.y = vm["pose-P"].as<double>();
  if (vm.count("pose-Y"))
    rpy.z = vm["pose-Y"].as<double>();
  pose.rot.SetFromEuler(rpy);
  modelPose->GetAttribute("pose")->Set(pose);

  std::cout << "Spawning " << modelName << " into "
            << worldName  << " world.\n";

  transport::init();
  transport::run();

  transport::NodePtr node(new transport::Node());
  node->Init();

  transport::PublisherPtr pub = node->Advertise<msgs::Factory>("~/factory");
  pub->WaitForConnection();

  msgs::Factory msg;
  msg.set_sdf(sdf->ToString());
  pub->Publish(msg, true);

  transport::fini();
}

/////////////////////////////////////////////////
void Delete(po::variables_map &vm)
{
  std::string filename, modelName;
  std::string worldName = "default";

  if (vm.count("world-name"))
    worldName = vm["world-name"].as<std::string>();

  if (vm.count("model-name"))
    modelName = vm["model-name"].as<std::string>();
  else
  {
    std::cerr << "Error: No model name specified.\n";
    return;
  }

  msgs::Request *msg = msgs::CreateRequest("entity_delete", modelName);

  transport::init();
  transport::run();

  transport::NodePtr node(new transport::Node());
  node->Init();

  transport::PublisherPtr pub = node->Advertise<msgs::Request>("~/request");
  pub->WaitForConnection();
  pub->Publish(*msg);
  delete msg;

  transport::fini();
}

/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  po::options_description v_desc("Allowed options");
  v_desc.add_options()
    ("help,h", "produce this help message")
    ("sdf,f", po::value<std::string>(), "SDF model file.")
    ("world-name,w", po::value<std::string>(), "Name of Gazebo world.")
    ("model-name,m", po::value<std::string>(), "Model name.")
    ("pose-x,x", po::value<double>(), "set model x position.")
    ("pose-y,y", po::value<double>(), "set model y position.")
    ("pose-z,z", po::value<double>(), "set model z positione.")
    ("pose-R,R", po::value<double>(), "set model roll orientation in radians.")
    ("pose-P,P", po::value<double>(), "set model pitch orientation in radians.")
    ("pose-Y,Y", po::value<double>(), "set model yaw orientation in radians.");

  po::options_description h_desc("Hidden options");
  h_desc.add_options()
    ("command", po::value<std::string>(), "<spawn|delete>");

  po::options_description desc("Allowed options");
  desc.add(v_desc).add(h_desc);

  po::positional_options_description p_desc;
  p_desc.add("command", 1);

  po::variables_map vm;
  try
  {
    po::store(po::command_line_parser(argc,
          argv).options(desc).positional(p_desc).run(), vm);
    po::notify(vm);
  } catch(boost::exception &_e)
  {
    std::cerr << "Error. Invalid arguments\n";
    return -1;
  }


  if (vm.count("help") || argc < 2)
  {
    help();
    std::cout << v_desc << "\n";
    return -1;
  }

  if (vm.count("command"))
  {
    std::string cmd = vm["command"].as<std::string>();
    if (cmd == "spawn")
      Spawn(vm);
    else if (cmd == "delete")
      Delete(vm);
    else
    {
      std::cerr << "Invalid command[" << cmd << "]. ";
      std::cerr << "Must use 'spawn' or 'delete'\n";
    }
  }
  else
    std::cerr << "Error: specify 'spawn' or 'delete'\n";

  return 0;
}
