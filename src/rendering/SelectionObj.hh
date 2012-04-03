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
#ifndef SELECTION_OBJ
#define SELECTION_OBJ

#include <string>

#include "math/Vector3.hh"
#include "rendering/RenderTypes.hh"

namespace gazebo
{
  namespace rendering
  {
    class Scene;

    /// \addtogroup gazebo_rendering
    /// \{
    /// \brief A graphical selection object
    class SelectionObj
    {
      /// \brief Constructor
      public: SelectionObj(Scene *scene_);

      /// \brief Destructor
      public: virtual ~SelectionObj();

      public: void Init();

      /// \brief Set the position of the node
      public: void Attach(VisualPtr visual);
      public: void Clear();

      /// \brief Return true if the user is move the selection obj
      public: bool IsActive() const;

      /// \brief Set true if the user is moving the selection obj
      public: void SetActive(bool _active);

      /// \brief Get the name of the visual the seleciton obj is attached to
      public: std::string GetVisualName() const;

      /// \brief Highlight the selection object based on a modifier
      public: void SetHighlight(const std::string &_mod);

      private: VisualPtr node;
      private: Scene *scene;
      private: std::string visualName;

      private: bool active;
      private: double boxSize;
    };
    /// \brief
  }
}
#endif
