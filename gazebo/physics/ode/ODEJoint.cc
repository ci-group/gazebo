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
/* Desc: The ODE base joint class
 * Author: Nate Koenig, Andrew Howard
 * Date: 12 Oct 2009
 */

#include "gazebo/common/Exception.hh"
#include "gazebo/common/Console.hh"
#include "gazebo/common/Assert.hh"

#include "gazebo/physics/World.hh"
#include "gazebo/physics/Link.hh"
#include "gazebo/physics/PhysicsEngine.hh"
#include "gazebo/physics/ode/ODELink.hh"
#include "gazebo/physics/ode/ODEJoint.hh"
#include "gazebo/physics/ScrewJoint.hh"
#include "gazebo/physics/JointWrench.hh"

using namespace gazebo;
using namespace physics;


//////////////////////////////////////////////////
ODEJoint::ODEJoint(BasePtr _parent)
  : Joint(_parent)
{
  this->jointId = NULL;
  this->cfmDampingState[0] = ODEJoint::NONE;
  this->cfmDampingState[1] = ODEJoint::NONE;
  this->dampingInitialized = false;
  this->feedback = NULL;
}

//////////////////////////////////////////////////
ODEJoint::~ODEJoint()
{
  delete this->feedback;
  this->Detach();

  if (this->jointId)
    dJointDestroy(this->jointId);
}

//////////////////////////////////////////////////
void ODEJoint::Load(sdf::ElementPtr _sdf)
{
  Joint::Load(_sdf);

  if (this->sdf->HasElement("physics") &&
      this->sdf->GetElement("physics")->HasElement("ode"))
  {
    sdf::ElementPtr elem = this->sdf->GetElement("physics")->GetElement("ode");

    if (elem->HasElement("provide_feedback"))
    {
      this->provideFeedback = elem->Get<bool>("provide_feedback");
    }

    if (elem->HasElement("cfm_damping"))
    {
      this->useCFMDamping = elem->Get<bool>("cfm_damping");
    }

    // initializa both axis, \todo: make cfm, erp per axis
    this->stopERP = elem->GetElement("limit")->Get<double>("erp");
    for (unsigned int i = 0; i < this->GetAngleCount(); ++i)
      this->SetAttribute("stop_erp", i, this->stopERP);

    // initializa both axis, \todo: make cfm, erp per axis
    this->stopCFM = elem->GetElement("limit")->Get<double>("cfm");
    for (unsigned int i = 0; i < this->GetAngleCount(); ++i)
      this->SetAttribute("stop_cfm", i, this->stopCFM);

    if (elem->HasElement("suspension"))
    {
      this->SetParam(dParamSuspensionERP,
          elem->GetElement("suspension")->Get<double>("erp"));
      this->SetParam(dParamSuspensionCFM,
          elem->GetElement("suspension")->Get<double>("cfm"));
    }

    if (elem->HasElement("fudge_factor"))
      this->SetParam(dParamFudgeFactor,
          elem->GetElement("fudge_factor")->Get<double>());

    if (elem->HasElement("cfm"))
        this->SetParam(dParamCFM, elem->GetElement("cfm")->Get<double>());

    if (elem->HasElement("bounce"))
        this->SetParam(dParamBounce,
          elem->GetElement("bounce")->Get<double>());

    if (elem->HasElement("max_force"))
      this->SetParam(dParamFMax,
          elem->GetElement("max_force")->Get<double>());

    if (elem->HasElement("velocity"))
      this->SetParam(dParamVel,
          elem->GetElement("velocity")->Get<double>());
  }

  if (this->sdf->HasElement("axis"))
  {
    sdf::ElementPtr axisElem = this->sdf->GetElement("axis");
    if (axisElem->HasElement("dynamics"))
    {
      sdf::ElementPtr dynamicsElem = axisElem->GetElement("dynamics");

      if (dynamicsElem->HasElement("damping"))
      {
        this->SetDamping(0, dynamicsElem->Get<double>("damping"));
      }
      if (dynamicsElem->HasElement("friction"))
      {
        sdf::ElementPtr frictionElem = dynamicsElem->GetElement("friction");
        gzlog << "joint friction not implemented\n";
      }
    }
  }


  if (this->provideFeedback)
  {
    this->feedback = new dJointFeedback;

    if (this->jointId)
      dJointSetFeedback(this->jointId, this->feedback);
    else
      gzerr << "ODE Joint ID is invalid\n";
  }
}

