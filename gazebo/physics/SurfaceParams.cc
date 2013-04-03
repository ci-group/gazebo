/*
 * Copyright 2012 Open Source Robotics Foundation
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
/* Desc: common::Parameters for contact joints
 * Author: Nate Koenig
 * Date: 30 July 2003
 */

#include <float.h>
#include "gazebo/common/Assert.hh"
#include "gazebo/physics/SurfaceParams.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
SurfaceParams::SurfaceParams()
  : collideWithoutContact(false)
{
}

//////////////////////////////////////////////////
SurfaceParams::~SurfaceParams()
{
}

//////////////////////////////////////////////////
void SurfaceParams::Load(sdf::ElementPtr _sdf)
{
  GZ_ASSERT(_sdf, "Surface _sdf is NULL");
  {
    sdf::ElementPtr bounceElem = _sdf->GetElement("bounce");
    GZ_ASSERT(bounceElem, "Surface sdf member is NULL");
    this->bounce = bounceElem->GetValueDouble("restitution_coefficient");
    this->bounceThreshold = bounceElem->GetValueDouble("threshold");
  }

  {
    sdf::ElementPtr frictionElem = _sdf->GetElement("friction");
    GZ_ASSERT(frictionElem, "Surface sdf member is NULL");
    {
      sdf::ElementPtr frictionOdeElem = frictionElem->GetElement("ode");
      GZ_ASSERT(frictionOdeElem, "Surface sdf member is NULL");
      this->mu1 = frictionOdeElem->GetValueDouble("mu");
      this->mu2 = frictionOdeElem->GetValueDouble("mu2");

      if (this->mu1 < 0)
        this->mu1 = FLT_MAX;
      if (this->mu2 < 0)
        this->mu2 = FLT_MAX;

      this->slip1 = frictionOdeElem->GetValueDouble("slip1");
      this->slip2 = frictionOdeElem->GetValueDouble("slip2");
      this->fdir1 = frictionOdeElem->GetValueVector3("fdir1");
    }
  }

  {
    sdf::ElementPtr contactElem = _sdf->GetElement("contact");
    GZ_ASSERT(contactElem, "Surface sdf member is NULL");
    {
      this->collideWithoutContact =
        contactElem->GetValueBool("collide_without_contact");
      sdf::ElementPtr contactOdeElem = contactElem->GetElement("ode");
      GZ_ASSERT(contactOdeElem, "Surface sdf member is NULL");
      this->kp = contactOdeElem->GetValueDouble("kp");
      this->kd = contactOdeElem->GetValueDouble("kd");
      this->cfm = contactOdeElem->GetValueDouble("soft_cfm");
      this->erp = contactOdeElem->GetValueDouble("soft_erp");
      this->maxVel = contactOdeElem->GetValueDouble("max_vel");
      this->minDepth = contactOdeElem->GetValueDouble("min_depth");
    }
  }
}

/////////////////////////////////////////////////
void SurfaceParams::FillMsg(msgs::Surface &_msg)
{
  _msg.mutable_friction()->set_mu(this->mu1);
  _msg.mutable_friction()->set_mu2(this->mu2);
  _msg.mutable_friction()->set_slip1(this->slip1);
  _msg.mutable_friction()->set_slip2(this->slip2);
  msgs::Set(_msg.mutable_friction()->mutable_fdir1(), this->fdir1);

  _msg.set_restitution_coefficient(this->bounce);
  _msg.set_bounce_threshold(this->bounceThreshold);

  _msg.set_soft_cfm(this->cfm);
  _msg.set_soft_erp(this->erp);
  _msg.set_kp(this->kp);
  _msg.set_kd(this->kd);
  _msg.set_max_vel(this->maxVel);
  _msg.set_min_depth(this->minDepth);
  _msg.set_collide_without_contact(this->collideWithoutContact);
}


void SurfaceParams::ProcessMsg(const msgs::Surface &_msg)
{
  if (_msg.has_friction())
  {
    if (_msg.friction().has_mu())
      this->mu1 = _msg.friction().mu();
    if (_msg.friction().has_mu2())
      this->mu2 = _msg.friction().mu2();
    if (_msg.friction().has_slip1())
      this->slip1 = _msg.friction().slip1();
    if (_msg.friction().has_slip2())
      this->slip2 = _msg.friction().slip2();
    if (_msg.friction().has_fdir1())
      this->fdir1 = msgs::Convert(_msg.friction().fdir1());

    if (this->mu1 < 0)
      this->mu1 = FLT_MAX;
    if (this->mu2 < 0)
      this->mu2 = FLT_MAX;
  }

  if (_msg.has_restitution_coefficient())
    this->bounce = _msg.restitution_coefficient();
  if (_msg.has_bounce_threshold())
    this->bounceThreshold = _msg.bounce_threshold();
  if (_msg.has_soft_cfm())
    this->cfm = _msg.soft_cfm();
  if (_msg.has_soft_erp())
    this->erp = _msg.soft_erp();
  if (_msg.has_kp())
    this->kp = _msg.kp();
  if (_msg.has_kd())
    this->kd = _msg.kd();
  if (_msg.has_max_vel())
    this->maxVel = _msg.max_vel();
  if (_msg.has_min_depth())
    this->minDepth = _msg.min_depth();
  if (_msg.has_collide_without_contact())
    this->collideWithoutContact = _msg.collide_without_contact();
}


