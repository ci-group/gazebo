/*
 * Copyright 2012 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <string.h>
#include <boost/filesystem.hpp>
#include <sstream>

#include "gazebo/common/Exception.hh"
#include "gazebo/common/Time.hh"
#include "gazebo/common/Console.hh"

using namespace gazebo;
using namespace common;

//////////////////////////////////////////////////
Console::Console()
{
  this->msgStream = &std::cout;
  this->errStream = &std::cerr;
  this->logStream = NULL;
}

//////////////////////////////////////////////////
Console::~Console()
{
  if (this->logStream)
    this->logStream->close();
}

//////////////////////////////////////////////////
void Console::Init(const std::string &_logFilename)
{
  if (!getenv("HOME"))
    gzthrow("Missing HOME environment variable");

  boost::filesystem::path logPath(getenv("HOME"));
  logPath = logPath / ".gazebo/" / _logFilename;

  this->logStream = new std::ofstream(logPath.string().c_str(), std::ios::out);
}

//////////////////////////////////////////////////
void Console::SetQuiet(bool)
{
}

//////////////////////////////////////////////////
std::ostream &Console::ColorMsg(const std::string &_lbl, int _color)
{
  // if (**this->quietP)
  // return this->nullStream;
  // else
  // {
  *this->msgStream << "\033[1;" << _color << "m" << _lbl << "\033[0m ";
  return *this->msgStream;
  // }
}

//////////////////////////////////////////////////
std::ofstream &Console::Log()
{
  if (!this->logStream)
    gzthrow("Console has not been initialized\n");

  *this->logStream << "[" << common::Time::GetWallTime() << "] ";
  this->logStream->flush();
  return *this->logStream;
}

//////////////////////////////////////////////////
std::ostream &Console::ColorErr(const std::string &lbl,
                                const std::string &file,
                                unsigned int line, int color)
{
  int index = file.find_last_of("/") + 1;

  *this->errStream << "\033[1;" << color << "m" << lbl << " [" <<
    file.substr(index , file.size() - index)<< ":" << line << "]\033[0m ";

  return *this->errStream;
}