//////////////////////////////////////////////////
LinkPtr ODEJoint::GetJointLink(int _index) const
{
  LinkPtr result;
  if (!this->jointId)
  {
    gzerr << "ODE Joint ID is invalid\n";
    return result;
  }

  if (_index == 0 || _index == 1)
  {
    ODELinkPtr odeLink1 = boost::static_pointer_cast<ODELink>(this->childLink);
    ODELinkPtr odeLink2 = boost::static_pointer_cast<ODELink>(this->parentLink);
    if (odeLink1 != NULL &&
        dJointGetBody(this->jointId, _index) == odeLink1->GetODEId())
      result = this->childLink;
    else if (odeLink2)
      result = this->parentLink;
  }

  return result;
}

//////////////////////////////////////////////////
bool ODEJoint::AreConnected(LinkPtr _one, LinkPtr _two) const
{
  ODELinkPtr odeLink1 = boost::dynamic_pointer_cast<ODELink>(_one);
  ODELinkPtr odeLink2 = boost::dynamic_pointer_cast<ODELink>(_two);

  if (odeLink1 == NULL || odeLink2 == NULL)
    gzthrow("ODEJoint requires ODE bodies\n");

  return dAreConnected(odeLink1->GetODEId(), odeLink2->GetODEId());
}

//////////////////////////////////////////////////
// child classes where appropriate
double ODEJoint::GetParam(int /*parameter*/) const
{
  return 0;
}

//////////////////////////////////////////////////
void ODEJoint::Attach(LinkPtr _parent, LinkPtr _child)
{
  Joint::Attach(_parent, _child);

  ODELinkPtr odechild = boost::dynamic_pointer_cast<ODELink>(this->childLink);
  ODELinkPtr odeparent = boost::dynamic_pointer_cast<ODELink>(this->parentLink);

  if (odechild == NULL && odeparent == NULL)
    gzthrow("ODEJoint requires at least one ODE link\n");

  if (!this->jointId)
    gzerr << "ODE Joint ID is invalid\n";

  if (!odechild && odeparent)
  {
    dJointAttach(this->jointId, 0, odeparent->GetODEId());
  }
  else if (odechild && !odeparent)
  {
    dJointAttach(this->jointId, odechild->GetODEId(), 0);
  }
  else if (odechild && odeparent)
  {
    if (this->HasType(Base::HINGE2_JOINT))
      dJointAttach(this->jointId, odeparent->GetODEId(), odechild->GetODEId());
    else
      dJointAttach(this->jointId, odechild->GetODEId(), odeparent->GetODEId());
  }
}

//////////////////////////////////////////////////
void ODEJoint::Detach()
{
  Joint::Detach();
  this->childLink.reset();
  this->parentLink.reset();

  if (this->jointId)
    dJointAttach(this->jointId, 0, 0);
  else
    gzerr << "ODE Joint ID is invalid\n";
}

//////////////////////////////////////////////////
// where appropriate
void ODEJoint::SetParam(int /*parameter*/, double /*value*/)
{
  if (this->childLink)
    this->childLink->SetEnabled(true);
  if (this->parentLink)
    this->parentLink->SetEnabled(true);
}

//////////////////////////////////////////////////
void ODEJoint::SetERP(double _newERP)
{
  this->SetParam(dParamSuspensionERP, _newERP);
}

//////////////////////////////////////////////////
double ODEJoint::GetERP()
{
  return this->GetParam(dParamSuspensionERP);
}

//////////////////////////////////////////////////
void ODEJoint::SetCFM(double _newCFM)
{
  this->SetParam(dParamSuspensionCFM, _newCFM);
}

//////////////////////////////////////////////////
double ODEJoint::GetCFM()
{
  return this->GetParam(dParamSuspensionCFM);
}

