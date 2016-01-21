/*
 * Copyright (C) 2012-2016 Open Source Robotics Foundation
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

#ifndef _GAZEBO_ANGLE_HH_
#define _GAZEBO_ANGLE_HH_

#include <math.h>
#include <ignition/math/Angle.hh>
#include <iostream>
#include "gazebo/util/system.hh"

/// \brief Macro that converts radians to degrees
/// \param[in] radians
/// \return degrees
#define GZ_RTOD(r) ((r) * 180 / M_PI)

/// \brief Converts degrees to radians
/// \param[in] degrees
/// \return radians
#define GZ_DTOR(d) ((d) * M_PI / 180)

/// \brief Macro tha normalizes an angle in the range -Pi to Pi
/// \param[in] angle
/// \return the angle, in range
#define GZ_NORMALIZE(a) (atan2(sin(a), cos(a)))

namespace gazebo
{
  /// \ingroup gazebo_math

  /// \brief Math namespace
  namespace math
  {
  /// \addtogroup gazebo_math Math
  /// \{

  /// \class Angle Angle.hh math/gzmath.hh
  /// \brief An angle and related functions.
  class GZ_MATH_VISIBLE Angle
  {
    /// \brief math::Angle(0)
    public: static const Angle Zero;

    /// \brief math::Angle(M_PI)
    public: static const Angle Pi;

    /// \brief math::Angle(M_PI * 0.5)
    public: static const Angle HalfPi;

    /// \brief math::Angle(M_PI * 2)
    public: static const Angle TwoPi;

    /// \brief Constructor
    public: Angle();

    /// \brief Copy Constructor
    /// \param[in] _radian Radians
    public: Angle(double _radian);

    /// \brief Copy constructor
    /// \param[in] _angle Angle to copy
    public: Angle(const Angle &_angle);

    /// \brief Ignition copy constructor
    /// \param[in] _angle Ignition angle to copy
    public: Angle(const ignition::math::Angle &_angle);

    /// \brief Destructor
    public: virtual ~Angle();

    /// \brief Set the value from an angle in radians
    /// \param[in] _radian Radian value
    public: void SetFromRadian(double _radian);

    /// \brief Set the value from an angle in degrees
    /// \param[in] _degree Degree value
    public: void SetFromDegree(double _degree);

    /// \brief Get the angle in radians
    /// \return double containing the angle's radian value
    public: double Radian() const;

    /// \brief Get the angle in degrees
    /// \return double containing the angle's degree value
    public: double Degree() const;

    /// \brief Normalize the angle in the range -Pi to Pi
    public: void Normalize();

    /// \brief Dereference operator
    /// \return Double containing the angle's radian value
    public: inline double operator*() const { return value; }
    /// \brief Substraction, result = this - _angle
    /// \param[in] _angle Angle for substraction
    /// \return the new angle
    public: Angle operator-(const Angle &_angle) const;

    /// \brief Addition operator, result = this + _angle
    /// \param[in] _angle Angle for addition
    /// \return the new angle
    public: Angle operator+(const Angle &_angle) const;

    /// \brief Multiplication operator, result = this * _angle
    /// \param[in] _angle Angle for multiplication
    /// \return the new angle
    public: Angle operator*(const Angle &_angle) const;

    /// \brief Division, result = this / _angle
    /// \param[in] _angle Angle for division
    /// \return the new angle
    public: Angle operator/(const Angle &_angle) const;

    /// \brief Subtraction set, this = this - _angle
    /// \param[in] _angle Angle for subtraction
    /// \return angle
    public: Angle operator-=(const Angle &_angle);

    /// \brief Addition set, this = this + _angle
    /// \param[in] _angle Angle for addition
    /// \return angle
    public: Angle operator+=(const Angle &_angle);

    /// \brief Multiplication set, this = this * _angle
    /// \param[in] _angle Angle for multiplication
    /// \return angle
    public: Angle operator*=(const Angle &_angle);

    /// \brief Division set, this = this / _angle
    /// \param[in] _angle Angle for division
    /// \return angle
    public: Angle operator/=(const Angle &_angle);

    /// \brief Convert this angle to an ignition::math::Angle.
    /// \return This Angle as an ignition::math::Angle.
    public: ignition::math::Angle Ign() const;

    /// \brief Assignment operator
    /// \param[in] _angle Radians
    /// \return The new angle
    public: Angle &operator=(const double &_angle);

    /// \brief Assignment operator
    /// \param[in] _angle ignition::math::Angle to copy
    /// \return The new angle
    public: Angle &operator=(const ignition::math::Angle &_angle);

    /// \brief Equality operator, result = this == _angle
    /// \param[in] _angle Angle to check for equality
    /// \return true if this == _angle
    public: bool operator ==(const Angle &_angle) const;

    /// \brief Inequality
    /// \param[in] _angle Angle to check for inequality
    /// \return true if this != _angle
    public: bool operator!=(const Angle &_angle) const;

    /// \brief Less than operator
    /// \param[in] _angle Angle to check
    /// \return true if this < _angle
    public: bool operator<(const Angle &_angle) const;

    /// \brief Less or equal operator
    /// \param[in] _angle Angle to check
    /// \return true if this <= _angle
    public: bool operator<=(const Angle &_angle) const;

    /// \brief Greater than operator
    /// \param[in] _angle Angle to check
    /// \return true if this > _angle
    public: bool operator>(const Angle &_angle) const;

    /// \brief Greater or equal operator
    /// \param[in] _angle Angle to check
    /// \return true if this >= _angle
    public: bool operator>=(const Angle &_angle) const;

    /// \brief Stream insertion operator. Outputs in degrees
    /// \param[in] _out output stream
    /// \param[in] _a angle to output
    /// \return The output stream
    public: friend std::ostream &operator<<(std::ostream &_out,
                                            const gazebo::math::Angle &_a)
    {
      _out << _a.Radian();
      return _out;
    }

    /// \brief Stream extraction operator. Assumes input is in degrees
    /// \param in input stream
    /// \param pt angle to read value into
    /// \return The input stream
    public: friend std::istream &operator>>(std::istream &_in,
                                            gazebo::math::Angle &_a)
    {
      // Skip white spaces
      _in.setf(std::ios_base::skipws);
      _in >> _a.value;
      return _in;
    }

    /// The angle in radians
    private: double value;
  };

  /// \}
  }
}

#endif



