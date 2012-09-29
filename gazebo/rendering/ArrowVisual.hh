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
/* Desc: Arrow Visualization Class
 * Author: Nate Koenig
 */

#ifndef __ARROWVISUAL_HH__
#define __ARROWVISUAL_HH__

#include <string>
#include "rendering/Visual.hh"

namespace ogre
{
  class SceneNode;
}

namespace gazebo
{
  namespace rendering
  {
    /// \addtogroup gazebo_rendering Rendering
    /// \{

    /// \class ArrowVisual ArrowVisual.hh rendering/ArrowVisual.hh
    /// \brief Basic arrow visualization
    class ArrowVisual : public Visual
    {
      /// \brief Constructor
      /// \param _name Name of the arrow visual
      /// \param _vis Pointer to the parent visual
      public: ArrowVisual(const std::string &_name, VisualPtr _vis);

      /// \brief Destructor
      public: virtual ~ArrowVisual();

      /// \brief Load the visual with default parameters
      public: virtual void Load();

      /// \brief Show the rotation of the visual
      public: void ShowRotation();

      private: Ogre::SceneNode *headNode;
      private: Ogre::SceneNode *shaftNode;
      private: Ogre::SceneNode *rotationNode;
    };
    /// \}
  }
}
#endif