//////////////////////////////////////////////////
dJointFeedback *ODEJoint::GetFeedback()
{
  if (this->jointId)
    return dJointGetFeedback(this->jointId);
  else
    gzerr << "ODE Joint ID is invalid\n";
  return NULL;
}

//////////////////////////////////////////////////
void ODEJoint::SetHighStop(int _index, const math::Angle &_angle)
{
  Joint::SetHighStop(_index, _angle);
  switch (_index)
  {
    case 0:
      this->SetParam(dParamHiStop, _angle.Radian());
      break;
    case 1:
      this->SetParam(dParamHiStop2, _angle.Radian());
      break;
    case 2:
      this->SetParam(dParamHiStop3, _angle.Radian());
      break;
    default:
      gzerr << "Invalid index[" << _index << "]\n";
      break;
  };
}

//////////////////////////////////////////////////
void ODEJoint::SetLowStop(int _index, const math::Angle &_angle)
{
  Joint::SetLowStop(_index, _angle);
  switch (_index)
  {
    case 0:
      this->SetParam(dParamLoStop, _angle.Radian());
      break;
    case 1:
      this->SetParam(dParamLoStop2, _angle.Radian());
      break;
    case 2:
      this->SetParam(dParamLoStop3, _angle.Radian());
      break;
    default:
      gzerr << "Invalid index[" << _index << "]\n";
  };
}

//////////////////////////////////////////////////
math::Angle ODEJoint::GetHighStop(int _index)
{
  return this->GetUpperLimit(_index);
}

//////////////////////////////////////////////////
math::Angle ODEJoint::GetLowStop(int _index)
{
  return this->GetLowerLimit(_index);
}

//////////////////////////////////////////////////
math::Vector3 ODEJoint::GetLinkForce(unsigned int _index) const
{
  math::Vector3 result;

  if (!this->jointId)
  {
    gzerr << "ODE Joint ID is invalid\n";
    return result;
  }

  dJointFeedback *jointFeedback = dJointGetFeedback(this->jointId);

  if (_index == 0)
    result.Set(jointFeedback->f1[0], jointFeedback->f1[1],
               jointFeedback->f1[2]);
  else
    result.Set(jointFeedback->f2[0], jointFeedback->f2[1],
               jointFeedback->f2[2]);

  return result;
}

//////////////////////////////////////////////////
math::Vector3 ODEJoint::GetLinkTorque(unsigned int _index) const
{
  math::Vector3 result;

  if (!this->jointId)
  {
    gzerr << "ODE Joint ID is invalid\n";
    return result;
  }

  dJointFeedback *jointFeedback = dJointGetFeedback(this->jointId);

  if (_index == 0)
    result.Set(jointFeedback->t1[0], jointFeedback->t1[1],
               jointFeedback->t1[2]);
  else
    result.Set(jointFeedback->t2[0], jointFeedback->t2[1],
               jointFeedback->t2[2]);

  return result;
}

//////////////////////////////////////////////////
void ODEJoint::SetAttribute(Attribute _attr, int _index, double _value)
{
  switch (_attr)
  {
    case FUDGE_FACTOR:
      this->SetParam(dParamFudgeFactor, _value);
      break;
    case SUSPENSION_ERP:
      this->SetParam(dParamSuspensionERP, _value);
      break;
    case SUSPENSION_CFM:
      this->SetParam(dParamSuspensionCFM, _value);
      break;
    case STOP_ERP:
      switch (_index)
      {
        case 0:
          this->SetParam(dParamStopERP, _value);
          break;
        case 1:
          this->SetParam(dParamStopERP2, _value);
          break;
        case 2:
          this->SetParam(dParamStopERP3, _value);
          break;
        default:
          gzerr << "Invalid index[" << _index << "]\n";
          break;
      };
      break;
    case STOP_CFM:
      switch (_index)
      {
        case 0:
          this->SetParam(dParamStopCFM, _value);
          break;
        case 1:
          this->SetParam(dParamStopCFM2, _value);
          break;
        case 2:
          this->SetParam(dParamStopCFM3, _value);
          break;
        default:
          gzerr << "Invalid index[" << _index << "]\n";
          break;
      };
      break;
    case ERP:
      this->SetParam(dParamERP, _value);
      break;
    case CFM:
      this->SetParam(dParamCFM, _value);
      break;
    case FMAX:
      this->SetParam(dParamFMax, _value);
      break;
    case VEL:
      this->SetParam(dParamVel, _value);
      break;
    case HI_STOP:
      switch (_index)
      {
        case 0:
          this->SetParam(dParamHiStop, _value);
          break;
        case 1:
          this->SetParam(dParamHiStop2, _value);
          break;
        case 2:
          this->SetParam(dParamHiStop3, _value);
          break;
        default:
          gzerr << "Invalid index[" << _index << "]\n";
          break;
      };
      break;
    case LO_STOP:
      switch (_index)
      {
        case 0:
          this->SetParam(dParamLoStop, _value);
          break;
        case 1:
          this->SetParam(dParamLoStop2, _value);
          break;
        case 2:
          this->SetParam(dParamLoStop3, _value);
          break;
        default:
          gzerr << "Invalid index[" << _index << "]\n";
          break;
      };
      break;
    default:
      gzerr << "Unable to handle joint attribute[" << _attr << "]\n";
      break;
  };
}

