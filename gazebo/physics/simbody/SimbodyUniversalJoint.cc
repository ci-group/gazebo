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

#include "gazebo/common/Exception.hh"
#include "gazebo/common/Console.hh"

#include "gazebo/physics/Model.hh"
#include "gazebo/physics/simbody/SimbodyLink.hh"
#include "gazebo/physics/simbody/SimbodyTypes.hh"
#include "gazebo/physics/simbody/SimbodyUniversalJoint.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
SimbodyUniversalJoint::SimbodyUniversalJoint(SimTK::MultibodySystem* /*_world*/,
  BasePtr _parent) : UniversalJoint<SimbodyJoint>(_parent)
{
  this->physicsInitialized = false;
}

//////////////////////////////////////////////////
SimbodyUniversalJoint::~SimbodyUniversalJoint()
{
}

//////////////////////////////////////////////////
void SimbodyUniversalJoint::Load(sdf::ElementPtr _sdf)
{
  UniversalJoint<SimbodyJoint>::Load(_sdf);
}

//////////////////////////////////////////////////
math::Vector3 SimbodyUniversalJoint::GetAnchor(unsigned int /*_index*/) const
{
  return this->anchorPos;
}

//////////////////////////////////////////////////
math::Vector3 SimbodyUniversalJoint::GetAxis(unsigned int /*_index*/) const
{
  return math::Vector3();
}

//////////////////////////////////////////////////
void SimbodyUniversalJoint::SetAxis(unsigned int /*_index*/,
                                   const math::Vector3 &/*_axis*/)
{
  /// Universal Joint are built in SimbodyPhysics.cc, so this init block
  /// actually does nothing.
  gzdbg << "SetAxis: setting axis is not yet implemented. The axes are set "
        << "during joint construction in SimbodyPhyiscs.cc for now.\n";

  /// \TODO:  Better document this
  /// Flip order of axis1 and axis2 because ODE/Gazebo currently uses
  /// space-fixed axis for a universal joint, whereas Simbody uses
  /// body-fixed axis for a universal joint.
  /// Simbody implements a x-y body-fixed (first rotate about x-axis then
  /// moving y-axis).  This is equivalent to y-x space-fixed (first rotate
  /// aobut y-axis then around the original x-axis).
}


//////////////////////////////////////////////////
double SimbodyUniversalJoint::GetVelocity(unsigned int _index) const
{
  if (_index < this->GetAngleCount())
  {
    if (this->physicsInitialized &&
        this->simbodyPhysics->simbodyPhysicsInitialized)
      return this->mobod.getOneU(
        this->simbodyPhysics->integ->getState(),
        SimTK::MobilizerUIndex(_index));
    else
    {
      gzdbg << "GetVelocity() simbody not yet initialized, "
            << "initial velocity should be zero until restart from "
            << "state has been implemented.\n";
      return 0.0;
    }
  }
  else
  {
    gzerr << "Invalid index for joint, returning NaN\n";
    return SimTK::NaN;
  }
}

//////////////////////////////////////////////////
void SimbodyUniversalJoint::SetVelocity(unsigned int _index,
    double _rate)
{
  if (_index < this->GetAngleCount())
    this->mobod.setOneU(
      this->simbodyPhysics->integ->updAdvancedState(),
      SimTK::MobilizerUIndex(_index), _rate);
  else
    gzerr << "SetVelocity _index too large.\n";
}

//////////////////////////////////////////////////
void SimbodyUniversalJoint::SetForceImpl(unsigned int _index,
    double _torque)
{
  if (_index < this->GetAngleCount() &&
      this->physicsInitialized)
    this->simbodyPhysics->discreteForces.setOneMobilityForce(
      this->simbodyPhysics->integ->updAdvancedState(),
      this->mobod, SimTK::MobilizerUIndex(_index), _torque);
}

//////////////////////////////////////////////////
void SimbodyUniversalJoint::SetMaxForce(unsigned int /*_index*/, double /*_t*/)
{
  gzerr << "Not implemented\n";
}

//////////////////////////////////////////////////
double SimbodyUniversalJoint::GetMaxForce(unsigned int /*_index*/)
{
  gzerr << "Not implemented\n";
  return 0;
}

//////////////////////////////////////////////////
void SimbodyUniversalJoint::SetHighStop(unsigned int _index,
  const math::Angle &_angle)
{
  if (_index < this->GetAngleCount())
  {
    Joint::SetHighStop(_index, _angle);
    if (this->physicsInitialized)
    {
      this->limitForce[_index].setBounds(
        this->simbodyPhysics->integ->updAdvancedState(),
        this->limitForce[_index].getLowerBound(
          this->simbodyPhysics->integ->updAdvancedState()),
        _angle.Radian());
    }
    else
    {
      gzerr << "SetHighStop: State not initialized, SetLowStop failed.\n";
    }
  }
  else
    gzerr << "SetHighStop: index out of bounds.\n";
}

//////////////////////////////////////////////////
void SimbodyUniversalJoint::SetLowStop(unsigned int _index,
  const math::Angle &_angle)
{
  if (_index < this->GetAngleCount())
  {
    Joint::SetLowStop(_index, _angle);
    if (this->physicsInitialized)
    {
      this->limitForce[_index].setBounds(
        this->simbodyPhysics->integ->updAdvancedState(),
        _angle.Radian(),
        this->limitForce[_index].getUpperBound(
          this->simbodyPhysics->integ->updAdvancedState()));
    }
    else
    {
      gzerr << "SetLowStop: State not initialized, SetLowStop failed.\n";
    }
  }
  else
    gzerr << "SetLowStop: index out of bounds.\n";
}

