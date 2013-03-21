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
/* Desc: A SimbodyHingeJoint
 * Author: Nate Koenig, Andrew Howard
 * Date: 21 May 2003
 */
#include "common/Console.hh"
#include "common/Exception.hh"

#include "physics/simbody/SimbodyLink.hh"
#include "physics/simbody/SimbodyPhysics.hh"
#include "physics/simbody/SimbodyHingeJoint.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
SimbodyHingeJoint::SimbodyHingeJoint(SimTK::MultibodySystem * /*_world*/,
                                     BasePtr _parent)
    : HingeJoint<SimbodyJoint>(_parent)
{
}

//////////////////////////////////////////////////
SimbodyHingeJoint::~SimbodyHingeJoint()
{
}

//////////////////////////////////////////////////
void SimbodyHingeJoint::Load(sdf::ElementPtr _sdf)
{
  HingeJoint<SimbodyJoint>::Load(_sdf);
}

//////////////////////////////////////////////////
math::Vector3 SimbodyHingeJoint::GetAnchor(int /*_index*/) const
{
  gzerr << "Not implemented...\n";
  return math::Vector3();
}

//////////////////////////////////////////////////
void SimbodyHingeJoint::SetAnchor(int /*_index*/,
                                 const math::Vector3 &/*_anchor*/)
{
  gzerr << "Not implemented...\n";
  // The anchor (pivot in Simbody lingo), can only be set on creation
}

//////////////////////////////////////////////////
void SimbodyHingeJoint::SetAxis(int /*_index*/, const math::Vector3 &/*_axis*/)
{
  // Simbody seems to handle setAxis improperly. It readjust all the pivot
  // points
  gzerr << "Not implemented...\n";
}

//////////////////////////////////////////////////
void SimbodyHingeJoint::SetDamping(int /*index*/, double /*_damping*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
void SimbodyHingeJoint::SetVelocity(int /*_index*/, double /*_angle*/)
{
  gzerr << "Not implemented...\n";
}

//////////////////////////////////////////////////
double SimbodyHingeJoint::GetVelocity(int _index) const
{
  if (_index < this->GetAngleCount())
    return this->mobod.getOneU(
      this->simbodyPhysics->integ->getState(),
      SimTK::MobilizerUIndex(_index));
  else
    gzerr << "Invalid index for joint, returning NaN\n";
  return SimTK::NaN;
}

//////////////////////////////////////////////////
void SimbodyHingeJoint::SetMaxForce(int /*_index*/, double _t)
{
  gzerr << "Not implemented...\n";
}

//////////////////////////////////////////////////
double SimbodyHingeJoint::GetMaxForce(int /*_index*/)
{
  gzerr << "Not implemented...\n";
  return 0;
}

//////////////////////////////////////////////////
void SimbodyHingeJoint::SetForce(int _index, double _torque)
{
  gzerr << "Setting Joint Force " << _torque << "\n";

  if (_index < this->GetAngleCount())
    this->simbodyPhysics->discreteForces.setOneMobilityForce(
      this->simbodyPhysics->integ->updAdvancedState(),
      this->mobod, SimTK::MobilizerUIndex(_index), _torque);
}

//////////////////////////////////////////////////
double SimbodyHingeJoint::GetForce(int /*_index*/)
{
  gzerr << "Not implemented...\n";
  return 0;
}

//////////////////////////////////////////////////
void SimbodyHingeJoint::SetHighStop(int /*_index*/,
                                   const math::Angle &/*_angle*/)
{
  gzerr << "Not implemented...\n";
}

//////////////////////////////////////////////////
void SimbodyHingeJoint::SetLowStop(int /*_index*/,
                                  const math::Angle &/*_angle*/)
{
  gzerr << "Not implemented...\n";
}

//////////////////////////////////////////////////
math::Angle SimbodyHingeJoint::GetHighStop(int /*_index*/)
{
  math::Angle result;
  gzerr << "Not implemented...\n";
  return result;
}

//////////////////////////////////////////////////
math::Angle SimbodyHingeJoint::GetLowStop(int /*_index*/)
{
  math::Angle result;
  gzerr << "Not implemented...\n";
  return result;
}

//////////////////////////////////////////////////
math::Vector3 SimbodyHingeJoint::GetGlobalAxis(int _index) const
{
  if (_index < static_cast<int>(this->GetAngleCount()))
  {
    const SimTK::Transform &X_OM = this->mobod.getOutboardFrame(
      this->simbodyPhysics->integ->getState());

    // express Z-axis of X_OM in world frame
    SimTK::Vec3 z_W(this->mobod.expressVectorInGroundFrame(
      this->simbodyPhysics->integ->getState(), X_OM.z()));

    return SimbodyPhysics::Vec3ToVector3(z_W);
  }
  else
  {
    gzerr << "index out of bound\n";
    return math::Vector3();
  }
}

//////////////////////////////////////////////////
math::Angle SimbodyHingeJoint::GetAngleImpl(int _index) const
{
  if (_index < static_cast<int>(this->GetAngleCount()))
  {
    return math::Angle(this->mobod.getOneQ(
      this->simbodyPhysics->integ->getState(), _index));
  }
  else
  {
    gzerr << "index out of bound\n";
    return math::Angle();
  }
}
