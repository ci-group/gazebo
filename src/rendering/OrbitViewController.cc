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
#include "common/MouseEvent.hh"
#include "math/Vector2i.hh"
#include "math/Angle.hh"

#include "rendering/Scene.hh"
#include "rendering/Visual.hh"
#include "rendering/UserCamera.hh"
#include "rendering/OrbitViewController.hh"

#define TYPE_STRING "OrbitViewController"
#define MIN_DISTANCE 0.01

using namespace gazebo;
using namespace rendering;


static const float PITCH_LIMIT_LOW = -M_PI*0.5 + 0.001;
static const float PITCH_LIMIT_HIGH = M_PI*0.5 - 0.001;

//////////////////////////////////////////////////
OrbitViewController::OrbitViewController(UserCamera *_camera)
  : ViewController(_camera), distance(5.0f)
{
  this->minDist = MIN_DISTANCE;
  this->maxDist = 0;
  this->typeString = TYPE_STRING;

  this->refVisual.reset(new Visual("OrbitViewController",
                        this->camera->GetScene()->GetWorldVisual()));
  this->refVisual->Init();
  this->refVisual->AttachMesh("unit_sphere");
  this->refVisual->SetScale(math::Vector3(0.2, 0.2, 0.1));
  this->refVisual->SetCastShadows(false);
  this->refVisual->SetMaterial("Gazebo/YellowTransparent");
  this->refVisual->SetVisible(false);
  this->refVisual->SetWorldPosition(this->focalPoint);
  this->refVisual->SetVisibilityFlags(GZ_VISIBILITY_GUI);
}

//////////////////////////////////////////////////
OrbitViewController::~OrbitViewController()
{
  this->refVisual.reset();
}

//////////////////////////////////////////////////
void OrbitViewController::Init(const math::Vector3 &_focalPoint)
{
  math::Vector3 rpy = this->camera->GetWorldPose().rot.GetAsEuler();
  this->yaw = rpy.z;
  this->pitch = rpy.y;

  this->focalPoint = _focalPoint;
  this->distance = this->camera->GetWorldPosition().Distance(this->focalPoint);

  this->refVisual->SetVisible(false);
  this->refVisual->SetWorldPosition(this->focalPoint);
}

//////////////////////////////////////////////////
void OrbitViewController::SetDistanceRange(double _minDist, double _maxDist)
{
  this->minDist = _minDist;
  this->maxDist = _maxDist;
  if (this->distance > this->maxDist)
    this->distance = this->maxDist;
  if (this->distance < this->minDist)
    this->distance = this->minDist;
}

//////////////////////////////////////////////////
void OrbitViewController::Init()
{
  double dist = -1;
  math::Vector3 fp;

  // Try to get a point on a plane to use as the reference point
  int width = this->camera->GetViewportWidth();
  int height = this->camera->GetViewportHeight();
  if (this->camera->GetWorldPointOnPlane(width/2.0, height/2.0,
        math::Plane(math::Vector3(0, 0, 1)), fp))
  {
    dist = this->camera->GetWorldPosition().Distance(fp);
  }

  // If the plane is too far away.
  if (dist < 0 || dist > 20 || isnan(dist))
  {
    // First, see if the camera is looking at the origin.
    math::Vector3 dir = this->camera->GetDirection();
    dir.Normalize();
    math::Vector3 origin(0, 0, 0);
    math::Vector3 cameraPos = this->camera->GetWorldPose().pos;
    double distOrigin = cameraPos.Distance(origin);

    dist = origin.GetDistToLine(cameraPos, cameraPos + dir * distOrigin);

    if (math::equal(dist, 0, 1e-3))
      dist = distOrigin;
    else
    {
      // If camera is not looking at the origin, see if the camera's
      // direction projected on the ground plane interescts the origin.
      // Otherwise, choose a default distance of 10m for the focal point
      cameraPos.z = 0;
      distOrigin = cameraPos.Distance(origin);
      dist = origin.GetDistToLine(cameraPos, cameraPos + dir * distOrigin);
      if (math::equal(dist, 0, 1e-3))
        dist = distOrigin;
      else
        dist = 10;

      cameraPos = this->camera->GetWorldPose().pos;
    }
    if (dist > 10)
      dist = 10.0;
    fp = cameraPos + dir * dist;
  }

  fp.Correct();
  this->Init(fp);
}

//////////////////////////////////////////////////
void OrbitViewController::Update()
{
}

