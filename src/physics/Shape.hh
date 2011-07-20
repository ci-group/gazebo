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
#ifndef SHAPE_HH
#define SHAPE_HH

#include <string>

#include "common/CommonTypes.hh"
#include "physics/PhysicsTypes.hh"

#include "physics/Base.hh"

namespace gazebo
{
	namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{

    /// \brief Base class for all shapes
    class Shape : public Base
    {
      /// \brief Constructor
      public: Shape(GeomPtr p);
  
      /// \brief Destructor
      public: virtual ~Shape();
    
      /// \brief Load the shape
      public: virtual void Load( sdf::ElementPtr _sdf );

      /// \brief Initialize the shape
      public: virtual void Init() = 0;

      protected: GeomPtr geomParent;
      protected: sdf::ElementPtr sdf;
    };
    /// \}
  }
}
#endif
