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
#ifndef SPHEREMAKER_HH
#define SPHEREMAKER_HH

#include "math/Vector2i.hh"
#include "gui/EntityMaker.hh"

namespace gazebo
{
  namespace msgs
  {
    class Visual;
  }

  namespace gui
  {
    class SphereMaker : public EntityMaker
    {
      public: SphereMaker();
      public: virtual ~SphereMaker();

      public: virtual void Start(const rendering::UserCameraPtr _camera);

      public: virtual void Stop();
      public: virtual bool IsActive() const;

      public: virtual void OnMousePush(const common::MouseEvent &_event);
      public: virtual void OnMouseRelease(const common::MouseEvent &_event);
      public: virtual void OnMouseDrag(const common::MouseEvent &_event);

      protected: virtual void CreateTheEntity();

      private: int state;
      private: bool leftMousePressed;
      private: math::Vector2i mousePushPos;
      private: msgs::Visual *visualMsg;

      private: static unsigned int counter;
    };
  }
}
#endif


