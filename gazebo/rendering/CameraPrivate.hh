/*
 * Copyright (C) 2012-2013 Open Source Robotics Foundation
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
#ifndef _GAZEBO_RENDERING_CAMERA_PRIVATE_HH_
#define _GAZEBO_RENDERING_CAMERA_PRIVATE_HH_

#include <deque>
#include <utility>
#include <list>

#include "gazebo/msgs/msgs.hh"

namespace Ogre
{
  class CompositorInstance;
}

namespace gazebo
{
  namespace rendering
  {
    /// \brief Private data for the Camera class
    class CameraPrivate
    {
      /// \brief Visual that the camera is tracking.
      public: VisualPtr trackedVisual;

      /// \brief Counter used to create unique camera names.
      public: static unsigned int cameraCounter;

      /// \brief Deferred shading geometry buffer.
      public: Ogre::CompositorInstance *dsGBufferInstance;

      /// \brief Deferred shading merge compositor.
      public: Ogre::CompositorInstance *dsMergeInstance;

      /// \brief Deferred lighting geometry buffer.
      public: Ogre::CompositorInstance *dlGBufferInstance;

      /// \brief Deferred lighting merge compositor.
      public: Ogre::CompositorInstance *dlMergeInstance;

      /// \brief Screen space ambient occlusion compositor.
      public: Ogre::CompositorInstance *ssaoInstance;

      /// \brief Gaussian noise compositor
      public: Ogre::CompositorInstance *gaussianNoiseInstance;

      /// \brief Gaussian noise compositor listener
      public: boost::shared_ptr<GaussianNoiseCompositorListener>
        gaussianNoiseCompositorListener;

      /// \brief Lens distortion model
      public: DistortionPtr distortion;

      /// \brief Queue of move positions.
      public: std::deque<std::pair<math::Pose, double> > moveToPositionQueue;

      /// \brief Render period.
      public: common::Time renderPeriod;

      /// \brief Position PID used to track a visual smoothly.
      public: common::PID trackVisualPID;

      /// \brief Pitch PID used to track a visual smoothly.
      public: common::PID trackVisualPitchPID;

      /// \brief Yaw PID used to track a visual smoothly.
      public: common::PID trackVisualYawPID;

      /// \brief Which noise type we support
      public: enum NoiseModelType
      {
        NONE,
        GAUSSIAN
      };

      /// \brief If true, apply the noise model specified by other
      /// noise parameters
      public: bool noiseActive;

      /// \brief Which type of noise we're applying
      public: enum NoiseModelType noiseType;

      /// \brief If noiseType==GAUSSIAN, noiseMean is the mean of the
      /// distibution from which we sample
      public: double noiseMean;

      /// \brief If noiseType==GAUSSIAN, noiseStdDev is the standard
      /// devation of the distibution from which we sample
      public: double noiseStdDev;

      /// \brief Which noise type we support
      public: enum DistortionModelType
      {
        BARREL
      };

      /// \brief Type of lens distortion we're applying
      public: DistortionModelType distortionType;

      /// \brief Radial distortion coefficients.
      public: math::Vector3 radialCoeff;

      /// \brief Tangential distortion coefficients.
      public: math::Vector2d tangentialCoeff;

      /// \brief Lens center used for distortion
      public: math::Vector2d lensCenter;

      /// \brief Scale applied to distorted image.
      public: math::Vector2d distortionScale;

      /// \brief True if the distorted image will be cropped to remove the
      /// black pixels at the corners of the image.
      public: bool distortionCrop;

      /// \brief Communication Node
      public: transport::NodePtr node;

      /// \brief Subscribe to camera command topic
      public: transport::SubscriberPtr cmdSub;

      /// \def CameraCmdMsgs_L
      /// \brief List for holding camera command messages.
      typedef std::list<boost::shared_ptr<msgs::CameraCmd const> >
        CameraCmdMsgs_L;

      /// \brief List of camera cmd messages.
      public: CameraCmdMsgs_L commandMsgs;

      /// \brief Mutex to lock the various message buffers.
      public: boost::mutex receiveMutex;
    };
  }
}
#endif
