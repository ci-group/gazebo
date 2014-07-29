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

#ifndef _ARROWVISUAL_HH_
#define _ARROWVISUAL_HH_

#include <string>
#include "gazebo/rendering/Visual.hh"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace rendering
  {
    /// \addtogroup gazebo_rendering Rendering
    /// \{

    /// \class ArrowVisual ArrowVisual.hh rendering/rendering.hh
    /// \brief Basic arrow visualization
    class GAZEBO_VISIBLE ArrowVisual : public Visual
    {
      /// \brief Constructor
      /// \param[in] _name Name of the arrow visual
      /// \param[in] _vis Pointer to the parent visual
      public: ArrowVisual(const std::string &_name, VisualPtr _vis);

      /// \brief Destructor
      public: virtual ~ArrowVisual();

      /// \brief Load the visual with default parameters
      public: virtual void Load();

      /// \brief Show the rotation of the visual
      public: void ShowRotation();
    };
    /// \}
  }
}
#endif