//////////////////////////////////////////////////
math::Angle SimbodyUniversalJoint::GetHighStop(unsigned int _index)
{
  if (_index >= this->GetAngleCount())
  {
    gzerr << "GetHighStop: Invalid joint index ["
          << _index << "] when trying to get high stop\n";
    return math::Angle(0.0);  /// \TODO: should return NaN
  }
  else if (_index == 0)
  {
    return math::Angle(this->sdf->GetElement("axis")->GetElement("limit")
             ->Get<double>("upper"));
  }
  else if (_index == 1)
  {
    return math::Angle(this->sdf->GetElement("axis2")->GetElement("limit")
             ->Get<double>("upper"));
  }
  else
  {
    gzerr << "GetHighStop: Should not be here in code, GetAngleCount < 0.\n";
    return math::Angle(0.0);  /// \TODO: should return NaN
  }
}

//////////////////////////////////////////////////
math::Angle SimbodyUniversalJoint::GetLowStop(unsigned int _index)
{
  if (_index >= this->GetAngleCount())
  {
    gzerr << "GetLowStop: Invalid joint index ["
          << _index << "] when trying to get low stop\n";
    return math::Angle(0.0);  /// \TODO: should return NaN
  }
  else if (_index == 0)
  {
    return math::Angle(this->sdf->GetElement("axis")->GetElement("limit")
             ->Get<double>("lower"));
  }
  else if (_index == 1)
  {
    return math::Angle(this->sdf->GetElement("axis2")->GetElement("limit")
             ->Get<double>("lower"));
  }
  else
  {
    gzerr << "GetLowStop: Should not be here in code, GetAngleCount < 0.\n";
    return math::Angle(0.0);  /// \TODO: should return NaN
  }
}

//////////////////////////////////////////////////
math::Vector3 SimbodyUniversalJoint::GetGlobalAxis(
    unsigned int _index) const
{
  if (this->simbodyPhysics &&
      this->simbodyPhysics->simbodyPhysicsStepped &&
      _index < this->GetAngleCount())
  {
    if (_index == 0)
    {
      // express X-axis of X_IF in world frame
      const SimTK::Transform &X_IF = this->mobod.getInboardFrame(
        this->simbodyPhysics->integ->getState());

      SimTK::Vec3 x_W(
        this->mobod.getParentMobilizedBody().expressVectorInGroundFrame(
        this->simbodyPhysics->integ->getState(), X_IF.x()));

      return SimbodyPhysics::Vec3ToVector3(x_W);
    }
    else if (_index == 1)
    {
      // express Y-axis of X_OM in world frame
      const SimTK::Transform &X_OM = this->mobod.getOutboardFrame(
        this->simbodyPhysics->integ->getState());

      SimTK::Vec3 y_W(
        this->mobod.expressVectorInGroundFrame(
        this->simbodyPhysics->integ->getState(), X_OM.y()));

      return SimbodyPhysics::Vec3ToVector3(y_W);
    }
    else
    {
      gzerr << "GetGlobalAxis: internal error, GetAngleCount < 0.\n";
      return math::Vector3(SimTK::NaN, SimTK::NaN, SimTK::NaN);
    }
  }
  else
  {
    if (_index >= this->GetAngleCount())
    {
      gzerr << "index out of bound\n";
      return math::Vector3(SimTK::NaN, SimTK::NaN, SimTK::NaN);
    }
    else
    {
      gzdbg << "GetGlobalAxis() sibmody physics engine not yet initialized, "
            << "use local axis and initial pose to compute "
            << "global axis.\n";
      // if local axis specified in model frame (to be changed)
      // switch to code below if issue #494 is to be addressed
      return this->model->GetWorldPose().rot.RotateVector(
        this->GetLocalAxis(_index));

      // if local axis specified in joint frame (Issue #494)
      // Remember to remove include of Model.hh when switching.
      // if (this->childLink)
      // {
      //   math::Pose jointPose =
      //    this->anchorPose + this->childLink->GetWorldPose();
      //   return jointPose.rot.RotateVector(this->GetLocalAxis(_index));
      // }
      // else
      // {
      //   gzerr << "Joint [" << this->GetName() << "] missing child link.\n";
      //   return math::Vector3(SimTK::NaN, SimTK::NaN, SimTK::NaN);
      // }
    }
  }
}

//////////////////////////////////////////////////
math::Angle SimbodyUniversalJoint::GetAngleImpl(unsigned int _index) const
{
  if (_index < this->GetAngleCount())
  {
    if (this->physicsInitialized &&
        this->simbodyPhysics->simbodyPhysicsInitialized)
      return math::Angle(this->mobod.getOneQ(
        this->simbodyPhysics->integ->getState(), _index));
    else
    {
      gzdbg << "GetAngleImpl(): simbody not yet initialized, "
            << "initial angle should be zero until <initial_angle> "
            << "is implemented.\n";
      return math::Angle(0.0);
    }
  }
  else
  {
    gzerr << "index out of bound\n";
    return math::Angle(SimTK::NaN);
  }
}
