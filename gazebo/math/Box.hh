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
#ifndef _BOX_HH_
#define _BOX_HH_

#include <iostream>
#include "gazebo/math/Vector3.hh"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace math
  {
    /// \addtogroup gazebo_math
    /// \{

    /// \class Box Box.hh math/gzmath.hh
    /// \brief Mathematical representation of a box and related functions.
    class GAZEBO_VISIBLE Box
    {
      /// \brief Default constructor
      public: Box();

      /// \brief Constructor
      /// \param[in] _min Minimum corner of the box
      /// \param[in] _max Maximum corner of the box
      public: Box(const Vector3 &_min, const Vector3 &_max);

      /// \brief Copy Constructor
      /// \param[in]  _b Box to copy
      public: Box(const Box &_b);

      /// \brief Destructor
      public: virtual ~Box();

      /// \brief Get the length along the x dimension
      /// \return Double value of the length in the x dimension
      public: double GetXLength() const;

      /// \brief Get the length along the y dimension
      /// \return Double value of the length in the y dimension
      public: double GetYLength() const;

      /// \brief Get the length along the z dimension
      /// \return Double value of the length in the z dimension
      public: double GetZLength() const;

      /// \brief Get the size of the box
      /// \return Size of the box
      public: math::Vector3 GetSize() const;

      /// \brief Get the box center
      /// \return The center position of the box
      public: math::Vector3 GetCenter() const;

      /// \brief Merge a box with this box
      /// \param[in]  _box Box to add to this box
      public: void Merge(const Box &_box);

      /// \brief Assignment operator. Set this box to the parameter
      /// \param[in]  _b Box to copy
      /// \return The new box.
      public: Box &operator =(const Box &_b);

      /// \brief Addition operator. result = this + _b
      /// \param[in] _b Box to add
      /// \return The new box
      public: Box operator+(const Box &_b) const;

      /// \brief Addition set operator. this = this + _b
      /// \param[in] _b Box to add
      /// \return This new box
      public: const Box &operator+=(const Box &_b);

      /// \brief Equality test operatoer
      /// \param[in] _b Box to test
      /// \return True if equal
      public: bool operator==(const Box &_b) const;

      /// \brief Subtract a vector from the min and max values
      /// \param _v The vector to use during subtraction
      /// \return The new box
      public: Box operator-(const Vector3 &_v);

      /// \brief Output operator
      /// \param[in] _out Output stream
      /// \param[in] _b Box to output to the stream
      /// \return The stream
      public: friend std::ostream &operator<<(std::ostream &_out,
                                               const gazebo::math::Box &_b)
      {
        _out << "Min[" << _b.min << "] Max[" << _b.max << "]";

        return _out;
      }

      /// \brief Minimum corner of the box
      public: Vector3 min;

      /// \brief Maximum corner of the box
      public: Vector3 max;

      /// \brief Enumeration of extents
      private: enum Extent {EXTENT_NULL, EXTENT_FINITE};

      /// \brief When set to EXTENT_NULL (in the default constructor)
      /// the min and max are not valid positions
      private: Extent extent;
    };
    /// \}
  }
}

#endif



