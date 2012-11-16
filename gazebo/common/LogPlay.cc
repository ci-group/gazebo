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

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/istream_iterator.hpp>

#include "gazebo/math/Rand.hh"

#include "gazebo/common/Exception.hh"
#include "gazebo/common/Console.hh"
#include "gazebo/common/LogWrite.hh"
#include "gazebo/common/LogPlay.hh"

using namespace gazebo;
using namespace common;

/////////////////////////////////////////////////
// Convert a Base64 string.
// We have to use our own function, instead of just using the
// boost::archive iterators, because the boost::archive iterators throw an
// error when the end of the Base64 string is reached. The expection then
// causes nothing to happen.
// TLDR; Boost is broken.
void base64_decode(std::string &_dest, const std::string &_src)
{
  typedef boost::archive::iterators::transform_width<
    boost::archive::iterators::binary_from_base64<const char*>, 8, 6>
    base64_dec;

  try
  {
    base64_dec srcIter(_src.c_str());
    for(unsigned int i=0; i < _src.size(); ++i)
    {
      _dest += *srcIter;
      ++srcIter;
    }
  }
  catch(boost::archive::iterators::dataflow_exception &)
  {
  }
}

/////////////////////////////////////////////////
LogPlay::LogPlay()
{
  this->logStartXml = NULL;
}

/////////////////////////////////////////////////
LogPlay::~LogPlay()
{
}

/////////////////////////////////////////////////
void LogPlay::Open(const std::string &_logFile)
{
  boost::filesystem::path path(_logFile);
  if (!boost::filesystem::exists(path))
    gzthrow("Invalid logfile[" + _logFile + "]. Does not exist.");

  // Parse the log file
  if (!this->xmlDoc.LoadFile(_logFile))
    gzthrow("Unable to parser log file[" << _logFile << "]");

  // Get the gazebo_log element
  this->logStartXml = this->xmlDoc.FirstChildElement("gazebo_log");

  if (!this->logStartXml)
    gzthrow("Log file is missing the <gazebo_log> element");

  // Store the filename for future use.
  this->filename = _logFile;

  // Read in the header.
  this->ReadHeader();
}

/////////////////////////////////////////////////
void LogPlay::ReadHeader()
{
  std::string logVersion, gazeboVersion;
  uint32_t randSeed;
  TiXmlElement *headerXml, *childXml;

  // Get the header element
  headerXml = this->logStartXml->FirstChildElement("header");
  if (!headerXml)
    gzthrow("Log file has no header");

  // Get the log format version
  childXml = headerXml->FirstChildElement("log_version");
  if (!childXml)
    gzerr << "Log file header is missing the log version.\n";
  else
    logVersion = childXml->GetText();

  // Get the gazebo version
  childXml = headerXml->FirstChildElement("gazebo_version");
  if (!childXml)
    gzerr << "Log file header is missing the gazebo version.\n";
  else
    gazeboVersion = childXml->GetText();

  // Get the random number seed.
  childXml = headerXml->FirstChildElement("rand_seed");
  if (!childXml)
    gzerr << "Log file header is missing the random number seed.\n";
  else
    randSeed = boost::lexical_cast<uint32_t>(childXml->GetText());

  gzmsg << "\nLog playback:\n"
        << "  Log Version[" << logVersion << "]\n"
        << "  Gazebo Version[" << gazeboVersion << "]\n"
        << "  Random Seed[" << randSeed << "]\n\n";

  if (logVersion != GZ_LOG_VERSION)
    gzwarn << "Log version[" << logVersion << "] in file[" << this->filename
           << "] does not match Gazebo's log version["
           << GZ_LOG_VERSION << "]\n";

  /// Set the random number seed for simulation
  math::Rand::SetSeed(randSeed);
}

/////////////////////////////////////////////////
bool LogPlay::IsOpen() const
{
  return this->logStartXml != NULL;
}

/////////////////////////////////////////////////
bool LogPlay::Step(std::string &_data)
{
  if (!this->logCurrXml)
    this->logCurrXml = this->logStartXml->FirstChildElement("chunk");
  else
    this->logCurrXml = this->logCurrXml->NextSiblingElement("chunk");

  // Make sure we have valid xml pointer
  if (!this->logCurrXml)
    return false;

  /// Get the chunk's encoding
  std::string encoding = this->logCurrXml->Attribute("encoding");

  // Make sure there is an encoding value.
  if (encoding.empty())
    gzthrow("Enconding missing for a chunk in log file[" +
            this->filename + "]");

  if (encoding == "txt")
    _data = this->logCurrXml->GetText();
  else if (encoding == "bz2")
  {
    std::string data = this->logCurrXml->GetText();
    std::string buffer;

    // Decode the base64 string
    base64_decode(buffer, data);

    // Decompress the bz2 data
    {
      boost::iostreams::filtering_istream in;
      in.push(boost::iostreams::bzip2_decompressor());
      in.push(boost::make_iterator_range(buffer));

      // Get the data
      std::getline(in, _data, '\0');
    }
  }
  else
  {
    gzerr << "Inavlid encoding[" << encoding << "] in log file["
          << this->filename << "]\n";
      return false;
  }

  return true;
}
