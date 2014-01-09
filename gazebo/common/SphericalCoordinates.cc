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

#include <string>
#include <math.h>

#include "gazebo/common/Console.hh"
#include "gazebo/common/SphericalCoordinates.hh"
#include "gazebo/common/SphericalCoordinatesPrivate.hh"


using namespace gazebo;
using namespace common;

// Parameters for EARTH_WGS84 model
// a: Semi-major equatorial axis (meters)
// b: Semi-minor polar axis (meters)
// if: inverse flattening (no units)
// wikipedia: World_Geodetic_System#A_new_World_Geodetic_System:_WGS_84
const double g_EarthWGS84AxisEquatorial = 6378137.0;
const double g_EarthWGS84AxisPolar = 6356752.314245;

const double g_EarthSphere = 6371000.0;
// const double g_EarthWGS84Flattening = 1/298.257223563;

//////////////////////////////////////////////////
SphericalCoordinates::SurfaceType SphericalCoordinates::Convert(
  const std::string &_str)
{
  if ("EARTH_WGS84" == _str)
    return EARTH_WGS84;

  gzerr << "SurfaceType string not recognized, "
        << "EARTH_WGS84 returned by default" << std::endl;
  return EARTH_WGS84;
}

//////////////////////////////////////////////////
SphericalCoordinates::SphericalCoordinates()
  : dataPtr(new SphericalCoordinatesPrivate)
{
  this->SetSurfaceType(EARTH_WGS84);
  this->SetElevationReference(0.0);
}

//////////////////////////////////////////////////
SphericalCoordinates::SphericalCoordinates(const SurfaceType _type)
  : dataPtr(new SphericalCoordinatesPrivate)
{
  this->SetSurfaceType(_type);
  this->SetElevationReference(0.0);
}

//////////////////////////////////////////////////
SphericalCoordinates::SphericalCoordinates(const SurfaceType _type,
                                           const math::Angle &_latitude,
                                           const math::Angle &_longitude,
                                           double _elevation,
                                           const math::Angle &_heading)
  : dataPtr(new SphericalCoordinatesPrivate)
{
  this->SetSurfaceType(_type);
  this->SetLatitudeReference(_latitude);
  this->SetLongitudeReference(_longitude);
  this->SetElevationReference(_elevation);
  this->SetHeadingOffset(_heading);
}

//////////////////////////////////////////////////
SphericalCoordinates::~SphericalCoordinates()
{
  delete this->dataPtr;
  this->dataPtr = NULL;
}

//////////////////////////////////////////////////
SphericalCoordinates::SurfaceType SphericalCoordinates::GetSurfaceType() const
{
  return this->dataPtr->surfaceType;
}

//////////////////////////////////////////////////
math::Angle SphericalCoordinates::GetLatitudeReference() const
{
  return this->dataPtr->latitudeReference;
}

//////////////////////////////////////////////////
math::Angle SphericalCoordinates::GetLongitudeReference() const
{
  return this->dataPtr->longitudeReference;
}

//////////////////////////////////////////////////
double SphericalCoordinates::GetElevationReference() const
{
  return this->dataPtr->elevationReference;
}

//////////////////////////////////////////////////
math::Angle SphericalCoordinates::GetHeadingOffset() const
{
  return this->dataPtr->headingOffset;
}

//////////////////////////////////////////////////
void SphericalCoordinates::SetSurfaceType(const SurfaceType &_type)
{
  this->dataPtr->surfaceType = _type;
}

//////////////////////////////////////////////////
void SphericalCoordinates::SetLatitudeReference(const math::Angle &_angle)
{
  this->dataPtr->latitudeReference.SetFromRadian(_angle.Radian());
}

//////////////////////////////////////////////////
void SphericalCoordinates::SetLongitudeReference(const math::Angle &_angle)
{
  this->dataPtr->longitudeReference.SetFromRadian(_angle.Radian());
}

//////////////////////////////////////////////////
void SphericalCoordinates::SetElevationReference(double _elevation)
{
  this->dataPtr->elevationReference = _elevation;
}

//////////////////////////////////////////////////
void SphericalCoordinates::SetHeadingOffset(const math::Angle &_angle)
{
  this->dataPtr->headingOffset.SetFromRadian(_angle.Radian());
}

//////////////////////////////////////////////////
math::Vector3 SphericalCoordinates::SphericalFromLocal(
    const math::Vector3 &_xyz) const
{
  double radiusMeridional = 1.0;
  double radiusNormal = 1.0;
  double headingSine = sin(this->dataPtr->headingOffset.Radian());
  double headingCosine = cos(this->dataPtr->headingOffset.Radian());

  switch (this->dataPtr->surfaceType)
  {
    case EARTH_WGS84:
      // Currently uses radius of curvature equations from wikipedia
      // http://en.wikipedia.org/wiki/Earth_radius#Radius_of_curvature
      {
        double a = g_EarthWGS84AxisEquatorial;
        double b = g_EarthWGS84AxisPolar;
        double ab = a*b;
        double cosLat = cos(this->dataPtr->latitudeReference.Radian());
        double sinLat = sin(this->dataPtr->latitudeReference.Radian());
        double denom = (a*cosLat)*(a*cosLat) + (b*sinLat)*(b*sinLat);
        radiusMeridional = ab*ab / denom / sqrt(denom);
        radiusNormal = a*a / sqrt(denom);
      }
      break;

    default:
      break;
  }

  math::Vector3 spherical;
  double east  = _xyz.x * headingCosine - _xyz.y * headingSine;
  double north = _xyz.x * headingSine   + _xyz.y * headingCosine;
  // Assumes small changes in latitude / longitude.
  // May not work well near the north / south poles.
  math::Angle deltaLatitude(north / radiusMeridional);
  math::Angle deltaLongitude(east / radiusNormal);
  // geodetic latitude in degrees
  spherical.x = this->dataPtr->latitudeReference.Degree() +
                deltaLatitude.Degree();
  // geodetic longitude in degrees
  spherical.y = this->dataPtr->longitudeReference.Degree() +
                deltaLongitude.Degree();
  // altitude relative to sea level
  spherical.z = this->dataPtr->elevationReference + _xyz.z;
  return spherical;
}

//////////////////////////////////////////////////
math::Vector3 SphericalCoordinates::GlobalFromLocal(const math::Vector3 &_xyz)
    const
{
  double headingSine = sin(this->dataPtr->headingOffset.Radian());
  double headingCosine = cos(this->dataPtr->headingOffset.Radian());
  double east  = _xyz.x * headingCosine - _xyz.y * headingSine;
  double north = _xyz.x * headingSine   + _xyz.y * headingCosine;
  return math::Vector3(east, north, _xyz.z);
}

//////////////////////////////////////////////////
/// Based on Haversine formula (http://en.wikipedia.org/wiki/Haversine_formula).
double SphericalCoordinates::Distance(const math::Angle &_latA,
                                      const math::Angle &_lonA,
                                      const math::Angle &_latB,
                                      const math::Angle &_lonB)
{
  math::Angle dLat = _latB - _latA;
  math::Angle dLon = _lonB - _lonA;
  double a = sin(dLat.Radian() / 2) * sin(dLat.Radian() / 2) +
             sin(dLon.Radian() / 2) * sin(dLon.Radian() / 2) *
             cos(_latA.Radian()) * cos(_latB.Radian());
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));
  double d = g_EarthSphere * c;
  return d;
}
