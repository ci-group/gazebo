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

#include "gazebo/gui/MouseEventHandler.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
MouseEventHandler::MouseEventHandler()
{
}

/////////////////////////////////////////////////
MouseEventHandler::~MouseEventHandler()
{
}

/////////////////////////////////////////////////
void MouseEventHandler::AddFilter(const std::string &_name,
    MouseEventFilter _filter)
{
  this->filters.push_back(_filter);
}

/////////////////////////////////////////////////
void MouseEventHandler::Handle(const common::MouseEvent &_event)
{
  for (std::list<MouseEventFilter>::iterator iter = this->filters.begin();
       iter != this->filters.end(); ++iter)
  {
    if ((*iter)(_event))
      break;
  }
}
