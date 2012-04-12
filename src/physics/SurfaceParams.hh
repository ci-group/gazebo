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
/* Desc: common::Parameters for a surface
 * Author: Nate Koenig
 * Date: 30 July 2003
 */

#ifndef SURFACEPARAMS_HH
#define SURFACEPARAMS_HH

#include "msgs/msgs.h"
#include "sdf/sdf.h"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{
    /// \brief Surface params
    class SurfaceParams
    {
      /// \brief Constructor
      public: SurfaceParams();

      /// \brief Constructor
      public: virtual ~SurfaceParams();

      /// \brief Load the contact params
      public: virtual void Load(sdf::ElementPtr _sdf);

      /// \brief Fill in a surface message
      public: void FillSurfaceMsg(msgs::Surface &_msg);

      public: virtual void ProcessMsg(const msgs::Surface &_msg);

      /// 0..1, 0 = no bounciness
      public: double bounce;

      /// \brief bounce vel
      public: double bounceThreshold;

      public: double kp;
      public: double kd;

      public: double cfm;
      public: double erp;
      public: double maxVel;
      public: double minDepth;
      public: double mu1;
      public: double mu2;
      public: double slip1;
      public: double slip2;
      public: math::Vector3 fdir1;
    };
    /// \}
  }
}
#endif


