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
#ifndef MODELMAKER_HH
#define MODELMAKER_HH

#include <list>
#include <string>

#include "sdf/sdf.h"
#include "gui/EntityMaker.hh"

namespace gazebo
{
  namespace msgs
  {
    class Visual;
  }

  namespace gui
  {
    class ModelMaker : public EntityMaker
    {
      public: ModelMaker();
      public: virtual ~ModelMaker();

      public: void InitFromModel(const std::string &_modelName);
      public: void InitFromFile(const std::string &_filename);
      public: virtual void Start(const rendering::UserCameraPtr _camera);

      public: virtual void Stop();
      public: virtual bool IsActive() const;

      public: virtual void OnMousePush(const common::MouseEvent &_event);
      public: virtual void OnMouseRelease(const common::MouseEvent &_event);
      public: virtual void OnMouseDrag(const common::MouseEvent &_event);
      public: virtual void OnMouseMove(const common::MouseEvent &_event);

      private: virtual void CreateTheEntity();
      private: int state;
      private: bool leftMousePressed;
      private: math::Vector2i mousePushPos, mouseReleasePos;

      private: rendering::VisualPtr modelVisual;
      private: std::list<rendering::VisualPtr> visuals;
      private: sdf::SDFPtr modelSDF;

      private: bool clone;
    };
  }
}
#endif