//////////////////////////////////////////////////
void ODEJoint::SetAttribute(const std::string &_key, int _index,
                            const boost::any &_value)
{
  if (_key == "fudge_factor")
  {
    try
    {
      this->SetParam(dParamFudgeFactor, boost::any_cast<double>(_value));
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
  else if (_key == "suspension_erp")
  {
    try
    {
      this->SetParam(dParamSuspensionERP, boost::any_cast<double>(_value));
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
  else if (_key == "suspension_cfm")
  {
    try
    {
      this->SetParam(dParamSuspensionCFM, boost::any_cast<double>(_value));
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
  else if (_key == "stop_erp")
  {
    try
    {
      switch (_index)
      {
        case 0:
          this->SetParam(dParamStopERP, boost::any_cast<double>(_value));
          break;
        case 1:
          this->SetParam(dParamStopERP2, boost::any_cast<double>(_value));
          break;
        case 2:
          this->SetParam(dParamStopERP3, boost::any_cast<double>(_value));
          break;
        default:
          gzerr << "Invalid index[" << _index << "]\n";
          break;
      };
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
  else if (_key == "stop_cfm")
  {
    try
    {
      switch (_index)
      {
        case 0:
          this->SetParam(dParamStopCFM, boost::any_cast<double>(_value));
          break;
        case 1:
          this->SetParam(dParamStopCFM2, boost::any_cast<double>(_value));
          break;
        case 2:
          this->SetParam(dParamStopCFM3, boost::any_cast<double>(_value));
          break;
        default:
          gzerr << "Invalid index[" << _index << "]\n";
          break;
      };
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
  else if (_key == "erp")
  {
    try
    {
      this->SetParam(dParamERP, boost::any_cast<double>(_value));
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
  else if (_key == "cfm")
  {
    try
    {
      this->SetParam(dParamCFM, boost::any_cast<double>(_value));
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
  else if (_key == "fmax")
  {
    try
    {
      this->SetParam(dParamFMax, boost::any_cast<double>(_value));
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
  else if (_key == "vel")
  {
    try
    {
      this->SetParam(dParamVel, boost::any_cast<double>(_value));
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
  else if (_key == "hi_stop")
  {
    try
    {
      switch (_index)
      {
        case 0:
          this->SetParam(dParamHiStop, boost::any_cast<double>(_value));
          break;
        case 1:
          this->SetParam(dParamHiStop2, boost::any_cast<double>(_value));
          break;
        case 2:
          this->SetParam(dParamHiStop3, boost::any_cast<double>(_value));
          break;
        default:
          gzerr << "Invalid index[" << _index << "]\n";
          break;
      };
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
  else if (_key == "lo_stop")
  {
    try
    {
      switch (_index)
      {
        case 0:
          this->SetParam(dParamLoStop, boost::any_cast<double>(_value));
          break;
        case 1:
          this->SetParam(dParamLoStop2, boost::any_cast<double>(_value));
          break;
        case 2:
          this->SetParam(dParamLoStop3, boost::any_cast<double>(_value));
          break;
        default:
          gzerr << "Invalid index[" << _index << "]\n";
          break;
      };
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
  else if (_key == "thread_pitch")
  {
    ScrewJoint<ODEJoint>* screwJoint =
      dynamic_cast<ScrewJoint<ODEJoint>* >(this);
    if (screwJoint != NULL)
    {
      try
      {
        screwJoint->SetThreadPitch(0, boost::any_cast<double>(_value));
      }
      catch(boost::bad_any_cast &e)
      {
        gzerr << "boost any_cast error:" << e.what() << "\n";
      }
    }
  }
  else
  {
    try
    {
      gzerr << "Unable to handle joint attribute["
            << boost::any_cast<std::string>(_value) << "]\n";
    }
    catch(boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
    }
  }
}

//////////////////////////////////////////////////
double ODEJoint::GetAttribute(const std::string &_key, unsigned int _index)
{
  if (_key == "fudge_factor")
  {
    try
    {
      return this->GetParam(dParamFudgeFactor);
    }
    catch(common::Exception &e)
    {
      gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
      return 0;
    }
  }
  else if (_key == "suspension_erp")
  {
    try
    {
      return this->GetParam(dParamSuspensionERP);
    }
    catch(common::Exception &e)
    {
      gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
      return 0;
    }
  }
  else if (_key == "suspension_cfm")
  {
    try
    {
      return this->GetParam(dParamSuspensionCFM);
    }
    catch(common::Exception &e)
    {
      gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
      return 0;
    }
  }
  else if (_key == "stop_erp")
  {
    try
    {
      /// \TODO: switch based on index
      return this->GetParam(dParamStopERP);
    }
    catch(common::Exception &e)
    {
      gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
      return 0;
    }
  }
  else if (_key == "stop_cfm")
  {
    try
    {
      /// \TODO: switch based on index
      return this->GetParam(dParamStopCFM);
    }
    catch(common::Exception &e)
    {
      gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
      return 0;
    }
  }
  else if (_key == "erp")
  {
    try
    {
      return this->GetParam(dParamERP);
    }
    catch(common::Exception &e)
    {
      gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
      return 0;
    }
  }
  else if (_key == "cfm")
  {
    try
    {
      return this->GetParam(dParamCFM);
    }
    catch(common::Exception &e)
    {
      gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
      return 0;
    }
  }
  else if (_key == "fmax")
  {
    try
    {
      return this->GetParam(dParamFMax);
    }
    catch(common::Exception &e)
    {
      gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
      return 0;
    }
  }
  else if (_key == "vel")
  {
    try
    {
      return this->GetParam(dParamVel);
    }
    catch(common::Exception &e)
    {
      gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
      return 0;
    }
  }
  else if (_key == "hi_stop")
  {
    try
    {
      switch (_index)
      {
        case 0:
          return this->GetParam(dParamHiStop);
        case 1:
          return this->GetParam(dParamHiStop2);
        case 2:
          return this->GetParam(dParamHiStop3);
        default:
          gzerr << "Invalid index[" << _index << "]\n";
          break;
      };
    }
    catch(common::Exception &e)
    {
      gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
      return 0;
    }
  }
  else if (_key == "lo_stop")
  {
    try
    {
      switch (_index)
      {
        case 0:
          return this->GetParam(dParamLoStop);
        case 1:
          return this->GetParam(dParamLoStop2);
        case 2:
          return this->GetParam(dParamLoStop3);
        default:
          gzerr << "Invalid index[" << _index << "]\n";
          break;
      };
    }
    catch(common::Exception &e)
    {
      gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
      return 0;
    }
  }
  else if (_key == "thread_pitch")
  {
    ScrewJoint<ODEJoint>* screwJoint =
      dynamic_cast<ScrewJoint<ODEJoint>* >(this);
    if (screwJoint != NULL)
    {
      try
      {
        return screwJoint->GetThreadPitch(0);
      }
      catch(common::Exception &e)
      {
        gzerr << "GetParam error:" << e.GetErrorStr() << "\n";
        return 0;
      }
    }
    else
    {
      gzerr << "Trying to get thread_pitch for non-screw joints.\n";
      return 0;
    }
  }
  else
  {
    gzerr << "Unable to get joint attribute[" << _key << "]\n";
    return 0;
  }

  gzerr << "should not be here\n";
  return 0;
}

//////////////////////////////////////////////////
void ODEJoint::Reset()
{
  if (this->jointId)
    dJointReset(this->jointId);
  else
    gzerr << "ODE Joint ID is invalid\n";

  Joint::Reset();
}

//////////////////////////////////////////////////
JointWrench ODEJoint::GetForceTorque(int _index)
{
  return this->GetForceTorque(static_cast<unsigned int>(_index));
}

//////////////////////////////////////////////////
JointWrench ODEJoint::GetForceTorque(unsigned int /*_index*/)
{
  JointWrench wrench;
  // Note that:
  // f2, t2 are the force torque measured on parent body's cg
  // f1, t1 are the force torque measured on child body's cg
  dJointFeedback* fb = this->GetFeedback();
  if (fb)
  {
    // kind of backwards here, body1 (parent) corresponds go f2, t2
    // and body2 (child) corresponds go f1, t1
    wrench.body2Force.Set(fb->f1[0], fb->f1[1], fb->f1[2]);
    wrench.body2Torque.Set(fb->t1[0], fb->t1[1], fb->t1[2]);
    wrench.body1Force.Set(fb->f2[0], fb->f2[1], fb->f2[2]);
    wrench.body1Torque.Set(fb->t2[0], fb->t2[1], fb->t2[2]);

    if (this->childLink)
    {
      math::Pose childPose = this->childLink->GetWorldPose();

      // convert torque from about child CG to joint anchor location
      // cg position specified in child link frame
      math::Vector3 cgPos = this->childLink->GetInertial()->GetPose().pos;

      // moment arm rotated into world frame (given feedback is in world frame)
      math::Vector3 childMomentArm =
        childPose.rot.RotateVector(
        (this->anchorPose - math::Pose(cgPos, math::Quaternion())).pos);

      // gzerr << "anchor [" << anchorPos
      //       << "] iarm[" << this->childLink->GetInertial()->GetPose().pos
      //       << "] childMomentArm[" << childMomentArm
      //       << "] f1[" << wrench.body2Force
      //       << "] t1[" << wrench.body2Torque
      //       << "] fxp[" << wrench.body2Force.Cross(childMomentArm)
      //       << "]\n";

      wrench.body2Torque += wrench.body2Force.Cross(childMomentArm);

      // rotate resulting body1Force in world frame into link frame
      wrench.body2Force = childPose.rot.RotateVectorReverse(
        -wrench.body2Force);

      // rotate resulting body1Torque in world frame into link frame
      wrench.body2Torque = childPose.rot.RotateVectorReverse(
        -wrench.body2Torque);
    }

    // convert torque from about parent CG to joint anchor location
    if (this->parentLink)
    {
      // get child pose, or it's the inertial world if childLink is NULL
      math::Pose childPose;
      if (this->childLink)
        childPose = this->childLink->GetWorldPose();

      math::Pose parentPose = this->parentLink->GetWorldPose();
      // if parent link exists, convert torque from about parent
      // CG to joint anchor location

      // parent cg specified in parent link frame
      math::Vector3 cgPos = this->parentLink->GetInertial()->GetPose().pos;

      // rotate momeent arms into world frame
      math::Vector3 parentMomentArm =
        childPose.rot.RotateVector(this->anchorPos - cgPos);

      // gzerr << "anchor [" << anchorPos
      //       << "] iarm[" << cgPos
      //       << "] parentMomentArm[" << parentMomentArm
      //       << "] f1[" << wrench.body1Force
      //       << "] t1[" << wrench.body1Torque
      //       << "] fxp[" << wrench.body1Force.Cross(parentMomentArm)
      //       << "]\n";

      wrench.body1Torque += wrench.body1Force.Cross(parentMomentArm);

      // rotate resulting body1Force in world frame into link frame
      wrench.body1Force = parentPose.rot.RotateVectorReverse(
        -wrench.body1Force);

      // rotate resulting body1Torque in world frame into link frame
      wrench.body1Torque = parentPose.rot.RotateVectorReverse(
        -wrench.body1Torque);

      if (!this->childLink)
      {
        // if child link does not exist, use equal and opposite
        wrench.body2Force = -wrench.body1Force;
        wrench.body2Torque = -wrench.body1Torque;
      }
    }
    else
    {
      if (!this->childLink)
      {
        gzerr << "Both parent and child links are invalid, abort.\n";
        return JointWrench();
      }
      else
      {
        // if parentLink does not exist, use equal opposite body1 wrench
        wrench.body1Force = -wrench.body2Force;
        wrench.body1Torque = -wrench.body2Torque;
      }
    }
  }
  else
  {
    // forgot to set provide_feedback?
    gzwarn << "GetForceTorque: forget to set <provide_feedback>?\n";
  }

  return wrench;
}

//////////////////////////////////////////////////
void ODEJoint::CFMDamping()
{
  // check if we are violating joint limits
  if (this->GetAngleCount() > 2)
  {
     gzerr << "Incompatible joint type, GetAngleCount() = "
           << this->GetAngleCount() << " > 2\n";
     return;
  }

  for (unsigned int i = 0; i < this->GetAngleCount(); ++i)
  {
    if (this->GetAngle(i) >= this->upperLimit[i] ||
        this->GetAngle(i) <= this->lowerLimit[i] ||
        math::equal(this->dampingCoefficient, 0.0))
    {
      if (this->cfmDampingState[i] != ODEJoint::JOINT_LIMIT)
      {
        // we have hit the actual joint limit!
        // turn off simulated damping by recovering cfm and erp,
        // and recover joint limits
        this->SetAttribute("stop_erp", i, this->stopERP);
        this->SetAttribute("stop_cfm", i, this->stopCFM);
        this->SetAttribute("hi_stop", i, this->upperLimit[i].Radian());
        this->SetAttribute("lo_stop", i, this->lowerLimit[i].Radian());
        this->SetAttribute("hi_stop", i, this->upperLimit[i].Radian());
        this->cfmDampingState[i] = ODEJoint::JOINT_LIMIT;
      }
    }
    else if (!math::equal(this->dampingCoefficient, 0.0))
    {
      if (this->cfmDampingState[i] != ODEJoint::DAMPING_ACTIVE)
      {
        // add additional constraint row by fake hitting joint limit
        // then, set erp and cfm to simulate viscous joint damping
        this->SetAttribute("stop_erp", i, 0.0);
        this->SetAttribute("stop_cfm", i, 1.0 / this->dampingCoefficient);
        this->SetAttribute("hi_stop", i, 0.0);
        this->SetAttribute("lo_stop", i, 0.0);
        this->SetAttribute("hi_stop", i, 0.0);
        this->cfmDampingState[i] = ODEJoint::DAMPING_ACTIVE;
      }
    }
  }
}

//////////////////////////////////////////////////
void ODEJoint::SetDamping(int /*_index*/, double _damping)
{
  this->dampingCoefficient = _damping;

  // \TODO: implement on a per axis basis (requires additional sdf parameters)
  // trigger an update in CFMDAmping if this is called
  if (this->useCFMDamping)
  {
    if (this->GetAngleCount() > 2)
    {
       gzerr << "Incompatible joint type, GetAngleCount() = "
             << this->GetAngleCount() << " > 2\n";
       return;
    }
    for (unsigned int i = 0; i < this->GetAngleCount(); ++i)
      this->cfmDampingState[i] = ODEJoint::NONE;
  }

  bool parentStatic = this->GetParent() ? this->GetParent()->IsStatic() : false;
  bool childStatic = this->GetChild() ? this->GetChild()->IsStatic() : false;

  if (!this->dampingInitialized && !parentStatic && !childStatic)
  {
    if (this->useCFMDamping)
      this->applyDamping = physics::Joint::ConnectJointUpdate(
        boost::bind(&ODEJoint::CFMDamping, this));
    else
      this->applyDamping = physics::Joint::ConnectJointUpdate(
        boost::bind(&ODEJoint::ApplyDamping, this));
    this->dampingInitialized = true;
  }
}
