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
/*
 * Desc: Gazebo Error
 * Author: Nathan Koenig
 * Date: 07 May 2007
 */

#ifndef GAZEBO_EXCEPTION_HH
#define GAZEBO_EXCEPTION_HH

#include <iostream>
#include <sstream>
#include <string>

namespace gazebo
{
  namespace common
  {
    /// \addtogroup gazebo_common
    /// \{
    /// Throw an error
    #define gzthrow(msg) {std::ostringstream throwStream;\
      throwStream << msg << std::endl << std::flush;\
      throw gazebo::common::Exception(__FILE__, __LINE__, throwStream.str()); }

    /// \brief Class for generating exceptions
    class Exception
    {
      /// \brief Constructor
      public: Exception();

      /// \brief Default constructor
      /// \param file File name
      /// \param line Line number where the error occurred
      /// \param msg Error message
      public: Exception(const char *file,
                          int line,
                          std::string msg);

      /// \brief Destructor
      public: virtual ~Exception();

      /// \brief Return the error function
      /// \return The error function name
      public: std::string GetErrorFile() const;

      /// \brief Return the error string
      /// \return The error string
      public: std::string GetErrorStr() const;

      /// \brief The error function
      private: std::string file;

      /// \brief Line the error occured on
      private: int line;

      /// \brief The error string
      private: std::string str;

      /// \brief Ostream operator for Gazebo Error
      public: friend std::ostream &operator<<(std::ostream& out,
                  const gazebo::common::Exception &err)
              {
                return out << err.GetErrorStr();
              }
    };
    /// \}
  }
}

#endif


