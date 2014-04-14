/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
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
#ifndef _KEY_EVENT_HANDLER_HH_
#define _KEY_EVENT_HANDLER_HH_

#include <boost/function.hpp>
#include <string>
#include <list>

#include "gazebo/common/SingletonT.hh"
#include "gazebo/common/KeyEvent.hh"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace gui
  {
    /// \class KeyEventHandler KeyEventHandler.hh gui/Gui.hh
    /// \brief Processes and filters keyboard events.
    class GAZEBO_VISIBLE KeyEventHandler : public SingletonT<KeyEventHandler>
    {
      /// \def KeyEventFilter
      /// \brief Key event function pointer.
      public: typedef boost::function<bool (const common::KeyEvent &_event)>
              KeyEventFilter;

      /// \cond
      /// \brief a class used to store key filters.
      private: class Filter
               {
                 /// \brief Constructor
                 /// \param[in] _name Name associated with the key filter
                 /// \param[in] _func Key callback function.
                 public: Filter(const std::string &_name,
                                KeyEventFilter _func)
                         : name(_name), func(_func) {}

                 /// \brief Equality operator
                 /// \param[in] _f Filter for compare
                 /// \return True if _f.name == this->name
                 public: bool operator==(const Filter &_f) const
                         {
                           return this->name == _f.name;
                         }

                 /// \brief Equality operator
                 /// \param[in] _f Name of a filter for comparison
                 /// \return True if _f == this->name
                 public: bool operator==(const std::string &_f) const
                         {
                           return this->name == _f;
                         }

                 /// \brief Name of the key filter.
                 public: std::string name;

                 /// \brief Event callback function.
                 public: KeyEventFilter func;
               };

      /// \brief Constructor
      private: KeyEventHandler();

      /// \brief Destructor
      private: virtual ~KeyEventHandler();

      /// \brief Add a filter to a key press event.
      /// \param[in] _name Name associated with the filter.
      /// \param[in] _filter Function to call when press event occurs.
      public: void AddPressFilter(const std::string &_name,
                  KeyEventFilter _filter);

      /// \brief Add a filter to a key release event.
      /// \param[in] _name Name associated with the filter.
      /// \param[in] _filter Function to call when release event occurs.
      public: void AddReleaseFilter(const std::string &_name,
                  KeyEventFilter _filter);

      /// \brief Remove a filter from a key press.
      /// \param[in] _name Name associated with the filter to remove.
      public: void RemovePressFilter(const std::string &_name);

      /// \brief Remove a filter from a key release.
      /// \param[in] _name Name associated with the filter to remove.
      public: void RemoveReleaseFilter(const std::string &_name);

      /// \brief Process a key press event.
      /// \param[in] _event The key event.
      public: void HandlePress(const common::KeyEvent &_event);

      /// \brief Process a key release event.
      /// \param[in] _event The key event.
      public: void HandleRelease(const common::KeyEvent &_event);

      /// \brief Helper function to add a named filter to an event list.
      /// \param[in] _name Name associated with the _filter.
      /// \param[in] _filter Filter function callback.
      /// \param[in] _list List which receives the filter.
      private: void Add(const std::string &_name, KeyEventFilter _filter,
                   std::list<Filter> &_list);

      /// \brief Helper function to remove a named filter from an event list.
      /// \param[in] _name Name associated with the filter to remove.
      /// \param[in] _list List which contains the filter to remove.
      private: void Remove(const std::string &_name, std::list<Filter> &_list);

      /// \brief Helper function to process a filters in an event list.
      /// \param[in] _event Key event to process.
      /// \param[in] _list List which contains the filters to process.
      private: void Handle(const common::KeyEvent &_event,
                   std::list<Filter> &_list);

      /// \brief List of key press filters.
      private: std::list<Filter> pressFilters;

      /// \brief List of key release filters.
      private: std::list<Filter> releaseFilters;

      /// \brief This is a singleton class.
      private: friend class SingletonT<KeyEventHandler>;
    };
  }
}
#endif
