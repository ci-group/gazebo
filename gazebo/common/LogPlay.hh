/*
 * Copyright 2012 Nate Koenig
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

#ifndef _LOGPLAY_HH_
#define _LOGPLAY_HH_

#include <tinyxml.h>

#include <list>
#include <string>
#include <fstream>

#include "common/SingletonT.hh"

namespace gazebo
{
  namespace common
  {
    /// \addtogroup gazebo_physics
    /// \{

    /// \class Logplay Logplay.hh common/common.hh
    /// \brief Open and playback log files that were recorded using LogRecord.
    ///
    /// Use Logplay to open a log file (Logplay::Open), and access the
    /// recorded state information. Iterators are available to step through
    /// the state information. It is also possible to replay the data in a
    /// World using the Play functions. Replay involves reading and applying
    /// state information to a World.
    ///
    /// \sa LogRecord, State
    class LogPlay : public SingletonT<LogPlay>
    {
      /// \brief Constructor
      private: LogPlay();

      /// \brief Destructor
      private: virtual ~LogPlay();

      /// \brief Open a log file for reading
      ///
      /// Open a log file that was previously recorded.
      /// \param[in] _logFile The file to load
      /// \throws Exception
      public: void Open(const std::string &_logFile);

      /// \brief Return true if a file is open.
      /// \return True if a log file is open.
      public: bool IsOpen() const;

      /// \brief Step through the open log file.
      /// \param[out] _data Data from next entry in the log file.
      public: bool Step(std::string &_data);

      /// \brief Read the header from the log file.
      private: void ReadHeader();

      /// \brief The XML document of the log file.
      private: TiXmlDocument xmlDoc;

      /// \brief Start of the log.
      private: TiXmlElement *logStartXml;

      /// \brief Current position in the log file.
      private: TiXmlElement *logCurrXml;

      /// \brief Name of the log file.
      private: std::string filename;

      /// \brief This is a singleton
      private: friend class SingletonT<LogPlay>;
    };
    /// \}
  }
}

#endif