//////////////////////////////////////////////////
void OrbitViewController::HandleMouseEvent(const common::MouseEvent &_event)
{
  if (!this->enabled)
    return;

  math::Vector2i drag = _event.pos - _event.prevPos;

  math::Vector3 directionVec(0, 0, 0);

  if (_event.buttons & common::MouseEvent::LEFT)
  {
    this->refVisual->SetVisible(true);
    this->yaw += drag.x * _event.moveScale * -0.2;
    this->pitch += drag.y * _event.moveScale * 0.2;

    this->NormalizeYaw(this->yaw);
    this->NormalizePitch(this->pitch);
  }
  else if (_event.type == common::MouseEvent::SCROLL)
  {
    this->refVisual->SetVisible(false);

    // This assumes that _event.scroll.y is -1 or +1
    if (_event.shift)
    {
      this->Translate(math::Vector3(
          -(_event.scroll.y*20) * _event.moveScale * (this->distance / 10.0),
          0, 0));
    }
    else
    {
      this->Zoom(
          -(_event.scroll.y*20) * _event.moveScale * (this->distance / 10.0));
    }
  }
  else if (_event.buttons & common::MouseEvent::RIGHT)
  {
    this->refVisual->SetVisible(true);
    if (_event.shift)
    {
      this->Translate(math::Vector3(
          -drag.y * _event.moveScale * (this->distance / 10.0), 0, 0));
    }
    else
    {
      this->Zoom(-drag.y * _event.moveScale * (this->distance / 10.0));
    }
  }
  else if (_event.buttons & common::MouseEvent::MIDDLE)
  {
    this->refVisual->SetVisible(true);
    double fovY = this->camera->GetVFOV().GetAsRadian();
    double fovX = 2.0f * atan(tan(fovY / 2.0f) *
                  this->camera->GetAspectRatio());

    int width = this->camera->GetViewportWidth();
    int height = this->camera->GetViewportHeight();

    this->Translate(math::Vector3(0.0,
          (drag.x / static_cast<float>(width)) *
          this->distance * tan(fovX / 2.0) * 2.0,
          (drag.y / static_cast<float>(height)) *
          this->distance * tan(fovY / 2.0) * 2.0));
  }
  else
    this->refVisual->SetVisible(false);

  this->UpdatePose();
}

//////////////////////////////////////////////////
void OrbitViewController::Translate(math::Vector3 vec)
{
  this->focalPoint += this->camera->GetWorldPose().rot * vec;
  this->refVisual->SetWorldPosition(this->focalPoint);
}

//////////////////////////////////////////////////
void OrbitViewController::SetDistance(float _d)
{
  this->distance = _d;
}

//////////////////////////////////////////////////
void OrbitViewController::SetFocalPoint(const math::Vector3 &_fp)
{
  this->focalPoint = _fp;
  this->refVisual->SetWorldPosition(this->focalPoint);
}

//////////////////////////////////////////////////
math::Vector3 OrbitViewController::GetFocalPoint() const
{
  return this->focalPoint;
}

//////////////////////////////////////////////////
void OrbitViewController::SetYaw(double _yaw)
{
  this->yaw = _yaw;
}

//////////////////////////////////////////////////
void OrbitViewController::SetPitch(double _pitch)
{
  this->pitch = _pitch;
}


//////////////////////////////////////////////////
void OrbitViewController::NormalizeYaw(float &v)
{
  v = fmod(v, M_PI*2);
  if (v < 0.0f)
  {
    v = M_PI * 2 + v;
  }
}

//////////////////////////////////////////////////
void OrbitViewController::NormalizePitch(float &v)
{
  if (v < PITCH_LIMIT_LOW)
    v = PITCH_LIMIT_LOW;
  else if (v > PITCH_LIMIT_HIGH)
    v = PITCH_LIMIT_HIGH;
}

//////////////////////////////////////////////////
void OrbitViewController::Zoom(float _amount)
{
  this->distance -= _amount;
  if (this->distance <= this->minDist)
    this->distance = this->minDist;
  if (this->maxDist > 0 && this->distance >= this->maxDist)
    this->distance = this->maxDist;
}

//////////////////////////////////////////////////
std::string OrbitViewController::GetTypeString()
{
  return TYPE_STRING;
}


//////////////////////////////////////////////////
void OrbitViewController::UpdatePose()
{
  math::Vector3 pos;

  pos.x = this->distance * -cos(this->yaw) * cos(this->pitch);
  pos.y = this->distance * -sin(this->yaw) * cos(this->pitch);
  pos.z = this->distance * sin(this->pitch);

  pos += this->focalPoint;

  this->camera->SetWorldPosition(pos);

  math::Quaternion rot;
  math::Vector3 rpy(0, this->pitch, this->yaw);
  rot.SetFromEuler(rpy);
  this->camera->SetWorldRotation(rot);
}
