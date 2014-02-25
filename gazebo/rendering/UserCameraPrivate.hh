/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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

#ifndef _USERCAMERA_PRIVATE_HH_
#define _USERCAMERA_PRIVATE_HH_

namespace gazebo
{
  namespace rendering
  {
    /// \brief Private data for the UserCamera class
    class UserCameraPrivate
    {
      /// \brief The currently active view controller.
      public: ViewController *viewController;

      /// \brief An orbit view controller.
      public: OrbitViewController *orbitViewController;

      /// \brief A FPS view controller.
      public: FPSViewController *fpsViewController;

      /// \brief The GUI overlay.
      public: GUIOverlay *gui;

      /// \brief Draws a 3D axis in the viewport.
      // public: Ogre::SceneNode *axisNode;

      /// \brief Used to select objects from mouse clicks.
      public: SelectionBuffer *selectionBuffer;

      /// \brief Flag to detect if the user changed the camera pose in the
      /// world file.
      public: bool isCameraSetInWorldFile;
    };
  }
}
#endif
