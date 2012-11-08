/*
 * Copyright 2011 Nate Koenig
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
#ifndef _INERTIAL_HH_
#define _INERTIAL_HH_

#include <string>

#include "gazebo/msgs/msgs.hh"
#include "gazebo/sdf/sdf.hh"
#include "gazebo/math/Quaternion.hh"
#include "gazebo/math/Vector3.hh"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{

    /// \class Inertial Inertial.hh physics/physics.hh
    /// \brief A class for inertial information about a link
    class Inertial
    {
      /// \brief Default Constructor
      public: Inertial();

      /// \brief Constructor.
      /// \param[in] _mass Mass value in kg if using metric.
      public: explicit Inertial(double _mass);

      /// \brief Copy constructor.
      /// \param[in] _inertial Inertial element to copy
      public: explicit Inertial(const Inertial &_inertial);

      /// \brief Destructor.
      public: virtual ~Inertial();

      /// \brief Load from SDF values.
      /// \param[in] _sdf SDF value to load from.
      public: void Load(sdf::ElementPtr _sdf);

      /// \brief update the parameters using new sdf values.
      /// \param[in] _sdf Update values from.
      public: void UpdateParameters(sdf::ElementPtr _sdf);

      /// \brief Reset all the mass properties.
      public: void Reset();

      /// \brief Set the mass.
      public: void SetMass(double m);

      /// \brief Get the mass
      public: double GetMass() const;

      /// \brief Set the mass matrix.
      /// \param[in] _ixx X second moment of inertia about x axis.
      /// \param[in] _iyy Y second moment of inertia about y axis.
      /// \param[in] _izz Z second moment of inertia about z axis.
      /// \param[in] _ixy XY inertia.
      /// \param[in] _ixz XZ inertia.
      /// \param[in] _iyz YZ inertia.
      public: void SetInertiaMatrix(double _ixx, double _iyy, double _izz,
                                     double _ixy, double _ixz, double iyz);

      /// \brief Set the center of gravity.
      /// \param[in] _cx X position.
      /// \param[in] _cy Y position.
      /// \param[in] _cz Z position.
      public: void SetCoG(double _cx, double _cy, double _cz);

      /// \brief Set the center of gravity.
      /// \param[in] _center Center of the gravity.
      public: void SetCoG(const math::Vector3 &_center);

      /// \brief Get the center of gravity.
      /// \return The cneter of gravity.
      public: inline const math::Vector3 &GetCoG() const
              {return this->cog;}

      /// \brief Get the pose about which the mass and inertia matrix is
      /// specified in the Link frame.
      /// \return The inertial pose.
      public: inline const math::Pose GetPose() const
              { return math::Pose(this->cog, math::Quaternion());}

      /// \brief Get the principal moments of inertia (Ixx, Iyy, Izz).
      /// \return The principal moments.
      public: math::Vector3 GetPrincipalMoments() const;

      /// \brief Get the products of inertia (Ixy, Ixy, Iyz).
      /// \return The products of inertia.
      public: math::Vector3 GetProductsofInertia() const;

      /// \brief Get IXX
      /// \return IXX value
      public: double GetIXX() const;

      /// \brief Get IYY
      /// \return IYY value
      public: double GetIYY() const;

      /// \brief Get IZZ
      /// \return IZZ value
      public: double GetIZZ() const;

      /// \brief Get IXY
      /// \return IXY value
      public: double GetIXY() const;

      /// \brief Get IXZ
      /// \return IXZ value
      public: double GetIXZ() const;

      /// \brief Get IXZ
      /// \return IYZ value
      public: double GetIYZ() const;

      /// \brief Set IXX
      /// \param[in] _v IXX value
      public: void SetIXX(double _v);

      /// \brief Set IYY
      /// \param[in] _v IYY value
      public: void SetIYY(double _v);

      /// \brief Set IZZ
      /// \param[in] _v IZZ value
      public: void SetIZZ(double _v);

      /// \brief Set IXY
      /// \param[in] _v IXY value
      public: void SetIXY(double _v);

      /// \brief Set IXZ
      /// \param[in] _v IXZ value
      public: void SetIXZ(double _v);

      /// \brief Set IYZ
      /// \param[in] _v IXX value
      public: void SetIYZ(double _v);

      /// \brief Rotate this mass.
      /// \param[in] _rot Rotation amount.
      public: void Rotate(const math::Quaternion &rot);

      /// \brief Equal operator.
      /// \param[in] _inertial Inertial to copy.
      /// \return Reference to this object.
      public: Inertial &operator=(const Inertial &_inertial);

      /// \brief Addition operator.
      /// \param[in] _inertial Inertial to add.
      /// \return The result of the addition.
      public: Inertial operator+(const Inertial &_inertial) const;

      /// \brief Addition equal operator.
      /// \param[in] _inertial Inertial to add.
      /// \return Reference to this object.
      public: const Inertial &operator+=(const Inertial &_inertial);

      /// \brief Update parameters from a message
      /// \param[in] _msg Message to read
      public: void ProcessMsg(const msgs::Inertial &_msg);

      /// \brief Output operator.
      /// \param[in] _out Output stream.
      /// \param[in] _inertial Inertial object to output.
      public: friend std::ostream &operator<<(std::ostream &_out,
                  const gazebo::physics::Inertial &_inertial)
              {
                _out << "Mass[" << _inertial.mass << "] CoG["
                    << _inertial.cog << "]\n";
                _out << "IXX[" << _inertial.principals.x << "] "
                     << "IYY[" << _inertial.principals.y << "] "
                     << "IZZ[" << _inertial.principals.z << "]\n";
                _out << "IXY[" << _inertial.products.x << "] "
                     << "IXZ[" << _inertial.products.y << "] "
                     << "IYZ[" << _inertial.products.z << "]\n";
                return _out;
              }

      /// \brief Mass the object. Default is 1.0.
      private: double mass;

      /// \brief Center of gravity. Default is (0.0 0.0 0.0)
      private: math::Vector3 cog;

      /// \brief Principal moments of inertia. Default is (1.0 1.0 1.0)
      private: math::Vector3 principals;

      /// \brief Product moments of inertia. Default is (0.0 0.0 0.0)
      private: math::Vector3 products;

      /// \brief Our SDF values.
      private: sdf::ElementPtr sdf;
    };
    /// \}
  }
}
#endif
