/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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

#ifndef _GAZEBO_COMMON_URI_HH_
#define _GAZEBO_COMMON_URI_HH_

#include <memory>
#include <string>
#include "gazebo/util/system.hh"

//#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace common
  {
    // Forward declare private data classes.
    class TokenizerPrivate;
    class URIPathPrivate;
    class URIQueryPrivate;
    class URIPrivate;

    /// \brief A string tokenizer class.
    /// Heavily inspired by http://www.songho.ca/misc/tokenizer/tokenizer.html
    class GZ_COMMON_VISIBLE Tokenizer
    {
      /// \brief Class constructor.
      /// \param[in] _str A string argument.
      public: Tokenizer(const std::string &_str);

      /// \brief Class destructor.
      public: ~Tokenizer() = default;

      /// \brief Splits the internal string into tokens.
      /// \param[in] _delim Token delimiter.
      /// \return Vector of tokens.
      public: std::vector<std::string> Split(const std::string &_delim);

      /// \brief Get the next token.
      /// \param[in] _delim Token delimiter.
      /// \return The token or empty string if there are no more tokens.
      private: std::string NextToken(const std::string &_delim);

      /// \brief Consume a delimiter if we are currently pointing to it.
      /// \param[in] _delim Token delimiter.
      private: void SkipDelimiter(const std::string &_delim);

      /// \brief Return if a delimiter starts at a given position
      /// \param[in] _delim Token delimiter.
      /// \param[in] _currPos Position to check.
      /// \return True when a delimiter starts at _pos.
      private: bool IsDelimiter(const std::string &_delim,
                                const size_t _pos) const;

      /// \internal
      /// \brief Pointer to private data.
      private: std::unique_ptr<TokenizerPrivate> dataPtr;
    };

    /// \brief The path component of a URI
    class GZ_COMMON_VISIBLE URIPath
    {
      /// \brief Constructor
      public: URIPath();

      /// \brief Copy constructor.
      /// \param[in] _path Another URIPath.
      public: URIPath(const URIPath &_path);

      /// \brief Construct a URIPath object from a string.
      /// \param[in] _str A string.
      public: URIPath(const std::string &_str);

      /// \brief Destructor
      public: virtual ~URIPath();

      /// \brief Parse a string and update the current path.
      /// \param[in] _str A string containing a valid path.
      /// \return True if the path was succesfully updated.
      public: bool Load(const std::string &_str);

      /// \brief Remove all parts of the path
      public: void Clear();

      /// \brief Push a new part onto the front of this path.
      /// \param[in] _part Path part to push
      public: void PushFront(const std::string &_part);

      /// \brief Push a new part onto the back of this path.
      /// \param[in] _part Path part to push
      /// \sa operator/
      public: void PushBack(const std::string &_part);

      /// \brief Compound assignment operator.
      /// \param[in] _part A new path to append.
      /// \return A new Path that consists of "this / _part"
      public: const URIPath &operator/=(const std::string &_part);

      /// \brief Get the current path with the _part added to the end.
      /// \param[in] _part Path part.
      /// \return A new Path that consists of "this / _part"
      /// \sa PushBack
      public: const URIPath operator/(const std::string &_part) const;

      /// \brief Return true if the two paths match.
      /// \param[in] _part Path part.
      /// return True of the paths match.
      public: bool operator==(const URIPath &_path) const;

      /// \brief Get the path as a string.
      /// \param[in] _delim Delimiter used to separate each part of the path.
      /// \return The path as a string, with each path part separated by _delim.
      public: std::string Str(const std::string &_delim = "/") const;

      /// \brief Equal operator.
      /// \param[in] _p another URIPath.
      /// \return itself.
      public: URIPath &operator=(const URIPath &_path);

      /// \brief Validate a string as URIPath.
      /// \param[in] _str A string.
      /// \return true if the string can be parsed as a URIPath.
      public: static bool Valid(const std::string &_str,
                                URIPath &_path);

      /// \internal
      /// \brief Pointer to private data.
      private: std::unique_ptr<URIPathPrivate> dataPtr;
    };

    /// \brief The query component of a URI
    class GZ_COMMON_VISIBLE URIQuery
    {
      /// \brief Constructor
      public: URIQuery();

      /// \brief Construct a URIQuery object from a string.
      /// \param[in] _str A string.
      public: URIQuery(const std::string &_str);

      /// \brief Copy constructor
      /// \param[in] _query Another query component
      public: URIQuery(const URIQuery &_query);

      /// \brief Destructor
      public: virtual ~URIQuery();

      /// \brief Parse a string and update the current path.
      /// \param[in] _str A string containing a valid path.
      /// \return True if the path was succesfully updated.
      public: bool Load(const std::string &_str);

      /// \brief Remove all values of the query
      public: void Clear();

      /// \brief Get this query with a new _key=_value pair added.
      /// \param[in] _key Key of the query.
      /// \param[in] _value Value of the query.
      /// \return This query with the additional _key = _value pair
      public: const URIQuery Insert(const std::string &_key,
                                    const std::string &_value);

      /// \brief Equal operator.
      /// \param[in] _p another URIQuery.
      /// \return itself.
      public: URIQuery &operator=(const URIQuery &_query);

      /// \brief Return true if the two queries contain the same values.
      /// \param[in] _query A URI query to compare.
      /// return True of the queries match.
      public: bool operator==(const URIQuery &_query) const;

      /// \brief Get the query as a string.
      /// \param[in] _delim Delimiter used to separate each tuple of the query.
      /// \return The query as a string, with each key,value pair separated by
      /// _delim.
      public: std::string Str(const std::string &_delim = "&") const;

      /// \brief Validate a string as URIQuery.
      /// \param[in] _str A string.
      /// \return true if the string can be parsed as a URIQuery.
      public: static bool Valid(const std::string &_str,
                                URIQuery &_query);

      /// \internal
      /// \brief Pointer to private data.
      private: std::unique_ptr<URIQueryPrivate> dataPtr;
    };

    /// \brief A complete URI
    class GZ_COMMON_VISIBLE URI
    {
      /// \brief Default constructor
      public: URI();

      /// \brief Construct a URI object from a string.
      /// \param[in] _str A string.
      public: URI(const std::string &_str);

      /// \brief Copy constructor
      /// \param[in] _uri Another URI.
      public: URI(const URI &_uri);

      /// \brief Destructor.
      public: ~URI();

      /// \brief Parse a string and update the current path.
      /// \param[in] _str A string containing a valid path.
      /// \return True if the path was succesfully updated.
      public: bool Load(const std::string &_str);

      /// \brief Get the URI as a string, which has the form:
      ///
      /// scheme://path?query
      ///
      /// \return The full URI as a string
      public: std::string Str() const;

      /// \brief Remove all components of the URI
      public: void Clear();

      /// \brief Get the URI's scheme
      /// \return The scheme
      public: std::string Scheme() const;

      /// \brief Set the URI's scheme
      /// \param[in] _scheme New scheme.
      public: void SetScheme(const std::string &_scheme);

      /// \brief Get a mutable version of the path component
      /// \return A reference to the path
      public: URIPath &Path();

      /// \brief Get a mutable version of the query component
      /// \return A reference to the query
      public: URIQuery &Query();

      /// \brief Equal operator.
      /// \param[in] _uri another URI.
      /// \return itself.
      public: URI &operator=(const URI &_uri);

      /// \brief Return true if the two URIs match.
      /// \param[in] _uri another URI to compare.
      /// \return true if the two URIs match.
      public: bool operator==(const URI &_uri) const;

      /// \brief Validate a string as URI.
      /// \param[in] _str A string.
      /// \return true if the string can be parsed as a URI.
      public: static bool Valid(const std::string &_str,
                                URI &_uri);

      /// \internal
      /// \brief Pointer to private data.
      private: std::unique_ptr<URIPrivate> dataPtr;
    };
  }
}
#endif
