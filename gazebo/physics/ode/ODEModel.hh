/*
 * Copyright (C) 2012-2015 Open Source Robotics Foundation
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
/* Desc: ODE Link class
 * Author: Elte Hupkes
 */
#ifndef _ODEMODEL_HH_
#define _ODEMODEL_HH_

#include "gazebo/physics/ode/ode_inc.h"
#include "gazebo/physics/ode/ODETypes.hh"
#include "gazebo/physics/Model.hh"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{

    /// \class ODEModel OdeModel.hh physics/physics.hh
    /// \brief A model is a collection of links, joints, and plugins.
    class GZ_PHYSICS_ODE_VISIBLE ODEModel : public Model
    {
      /// \brief Constructor.
      /// \param[in] _parent Parent object.
      /// \param[in] _parentSpaceId ID of the parent collision space
      public: explicit ODEModel(BasePtr _parent, dSpaceID _parentSpaceId);

      /// \brief Destructor.
      public: virtual ~ODEModel();

      /// \brief Get the ID of the collision space for this model
      /// \return The collision space ID for this model.
      public: dSpaceID GetSpaceId();

      /// \brief The collision space for this model
      private: dSpaceID spaceId;
    };
    /// \}
  }
}

#endif // _ODEMODEL_HH_
