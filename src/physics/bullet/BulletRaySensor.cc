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
/* Desc: Bullet ray sensor
 * Author: Nate Koenig
 * Date: 21 May 2009
 * SVN: $Id:$
 */

#include "common/Global.hh"
#include "World.hh"

#include "BulletRayCollision.hh"
#include "BulletPhysics.hh"
#include "BulletLink.hh"
#include "BulletRaySensor.hh"

using namespace gazebo;
using namespace physics;

using namespace physics;

using namespace physics;


////////////////////////////////////////////////////////////////////////////////
/// Constructor
BulletRaySensor::BulletRaySensor(Link *_body)
  : PhysicsRaySensor(_body)
{
  this->body = dynamic_cast<BulletLink*>(_body);

  if (this->body == NULL)
    gzthrow("BulletRaySensor requires an BulletLink");
}

////////////////////////////////////////////////////////////////////////////////
/// Destructor
BulletRaySensor::~BulletRaySensor()
{
  std::vector<BulletRayCollision*>::iterator iter;

  for (iter = this->rays.begin(); iter != this->rays.end(); iter++)
  {
    delete (*iter);
  }
  this->rays.clear();
}

////////////////////////////////////////////////////////////////////////////////
/// Add a ray to the sensor
void BulletRaySensor::AddRay(math::Vector3 start, math::Vector3 end, double minRange, 
                          double maxRange, bool display)
{
  BulletRayCollision *rayCollision;
  
  rayCollision = (BulletRayCollision*)this->GetWorld()->CreateCollision("ray", this->body );
  rayCollision->SetDisplayRays(display);
  rayCollision->SetMinLength(minRange);
  rayCollision->SetMaxLength(maxRange);

  rayCollision->SetPoints(start,end);
  this->rays.push_back(rayCollision);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the number of rays
int BulletRaySensor::GetCount() const
{
  return this->rays.size();
}

////////////////////////////////////////////////////////////////////////////////
/// Get the relative starting and ending points of a ray
void BulletRaySensor::GetRelativePoints(int index, math::Vector3 &a, math::Vector3 &b)
{
  if (index <0 || index >= (int)(this->rays.size()))
  {
    std::ostringstream stream;
    stream << "index[" << index << "] is out of range[0-" 
           << this->GetCount() << "]";
    gzthrow(stream.str());
  }

  this->rays[index]->GetRelativePoints(a,b);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the range of a ray
double BulletRaySensor::GetRange(int index) const
{
  if (index <0 || index >= (int)(this->rays.size()))
  {
    std::ostringstream stream;
    stream << "index[" << index << "] is out of range[0-" 
           << this->GetCount() << "]";
    gzthrow(stream.str());
  }

  return this->rays[index]->GetLength();
}

////////////////////////////////////////////////////////////////////////////////
/// Get the retro reflectance value of a ray
double BulletRaySensor::GetRetro(int index) const
{
  if (index <0 || index >= (int)(this->rays.size()))
  {
    std::ostringstream stream;
    stream << "index[" << index << "] is out of range[0-" 
           << this->GetCount() << "]";
    gzthrow(stream.str());
  }

  return this->rays[index]->GetRetro();
}

////////////////////////////////////////////////////////////////////////////////
/// Get the fiducial value of a ray
double BulletRaySensor::GetFiducial(int index) const
{
  if (index <0 || index >= (int)(this->rays.size()))
  {
    std::ostringstream stream;
    stream << "index[" << index << "] is out of range[0-" 
           << this->GetCount() << "]";
    gzthrow(stream.str());
  }

  return this->rays[index]->GetFiducial();
}

////////////////////////////////////////////////////////////////////////////////
/// Update the ray sensor
void BulletRaySensor::Update()
{
  std::vector<BulletRayCollision*>::iterator iter;

  for (iter = this->rays.begin(); iter != this->rays.end(); iter++)
  {
    (*iter)->SetLength( (*iter)->GetMaxLength() );
    (*iter)->SetRetro( 0.0 );
    (*iter)->SetFiducial( -1 );

    // Get the global points of the line
    (*iter)->Update();
  }
}
