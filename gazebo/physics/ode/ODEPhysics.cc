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
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <sdf/sdf.hh>

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "gazebo/util/Diagnostics.hh"
#include "gazebo/common/Assert.hh"
#include "gazebo/common/Console.hh"
#include "gazebo/common/Exception.hh"
#include "gazebo/math/Vector3.hh"
#include "gazebo/math/Rand.hh"
#include "gazebo/common/Time.hh"
#include "gazebo/common/Timer.hh"

#include "gazebo/transport/Publisher.hh"

#include "gazebo/physics/PhysicsTypes.hh"
#include "gazebo/physics/PhysicsFactory.hh"
#include "gazebo/physics/World.hh"
#include "gazebo/physics/Entity.hh"
#include "gazebo/physics/Model.hh"
#include "gazebo/physics/SurfaceParams.hh"
#include "gazebo/physics/Collision.hh"
#include "gazebo/physics/MapShape.hh"
#include "gazebo/physics/ContactManager.hh"

#include "gazebo/physics/ode/ODECollision.hh"
#include "gazebo/physics/ode/ODELink.hh"
#include "gazebo/physics/ode/ODEScrewJoint.hh"
#include "gazebo/physics/ode/ODEHingeJoint.hh"
#include "gazebo/physics/ode/ODEGearboxJoint.hh"
#include "gazebo/physics/ode/ODEHinge2Joint.hh"
#include "gazebo/physics/ode/ODESliderJoint.hh"
#include "gazebo/physics/ode/ODEBallJoint.hh"
#include "gazebo/physics/ode/ODEUniversalJoint.hh"

#include "gazebo/physics/ode/ODERayShape.hh"
#include "gazebo/physics/ode/ODEBoxShape.hh"
#include "gazebo/physics/ode/ODESphereShape.hh"
#include "gazebo/physics/ode/ODECylinderShape.hh"
#include "gazebo/physics/ode/ODEPlaneShape.hh"
#include "gazebo/physics/ode/ODEMeshShape.hh"
#include "gazebo/physics/ode/ODEMultiRayShape.hh"
#include "gazebo/physics/ode/ODEHeightmapShape.hh"
#include "gazebo/physics/ode/ODEPolylineShape.hh"

#include "gazebo/physics/ode/ODEPhysics.hh"
#include "gazebo/physics/ode/ODESurfaceParams.hh"

using namespace gazebo;
using namespace physics;

GZ_REGISTER_PHYSICS_ENGINE("ode", ODEPhysics)

/*
class ContactUpdate_TBB
{
  public: ContactUpdate_TBB(tbb::concurrent_vector<ContactFeedback> *_contacts)
          : contacts(contacts) {}
  public: void operator() (const tbb::blocked_range<size_t> &_r) const
  {
    for (size_t i = _r.begin(); i != _r.end(); i++)
    {
      this->engine->ProcessContactFeedback((*this->contacts)[i]);
    }
  }

  private: tbb::concurrent_vector<ContactFeedback> *contacts;
  private: ODEPhysics *engine;
};
*/

class Colliders_TBB
{
  public: Colliders_TBB(
              std::vector<std::pair<ODECollision*, ODECollision*> > *_colliders,
              ODEPhysics *_engine,
              dContactGeom* _contactCollisions) :
    colliders(_colliders),
              engine(_engine), contactCollisions(_contactCollisions)
  {
    // dAllocateODEDataForThread(dAllocateMaskAll);
  }

  public: void operator() (const tbb::blocked_range<size_t> &_r) const
  {
    for (size_t i = _r.begin(); i != _r.end(); i++)
    {
      ODECollision *collision1 = (*this->colliders)[i].first;
      ODECollision *collision2 = (*this->colliders)[i].second;
      this->engine->Collide(collision1, collision2, contactCollisions);
    }
  }

  private: std::vector< std::pair<ODECollision*, ODECollision*> > *colliders;
  private: ODEPhysics *engine;
  private: dContactGeom* contactCollisions;
};

//////////////////////////////////////////////////
ODEPhysics::ODEPhysics(WorldPtr _world)
    : PhysicsEngine(_world), physicsStepFunc(NULL), maxContacts(0)
{
  // Collision detection init
  dInitODE2(0);

  dAllocateODEDataForThread(dAllocateMaskAll);

  this->worldId = dWorldCreate();

  this->spaceId = dHashSpaceCreate(0);
  dHashSpaceSetLevels(this->spaceId, -2, 8);

  this->contactGroup = dJointGroupCreate(0);

  this->colliders.resize(100);

  // Set random seed for physics engine based on gazebo's random seed.
  // Note: this was moved from physics::PhysicsEngine constructor.
  this->SetSeed(math::Rand::GetSeed());
}

//////////////////////////////////////////////////
ODEPhysics::~ODEPhysics()
{
  dCloseODE();

  dJointGroupDestroy(this->contactGroup);

  // Delete all the joint feedbacks.
  for (std::vector<ODEJointFeedback*>::iterator iter =
      this->jointFeedbacks.begin(); iter != this->jointFeedbacks.end(); ++iter)
  {
    delete *iter;
  }
  this->jointFeedbacks.clear();

  if (this->spaceId)
  {
    dSpaceSetCleanup(this->spaceId, 0);
    dSpaceDestroy(this->spaceId);
  }

  if (this->worldId)
    dWorldDestroy(this->worldId);

  this->spaceId = NULL;
  this->worldId = NULL;
}

//////////////////////////////////////////////////
void ODEPhysics::Load(sdf::ElementPtr _sdf)
{
  PhysicsEngine::Load(_sdf);

  this->maxContacts = _sdf->Get<unsigned int>("max_contacts");
  this->SetMaxContacts(this->maxContacts);

  sdf::ElementPtr odeElem = this->sdf->GetElement("ode");
  sdf::ElementPtr solverElem = odeElem->GetElement("solver");

  this->stepType = solverElem->Get<std::string>("type");

  dWorldSetDamping(this->worldId, 0.0001, 0.0001);

  // Help prevent "popping of deeply embedded object
  dWorldSetContactMaxCorrectingVel(this->worldId,
      odeElem->GetElement("constraints")->Get<double>(
        "contact_max_correcting_vel"));

  // This helps prevent jittering problems.
  dWorldSetContactSurfaceLayer(this->worldId,
       odeElem->GetElement("constraints")->Get<double>(
        "contact_surface_layer"));

  // Enable auto-disable by default. Models with joints are excluded from
  // auto-disable
  dWorldSetAutoDisableFlag(this->worldId, 1);

  dWorldSetAutoDisableTime(this->worldId, 1);
  dWorldSetAutoDisableLinearThreshold(this->worldId, 0.1);
  dWorldSetAutoDisableAngularThreshold(this->worldId, 0.1);
  dWorldSetAutoDisableSteps(this->worldId, 5);

  math::Vector3 g = this->sdf->Get<math::Vector3>("gravity");

  if (g == math::Vector3(0, 0, 0))
    gzwarn << "Gravity vector is (0, 0, 0). Objects will float.\n";

  dWorldSetGravity(this->worldId, g.x, g.y, g.z);

  if (odeElem->HasElement("constraints"))
  {
    dWorldSetCFM(this->worldId,
        odeElem->GetElement("constraints")->Get<double>("cfm"));
    dWorldSetERP(this->worldId,
        odeElem->GetElement("constraints")->Get<double>("erp"));
  }
  else
    dWorldSetERP(this->worldId, 0.2);

  dWorldSetQuickStepNumIterations(this->worldId, this->GetSORPGSIters());
  dWorldSetQuickStepW(this->worldId, this->GetSORPGSW());

  // Set the physics update function
  this->SetStepType(this->stepType);
  if (this->physicsStepFunc == NULL)
    gzthrow(std::string("Invalid step type[") + this->stepType);
}

/////////////////////////////////////////////////
void ODEPhysics::OnRequest(ConstRequestPtr &_msg)
{
  msgs::Response response;
  response.set_id(_msg->id());
  response.set_request(_msg->request());
  response.set_response("success");
  std::string *serializedData = response.mutable_serialized_data();

  if (_msg->request() == "physics_info")
  {
    msgs::Physics physicsMsg;
    physicsMsg.set_type(msgs::Physics::ODE);
    physicsMsg.set_solver_type(this->stepType);
    // min_step_size is defined but not yet used
    physicsMsg.set_min_step_size(
        boost::any_cast<double>(this->GetParam("min_step_size")));
    physicsMsg.set_precon_iters(this->GetSORPGSPreconIters());
    physicsMsg.set_iters(this->GetSORPGSIters());
    physicsMsg.set_enable_physics(this->world->GetEnablePhysicsEngine());
    physicsMsg.set_sor(this->GetSORPGSW());
    physicsMsg.set_cfm(this->GetWorldCFM());
    physicsMsg.set_erp(this->GetWorldERP());
    physicsMsg.set_contact_max_correcting_vel(
      this->GetContactMaxCorrectingVel());
    physicsMsg.set_contact_surface_layer(
      this->GetContactSurfaceLayer());
    physicsMsg.mutable_gravity()->CopyFrom(msgs::Convert(this->GetGravity()));
    physicsMsg.set_real_time_update_rate(this->realTimeUpdateRate);
    physicsMsg.set_real_time_factor(this->targetRealTimeFactor);
    physicsMsg.set_max_step_size(this->maxStepSize);

    response.set_type(physicsMsg.GetTypeName());
    physicsMsg.SerializeToString(serializedData);
    this->responsePub->Publish(response);
  }
}

/////////////////////////////////////////////////
void ODEPhysics::OnPhysicsMsg(ConstPhysicsPtr &_msg)
{
  if (_msg->has_solver_type())
    this->SetStepType(_msg->solver_type());

  if (_msg->has_min_step_size())
    this->SetParam("min_step_size", _msg->min_step_size());

  if (_msg->has_precon_iters())
    this->SetSORPGSPreconIters(_msg->precon_iters());

  if (_msg->has_iters())
    this->SetSORPGSIters(_msg->iters());

  if (_msg->has_sor())
    this->SetSORPGSW(_msg->sor());

  if (_msg->has_cfm())
    this->SetWorldCFM(_msg->cfm());

  if (_msg->has_erp())
    this->SetWorldERP(_msg->erp());

  if (_msg->has_enable_physics())
    this->world->EnablePhysicsEngine(_msg->enable_physics());

  if (_msg->has_contact_max_correcting_vel())
    this->SetContactMaxCorrectingVel(_msg->contact_max_correcting_vel());

  if (_msg->has_contact_surface_layer())
    this->SetContactSurfaceLayer(_msg->contact_surface_layer());

  if (_msg->has_gravity())
    this->SetGravity(msgs::Convert(_msg->gravity()));

  if (_msg->has_real_time_factor())
    this->SetTargetRealTimeFactor(_msg->real_time_factor());

  if (_msg->has_real_time_update_rate())
  {
    this->SetRealTimeUpdateRate(_msg->real_time_update_rate());
  }

  if (_msg->has_max_step_size())
  {
    this->SetMaxStepSize(_msg->max_step_size());
  }

  /// Make sure all models get at least on update cycle.
  this->world->EnableAllModels();

  // Parent class handles many generic parameters
  PhysicsEngine::OnPhysicsMsg(_msg);
}



//////////////////////////////////////////////////
void ODEPhysics::Init()
{
}

//////////////////////////////////////////////////
void ODEPhysics::InitForThread()
{
  dAllocateODEDataForThread(dAllocateMaskAll);
}

//////////////////////////////////////////////////
void ODEPhysics::UpdateCollision()
{
  DIAG_TIMER_START("ODEPhysics::UpdateCollision");

  boost::recursive_mutex::scoped_lock lock(*this->physicsUpdateMutex);
  dJointGroupEmpty(this->contactGroup);

  unsigned int i = 0;
  this->collidersCount = 0;
  this->trimeshCollidersCount = 0;
  this->jointFeedbackIndex = 0;

  // Reset the contact count
  this->contactManager->ResetCount();

  // Do collision detection; this will add contacts to the contact group
  dSpaceCollide(this->spaceId, this, CollisionCallback);
  DIAG_TIMER_LAP("ODEPhysics::UpdateCollision", "dSpaceCollide");

  // Generate non-trimesh collisions.
  for (i = 0; i < this->collidersCount; ++i)
  {
    this->Collide(this->colliders[i].first,
        this->colliders[i].second, this->contactCollisions);
  }
  DIAG_TIMER_LAP("ODEPhysics::UpdateCollision", "collideShapes");

  // Generate trimesh collision.
  // This must happen in this thread sequentially
  for (i = 0; i < this->trimeshCollidersCount; ++i)
  {
    ODECollision *collision1 = this->trimeshColliders[i].first;
    ODECollision *collision2 = this->trimeshColliders[i].second;
    this->Collide(collision1, collision2, this->contactCollisions);
  }
  DIAG_TIMER_LAP("ODEPhysics::UpdateCollision", "collideTrimeshes");

  DIAG_TIMER_STOP("ODEPhysics::UpdateCollision");
}

//////////////////////////////////////////////////
void ODEPhysics::UpdatePhysics()
{
  DIAG_TIMER_START("ODEPhysics::UpdatePhysics");

  // need to lock, otherwise might conflict with world resetting
  {
    boost::recursive_mutex::scoped_lock lock(*this->physicsUpdateMutex);

    // Update the dynamical model
    (*physicsStepFunc)(this->worldId, this->maxStepSize);

    math::Vector3 f1, f2, t1, t2;

    // Set the joint contact feedback for each contact.
    for (unsigned int i = 0; i < this->jointFeedbackIndex; ++i)
    {
      Contact *contactFeedback = this->jointFeedbacks[i]->contact;
      Collision *col1 = contactFeedback->collision1;
      Collision *col2 = contactFeedback->collision2;

      GZ_ASSERT(col1 != NULL, "Collision 1 is NULL");
      GZ_ASSERT(col2 != NULL, "Collision 2 is NULL");

      for (int j = 0; j < this->jointFeedbacks[i]->count; ++j)
      {
        dJointFeedback fb = this->jointFeedbacks[i]->feedbacks[j];
        f1.Set(fb.f1[0], fb.f1[1], fb.f1[2]);
        f2.Set(fb.f2[0], fb.f2[1], fb.f2[2]);
        t1.Set(fb.t1[0], fb.t1[1], fb.t1[2]);
        t2.Set(fb.t2[0], fb.t2[1], fb.t2[2]);

        // set force torque in link frame
        this->jointFeedbacks[i]->contact->wrench[j].body1Force =
             col1->GetLink()->GetWorldPose().rot.RotateVectorReverse(f1);
        this->jointFeedbacks[i]->contact->wrench[j].body2Force =
             col2->GetLink()->GetWorldPose().rot.RotateVectorReverse(f2);
        this->jointFeedbacks[i]->contact->wrench[j].body1Torque =
             col1->GetLink()->GetWorldPose().rot.RotateVectorReverse(t1);
        this->jointFeedbacks[i]->contact->wrench[j].body2Torque =
             col2->GetLink()->GetWorldPose().rot.RotateVectorReverse(t2);
      }
    }
  }

  DIAG_TIMER_STOP("ODEPhysics::UpdatePhysics");
}

//////////////////////////////////////////////////
void ODEPhysics::Fini()
{
  PhysicsEngine::Fini();
}

//////////////////////////////////////////////////
void ODEPhysics::Reset()
{
  boost::recursive_mutex::scoped_lock lock(*this->physicsUpdateMutex);
  // Very important to clear out the contact group
  dJointGroupEmpty(this->contactGroup);
}

//////////////////////////////////////////////////
LinkPtr ODEPhysics::CreateLink(ModelPtr _parent)
{
  if (_parent == NULL)
    gzthrow("Link must have a parent\n");

  std::map<std::string, dSpaceID>::iterator iter;
  iter = this->spaces.find(_parent->GetName());

  if (iter == this->spaces.end())
    this->spaces[_parent->GetName()] = dSimpleSpaceCreate(this->spaceId);

  ODELinkPtr link(new ODELink(_parent));

  link->SetSpaceId(this->spaces[_parent->GetName()]);
  link->SetWorld(_parent->GetWorld());

  return link;
}

//////////////////////////////////////////////////
CollisionPtr ODEPhysics::CreateCollision(const std::string &_type,
                                         LinkPtr _body)
{
  ODECollisionPtr collision(new ODECollision(_body));
  ShapePtr shape = this->CreateShape(_type, collision);
  collision->SetShape(shape);
  shape->SetWorld(_body->GetWorld());
  return collision;
}

//////////////////////////////////////////////////
ShapePtr ODEPhysics::CreateShape(const std::string &_type,
                                 CollisionPtr _collision)
{
  ShapePtr shape;
  ODECollisionPtr collision =
    boost::dynamic_pointer_cast<ODECollision>(_collision);

  if (_type == "sphere")
    shape.reset(new ODESphereShape(collision));
  else if (_type == "plane")
    shape.reset(new ODEPlaneShape(collision));
  else if (_type == "box")
    shape.reset(new ODEBoxShape(collision));
  else if (_type == "cylinder")
    shape.reset(new ODECylinderShape(collision));
  else if (_type == "polyline")
    shape.reset(new ODEPolylineShape(collision));
  else if (_type == "multiray")
    shape.reset(new ODEMultiRayShape(collision));
  else if (_type == "mesh" || _type == "trimesh")
    shape.reset(new ODEMeshShape(collision));
  else if (_type == "heightmap")
    shape.reset(new ODEHeightmapShape(collision));
  else if (_type == "map" || _type == "image")
    shape.reset(new MapShape(collision));
  else if (_type == "ray")
    if (_collision)
      shape.reset(new ODERayShape(collision));
    else
      shape.reset(new ODERayShape(this->world->GetPhysicsEngine()));
  else
    gzerr << "Unable to create collision of type[" << _type << "]\n";

  return shape;
}

//////////////////////////////////////////////////
dWorldID ODEPhysics::GetWorldId()
{
  return this->worldId;
}

//////////////////////////////////////////////////
void ODEPhysics::ConvertMass(InertialPtr _inertial, void *_engineMass)
{
  dMass *odeMass = static_cast<dMass*>(_engineMass);

  _inertial->SetMass(odeMass->mass);
  _inertial->SetCoG(odeMass->c[0], odeMass->c[1], odeMass->c[2]);
  _inertial->SetInertiaMatrix(odeMass->I[0*4+0], odeMass->I[1*4+1],
      odeMass->I[2*4+2], odeMass->I[0*4+1],
      odeMass->I[0*4+2], odeMass->I[1*4+2]);
}

//////////////////////////////////////////////////
void ODEPhysics::SetSORPGSPreconIters(unsigned int _iters)
{
  this->sdf->GetElement("ode")->GetElement("solver")->
    GetElement("precon_iters")->Set(_iters);

  dWorldSetQuickStepPreconIterations(this->worldId, _iters);
}

//////////////////////////////////////////////////
void ODEPhysics::SetSORPGSIters(unsigned int _iters)
{
  this->sdf->GetElement("ode")->GetElement(
      "solver")->GetElement("iters")->Set(_iters);
  dWorldSetQuickStepNumIterations(this->worldId, _iters);
}

//////////////////////////////////////////////////
void ODEPhysics::SetSORPGSW(double _w)
{
  this->sdf->GetElement("ode")->GetElement(
      "solver")->GetElement("sor")->Set(_w);
  dWorldSetQuickStepW(this->worldId, _w);
}

//////////////////////////////////////////////////
void ODEPhysics::SetWorldCFM(double _cfm)
{
  sdf::ElementPtr elem = this->sdf->GetElement("ode");
  elem = elem->GetElement("constraints");
  elem->GetElement("cfm")->Set(_cfm);

  dWorldSetCFM(this->worldId, _cfm);
}

//////////////////////////////////////////////////
void ODEPhysics::SetWorldERP(double _erp)
{
  sdf::ElementPtr elem = this->sdf->GetElement("ode");
  elem = elem->GetElement("constraints");
  elem->GetElement("erp")->Set(_erp);
  dWorldSetERP(this->worldId, _erp);
}

//////////////////////////////////////////////////
void ODEPhysics::SetContactMaxCorrectingVel(double _vel)
{
  this->sdf->GetElement("ode")->GetElement(
      "constraints")->GetElement(
        "contact_max_correcting_vel")->Set(_vel);
  dWorldSetContactMaxCorrectingVel(this->worldId, _vel);
}

//////////////////////////////////////////////////
void ODEPhysics::SetContactSurfaceLayer(double _depth)
{
  this->sdf->GetElement("ode")->GetElement(
      "constraints")->GetElement("contact_surface_layer")->Set(_depth);
  dWorldSetContactSurfaceLayer(this->worldId, _depth);
}

//////////////////////////////////////////////////
void ODEPhysics::SetMaxContacts(unsigned int _maxContacts)
{
  this->maxContacts = _maxContacts;
  this->sdf->GetElement("max_contacts")->GetValue()->Set(_maxContacts);
}

//////////////////////////////////////////////////
int ODEPhysics::GetSORPGSPreconIters()
{
  return this->sdf->GetElement("ode")->GetElement(
      "solver")->Get<int>("precon_iters");
}
//////////////////////////////////////////////////
int ODEPhysics::GetSORPGSIters()
{
  return this->sdf->GetElement("ode")->GetElement(
      "solver")->Get<int>("iters");
}

//////////////////////////////////////////////////
double ODEPhysics::GetSORPGSW()
{
  return this->sdf->GetElement("ode")->GetElement(
      "solver")->Get<double>("sor");
}

//////////////////////////////////////////////////
double ODEPhysics::GetWorldCFM()
{
  sdf::ElementPtr elem = this->sdf->GetElement("ode");
  elem = elem->GetElement("constraints");
  return elem->Get<double>("cfm");
}

//////////////////////////////////////////////////
double ODEPhysics::GetWorldERP()
{
  sdf::ElementPtr elem = this->sdf->GetElement("ode");
  elem = elem->GetElement("constraints");
  return elem->Get<double>("erp");
}

//////////////////////////////////////////////////
double ODEPhysics::GetContactMaxCorrectingVel()
{
  return this->sdf->GetElement("ode")->GetElement(
      "constraints")->Get<double>("contact_max_correcting_vel");
}

//////////////////////////////////////////////////
double ODEPhysics::GetContactSurfaceLayer()
{
  return this->sdf->GetElement("ode")->GetElement(
      "constraints")->Get<double>("contact_surface_layer");
}

//////////////////////////////////////////////////
unsigned int ODEPhysics::GetMaxContacts()
{
  return this->maxContacts;
}

//////////////////////////////////////////////////
void ODEPhysics::ConvertMass(void *_engineMass, InertialPtr _inertial)
{
  dMass *odeMass = static_cast<dMass*>(_engineMass);

  odeMass->mass = _inertial->GetMass();
  odeMass->c[0] = _inertial->GetCoG()[0];
  odeMass->c[1] = _inertial->GetCoG()[1];
  odeMass->c[2] = _inertial->GetCoG()[2];

  odeMass->I[0*4+0] = _inertial->GetPrincipalMoments()[0];
  odeMass->I[1*4+1] = _inertial->GetPrincipalMoments()[1];
  odeMass->I[2*4+2] = _inertial->GetPrincipalMoments()[2];

  odeMass->I[0*4+1] = _inertial->GetProductsofInertia()[0];
  odeMass->I[1*4+0] = _inertial->GetProductsofInertia()[0];

  odeMass->I[0*4+2] = _inertial->GetProductsofInertia()[1];
  odeMass->I[1*4+0] = _inertial->GetProductsofInertia()[1];

  odeMass->I[1*4+2] = _inertial->GetProductsofInertia()[2];
  odeMass->I[2*4+1] = _inertial->GetProductsofInertia()[2];
}

//////////////////////////////////////////////////
JointPtr ODEPhysics::CreateJoint(const std::string &_type, ModelPtr _parent)
{
  JointPtr joint;

  if (_type == "prismatic")
    joint.reset(new ODESliderJoint(this->worldId, _parent));
  else if (_type == "screw")
    joint.reset(new ODEScrewJoint(this->worldId, _parent));
  else if (_type == "revolute")
    joint.reset(new ODEHingeJoint(this->worldId, _parent));
  else if (_type == "gearbox")
    joint.reset(new ODEGearboxJoint(this->worldId, _parent));
  else if (_type == "revolute2")
    joint.reset(new ODEHinge2Joint(this->worldId, _parent));
  else if (_type == "ball")
    joint.reset(new ODEBallJoint(this->worldId, _parent));
  else if (_type == "universal")
    joint.reset(new ODEUniversalJoint(this->worldId, _parent));
  else
    gzthrow("Unable to create joint of type[" << _type << "]");

  return joint;
}

//////////////////////////////////////////////////
dSpaceID ODEPhysics::GetSpaceId() const
{
  return this->spaceId;
}

//////////////////////////////////////////////////
std::string ODEPhysics::GetStepType() const
{
  sdf::ElementPtr elem = this->sdf->GetElement("ode")->GetElement("solver");
  return elem->Get<std::string>("type");
}

//////////////////////////////////////////////////
void ODEPhysics::SetStepType(const std::string &_type)
{
  sdf::ElementPtr elem = this->sdf->GetElement("ode")->GetElement("solver");
  elem->GetElement("type")->Set(_type);
  this->stepType = _type;

  // Set the physics update function
  if (this->stepType == "quick")
    this->physicsStepFunc = &dWorldQuickStep;
  else if (this->stepType == "world")
    this->physicsStepFunc = &dWorldStep;
  else
    gzerr << "Invalid step type[" << this->stepType << "]" << std::endl;
}

//////////////////////////////////////////////////
void ODEPhysics::SetGravity(const gazebo::math::Vector3 &_gravity)
{
  this->sdf->GetElement("gravity")->Set(_gravity);
  dWorldSetGravity(this->worldId, _gravity.x, _gravity.y, _gravity.z);
}

//////////////////////////////////////////////////
void ODEPhysics::CollisionCallback(void *_data, dGeomID _o1, dGeomID _o2)
{
  dBodyID b1 = dGeomGetBody(_o1);
  dBodyID b2 = dGeomGetBody(_o2);

  // exit without doing anything if the two bodies are connected by a joint
  if (b1 && b2 && dAreConnectedExcluding(b1, b2, dJointTypeContact))
    return;

  // Get a pointer to the physics engine
  ODEPhysics *self = static_cast<ODEPhysics*>(_data);

  // Check if either are spaces
  if (dGeomIsSpace(_o1) || dGeomIsSpace(_o2))
  {
    dSpaceCollide2(_o1, _o2, self, &CollisionCallback);
  }
  else
  {
    ODECollision *collision1 = NULL;
    ODECollision *collision2 = NULL;

    // Exit if both bodies are not enabled
    if (dGeomGetCategoryBits(_o1) != GZ_SENSOR_COLLIDE &&
        dGeomGetCategoryBits(_o2) != GZ_SENSOR_COLLIDE &&
        ((b1 && b2 && !dBodyIsEnabled(b1) && !dBodyIsEnabled(b2)) ||
        (!b2 && b1 && !dBodyIsEnabled(b1)) ||
        (!b1 && b2 && !dBodyIsEnabled(b2))))
    {
      return;
    }

    // Get pointers to the underlying collisions
    if (dGeomGetClass(_o1) == dGeomTransformClass)
      collision1 =
        static_cast<ODECollision*>(dGeomGetData(dGeomTransformGetGeom(_o1)));
    else
      collision1 = static_cast<ODECollision*>(dGeomGetData(_o1));

    if (dGeomGetClass(_o2) == dGeomTransformClass)
      collision2 =
        static_cast<ODECollision*>(dGeomGetData(dGeomTransformGetGeom(_o2)));
    else
      collision2 = static_cast<ODECollision*>(dGeomGetData(_o2));

    // Make sure both collision pointers are valid.
    if (collision1 && collision2)
    {
      // Add either a tri-mesh collider or a regular collider.
      if (collision1->HasType(Base::MESH_SHAPE) ||
          collision2->HasType(Base::MESH_SHAPE))
        self->AddTrimeshCollider(collision1, collision2);
      else
      {
        self->AddCollider(collision1, collision2);
      }
    }
  }
}


//////////////////////////////////////////////////
void ODEPhysics::Collide(ODECollision *_collision1, ODECollision *_collision2,
                         dContactGeom *_contactCollisions)
{
  // Filter collisions based on contact bitmask if collide_without_contact is
  // on.The bitmask is set mainly for speed improvements otherwise a collision
  // with collide_without_contact may potentially generate a large number of
  // contacts.
  if (_collision1->GetSurface()->collideWithoutContact ||
      _collision2->GetSurface()->collideWithoutContact)
  {
    if ((_collision1->GetSurface()->collideWithoutContactBitmask &
         _collision2->GetSurface()->collideWithoutContactBitmask) == 0)
    {
      return;
    }
  }

  /*
  if (_collision1->GetCollisionId() && _collision2->GetCollisionId())
  {
    const dVector3 *pos1 = (const dVector3*)dGeomGetPosition(_collision1->GetCollisionId());
    const dVector3 *pos2 = (const dVector3*)dGeomGetPosition(_collision2->GetCollisionId());
    std::cout << "1[" << (*pos1)[0]<< " " << (*pos1)[1] << " " << (*pos1)[2] << "] "
      << "2[" << (*pos2)[0]<< " " << (*pos2)[1] << " " << (*pos2)[2] << "]\n";
  }*/

  unsigned int numc = 0;
  dContact contact;

  // maxCollide must less than the size of this->indices. Check the header
  unsigned int maxCollide = MAX_CONTACT_JOINTS;

  // max_contacts specified globally
  if (this->GetMaxContacts() > 0 && this->GetMaxContacts() < MAX_CONTACT_JOINTS)
    maxCollide = this->GetMaxContacts();

  // over-ride with minimum of max_contacts from both collisions
  if (_collision1->GetMaxContacts() < maxCollide)
    maxCollide = _collision1->GetMaxContacts();

  if (_collision2->GetMaxContacts() < maxCollide)
    maxCollide = _collision2->GetMaxContacts();

  // Generate the contacts
  numc = dCollide(_collision1->GetCollisionId(), _collision2->GetCollisionId(),
      MAX_COLLIDE_RETURNS, _contactCollisions, sizeof(_contactCollisions[0]));

  // Return if no contacts.
  if (numc == 0)
    return;

  // Store the indices of the contacts.
  for (int i = 0; i < MAX_CONTACT_JOINTS; i++)
    this->indices[i] = i;

  // Choose only the best contacts if too many were generated.
  if (numc > maxCollide)
  {
    double max = _contactCollisions[maxCollide-1].depth;
    for (unsigned int i = maxCollide; i < numc; ++i)
    {
      if (_contactCollisions[i].depth > max)
      {
        max = _contactCollisions[i].depth;
        this->indices[maxCollide-1] = i;
      }
    }

    // Make sure numc has the valid number of contacts.
    numc = maxCollide;
  }

  // Set the contact surface parameter flags.
  contact.surface.mode = dContactBounce |
                         dContactMu2 |
                         dContactSoftERP |
                         dContactSoftCFM |
                         dContactApprox1 |
                         dContactSlip1 |
                         dContactSlip2;

  ODESurfaceParamsPtr surf1 = _collision1->GetODESurface();
  ODESurfaceParamsPtr surf2 = _collision2->GetODESurface();

  // Compute the CFM and ERP by assuming the two bodies form a
  // spring-damper system.
  double kp = 1.0 / (1.0 / surf1->kp + 1.0 / surf2->kp);
  double kd = surf1->kd + surf2->kd;

  contact.surface.soft_erp = (this->maxStepSize * kp) /
                             (this->maxStepSize * kp + kd);

  contact.surface.soft_cfm = 1.0 / (this->maxStepSize * kp + kd);

  // contact.surface.soft_erp = 0.5*(_collision1->surface->softERP +
  //                                _collision2->surface->softERP);
  // contact.surface.soft_cfm = 0.5*(_collision1->surface->softCFM +
  //                                _collision2->surface->softCFM);

  // assign fdir1 if not set as 0
  math::Vector3 fd = surf1->frictionPyramid.direction1;
  if (fd != math::Vector3::Zero)
  {
    // fdir1 is in body local frame, rotate it into world frame
    fd = _collision1->GetWorldPose().rot.RotateVector(fd);
  }

  /// \TODO: Better treatment when both surfaces have fdir1 specified.
  /// Ideally, we want to use fdir1 specified by surface with
  /// a smaller friction coefficient, but it's not clear how
  /// that can be determined with friction pyramid approximations.
  /// As a hack, we'll simply compare mu1 from
  /// both surfaces for now, and use fdir1 specified by
  /// surface with smaller mu1.
  math::Vector3 fd2 = surf2->frictionPyramid.direction1;
  if (fd2 != math::Vector3::Zero && (fd == math::Vector3::Zero ||
        surf1->frictionPyramid.GetMuPrimary() >
        surf2->frictionPyramid.GetMuPrimary()))
  {
    // fdir1 is in body local frame, rotate it into world frame
    fd2 = _collision2->GetWorldPose().rot.RotateVector(fd2);

    /// \TODO: uncomment gzlog below once we confirm it does not affect
    /// performance
    /// if (fd2 != math::Vector3::Zero && fd != math::Vector3::Zero &&
    ///       _collision1->surface->mu1 > _collision2->surface->mu1)
    ///   gzlog << "both contact surfaces have non-zero fdir1, comparing"
    ///         << " comparing mu1 from both surfaces, and use fdir1"
    ///         << " from surface with smaller mu1\n";
  }

  if (fd != math::Vector3::Zero)
  {
    contact.surface.mode |= dContactFDir1;
    contact.fdir1[0] = fd.x;
    contact.fdir1[1] = fd.y;
    contact.fdir1[2] = fd.z;
  }

  // Set the friction coefficients.
  contact.surface.mu = std::min(surf1->frictionPyramid.GetMuPrimary(),
                                surf2->frictionPyramid.GetMuPrimary());
  contact.surface.mu2 = std::min(surf1->frictionPyramid.GetMuSecondary(),
                                 surf2->frictionPyramid.GetMuSecondary());


  // Set the slip values
  contact.surface.slip1 = std::min(surf1->slip1,
                                   surf2->slip1);
  contact.surface.slip2 = std::min(surf1->slip2,
                                   surf2->slip2);

  // Set the bounce values
  contact.surface.bounce = std::min(surf1->bounce,
                                    surf2->bounce);
  contact.surface.bounce_vel =
    std::min(surf1->bounceThreshold,
             surf2->bounceThreshold);

  // Get the ODE body IDs
  dBodyID b1 = dGeomGetBody(_collision1->GetCollisionId());
  dBodyID b2 = dGeomGetBody(_collision2->GetCollisionId());

  // Add a new contact to the manager. This will return NULL if no one is
  // listening for contact information.
  Contact *contactFeedback = this->contactManager->NewContact(_collision1,
      _collision2, this->world->GetSimTime());

  ODEJointFeedback *jointFeedback = NULL;

  // Create a joint feedback mechanism
  if (contactFeedback)
  {
    if (this->jointFeedbackIndex < this->jointFeedbacks.size())
      jointFeedback = this->jointFeedbacks[this->jointFeedbackIndex];
    else
    {
      jointFeedback = new ODEJointFeedback();
      this->jointFeedbacks.push_back(jointFeedback);
    }

    this->jointFeedbackIndex++;
    jointFeedback->count = 0;
    jointFeedback->contact = contactFeedback;
  }

  // Create a joint for each contact
  for (unsigned int j = 0; j < numc; ++j)
  {
    contact.geom = _contactCollisions[this->indices[j]];

    // Create the contact joint. This introduces the contact constraint to
    // ODE
    dJointID contactJoint =
      dJointCreateContact(this->worldId, this->contactGroup, &contact);

    // Store contact information.
    if (contactFeedback && jointFeedback)
    {
      // Store the contact depth
      contactFeedback->depths[j] = _contactCollisions[this->indices[j]].depth;

      // Store the contact position
      contactFeedback->positions[j].Set(
          _contactCollisions[this->indices[j]].pos[0],
          _contactCollisions[this->indices[j]].pos[1],
          _contactCollisions[this->indices[j]].pos[2]);

      // Store the contact normal
      contactFeedback->normals[j].Set(
          _contactCollisions[this->indices[j]].normal[0],
          _contactCollisions[this->indices[j]].normal[1],
          _contactCollisions[this->indices[j]].normal[2]);

      // Set the joint feedback.
      dJointSetFeedback(contactJoint, &(jointFeedback->feedbacks[j]));

      // Increase the counters
      contactFeedback->count++;
      jointFeedback->count++;
    }

    // Attach the contact joint if collideWithoutContact flags aren't set.
    if (!_collision1->GetSurface()->collideWithoutContact &&
        !_collision2->GetSurface()->collideWithoutContact)
      dJointAttach(contactJoint, b1, b2);
  }
}

/////////////////////////////////////////////////
void ODEPhysics::AddTrimeshCollider(ODECollision *_collision1,
                                    ODECollision *_collision2)
{
  if (this->trimeshCollidersCount >= this->trimeshColliders.size())
    this->trimeshColliders.resize(this->trimeshColliders.size() + 100);

  this->trimeshColliders[this->trimeshCollidersCount].first  = _collision1;
  this->trimeshColliders[this->trimeshCollidersCount].second = _collision2;
  this->trimeshCollidersCount++;
}

/////////////////////////////////////////////////
void ODEPhysics::AddCollider(ODECollision *_collision1,
                             ODECollision *_collision2)
{
  if (this->collidersCount >= this->colliders.size())
    this->colliders.resize(this->colliders.size() + 100);

  this->colliders[this->collidersCount].first  = _collision1;
  this->colliders[this->collidersCount].second = _collision2;
  this->collidersCount++;
}

/////////////////////////////////////////////////
void ODEPhysics::DebugPrint() const
{
  dBodyID b;
  std::cout << "Debug Print[" << dWorldGetBodyCount(this->worldId) << "]\n";
  for (int i = 0; i < dWorldGetBodyCount(this->worldId); ++i)
  {
    b = dWorldGetBody(this->worldId, i);
    ODELink *link = static_cast<ODELink*>(dBodyGetData(b));
    math::Pose pose = link->GetWorldPose();
    const dReal *pos = dBodyGetPosition(b);
    const dReal *rot = dBodyGetRotation(b);
    math::Vector3 dpos(pos[0], pos[1], pos[2]);
    math::Quaternion drot(rot[0], rot[1], rot[2], rot[3]);

    std::cout << "Body[" << link->GetScopedName() << "]\n";
    std::cout << "  World: Pos[" << dpos << "] Rot[" << drot << "]\n";
    if (pose.pos != dpos)
      std::cout << "    Incorrect world pos[" << pose.pos << "]\n";
    if (pose.rot != drot)
      std::cout << "    Incorrect world rot[" << pose.rot << "]\n";

    dMass mass;
    dBodyGetMass(b, &mass);
    std::cout << "  Mass[" << mass.mass << "] COG[" << mass.c[0]
              << " " << mass.c[1] << " " << mass.c[2] << "]\n";

    dGeomID g = dBodyGetFirstGeom(b);
    while (g)
    {
      ODECollision *coll = static_cast<ODECollision*>(dGeomGetData(g));

      pose = coll->GetWorldPose();
      const dReal *gpos = dGeomGetPosition(g);
      const dReal *grot = dGeomGetRotation(g);
      dpos.Set(gpos[0], gpos[1], gpos[2]);
      drot.Set(grot[0], grot[1], grot[2], grot[3]);

      std::cout << "    Geom[" << coll->GetScopedName() << "]\n";
      std::cout << "      World: Pos[" << dpos << "] Rot[" << drot << "]\n";

      if (pose.pos != dpos)
        std::cout << "      Incorrect world pos[" << pose.pos << "]\n";
      if (pose.rot != drot)
        std::cout << "      Incorrect world rot[" << pose.rot << "]\n";

      g = dBodyGetNextGeom(g);
    }
  }
}

/////////////////////////////////////////////////
void ODEPhysics::SetSeed(uint32_t _seed)
{
  dRandSetSeed(_seed);
}

//////////////////////////////////////////////////
bool ODEPhysics::SetParam(const std::string &_key, const boost::any &_value)
{
  sdf::ElementPtr odeElem = this->sdf->GetElement("ode");
  GZ_ASSERT(odeElem != NULL, "ODE SDF element does not exist");

  if (_key == "solver_type")
  {
    std::string value;
    try
    {
      value = boost::any_cast<std::string>(_value);
    }
    catch(const boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
      return false;
    }
    this->SetStepType(value);
  }
  else if (_key == "cfm")
  {
    double value;
    try
    {
      value = boost::any_cast<double>(_value);
    }
    catch(const boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
      return false;
    }
    odeElem->GetElement("constraints")->GetElement("cfm")->Set(value);
    dWorldSetCFM(this->worldId, value);
  }
  else if (_key == "erp")
  {
    double value;
    try
    {
      value = boost::any_cast<double>(_value);
    }
    catch(const boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
      return false;
    }
    odeElem->GetElement("constraints")->GetElement("erp")->Set(value);
    dWorldSetERP(this->worldId, value);
  }
  else if (_key == "precon_iters")
  {
    int value;
    try
    {
      try
      {
        value = boost::any_cast<int>(_value);
      }
      catch(const boost::bad_any_cast &e)
      {
        value = boost::any_cast<unsigned int>(_value);
      }
    }
    catch(const boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
      return false;
    }
    odeElem->GetElement("solver")->GetElement("precon_iters")->Set(value);
    dWorldSetQuickStepPreconIterations(this->worldId, value);
  }
  else if (_key == "iters")
  {
    int value;
    try
    {
      try
      {
        value = boost::any_cast<int>(_value);
      }
      catch(const boost::bad_any_cast &e)
      {
        value = boost::any_cast<unsigned int>(_value);
      }
    }
    catch(const boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
      return false;
    }
    odeElem->GetElement("solver")->GetElement("iters")->Set(value);
    dWorldSetQuickStepNumIterations(this->worldId, value);
  }
  else if (_key == "sor")
  {
    double value = boost::any_cast<double>(_value);
    odeElem->GetElement("solver")->GetElement("sor")->Set(value);
    dWorldSetQuickStepW(this->worldId, value);
  }
  else if (_key == "contact_max_correcting_vel")
  {
    double value;
    try
    {
      value = boost::any_cast<double>(_value);
    }
    catch(const boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
      return false;
    }
    odeElem->GetElement("constraints")->GetElement(
        "contact_max_correcting_vel")->Set(value);
    dWorldSetContactMaxCorrectingVel(this->worldId, value);
  }
  else if (_key == "contact_surface_layer")
  {
    double value;
    try
    {
      value = boost::any_cast<double>(_value);
    }
    catch(const boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
      return false;
    }
    odeElem->GetElement("constraints")->GetElement(
        "contact_surface_layer")->Set(value);
    dWorldSetContactSurfaceLayer(this->worldId, value);
  }
  else if (_key == "max_contacts")
  {
    int value;
    try
    {
      try
      {
        value = boost::any_cast<int>(_value);
      }
      catch(const boost::bad_any_cast &e)
      {
        value = boost::any_cast<unsigned int>(_value);
      }
    }
    catch(const boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
      return false;
    }
    this->sdf->GetElement("max_contacts")->GetValue()->Set(value);
  }
  else if (_key == "min_step_size")
  {
    /// TODO: Implement min step size param
    double value;
    try
    {
      value = boost::any_cast<double>(_value);
    }
    catch(const boost::bad_any_cast &e)
    {
      gzerr << "boost any_cast error:" << e.what() << "\n";
      return false;
    }
    odeElem->GetElement("solver")->GetElement("min_step_size")->Set(value);
  }
  else if (_key == "max_step_size")
  {
    this->SetMaxStepSize(boost::any_cast<double>(_value));
  }
  else if (_key == "sor_lcp_tolerance")
  {
    dWorldSetQuickStepTolerance(this->worldId,
        boost::any_cast<double>(_value));
  }
  else if (_key == "rms_error_tolerance")
  {
    gzwarn << "please use sor_lcp_tolerance in the future.\n";
    dWorldSetQuickStepTolerance(this->worldId,
        boost::any_cast<double>(_value));
  }
  else if (_key == "inertia_ratio_reduction")
  {
    dWorldSetQuickStepInertiaRatioReduction(this->worldId,
        boost::any_cast<bool>(_value));
  }
  else if (_key == "contact_residual_smoothing")
  {
    dWorldSetQuickStepContactResidualSmoothing(this->worldId,
      boost::any_cast<double>(_value));
  }
  else if (_key == "experimental_row_reordering")
  {
    dWorldSetQuickStepExperimentalRowReordering(this->worldId,
      boost::any_cast<bool>(_value));
  }
  else if (_key == "warm_start_factor")
  {
    dWorldSetQuickStepWarmStartFactor(this->worldId,
      boost::any_cast<double>(_value));
  }
  else if (_key == "extra_friction_iterations")
  {
    dWorldSetQuickStepExtraFrictionIterations(this->worldId,
      boost::any_cast<int>(_value));
  }
  else
  {
    gzwarn << _key << " is not supported in ode" << std::endl;
    return false;
  }
  return true;
}

//////////////////////////////////////////////////
boost::any ODEPhysics::GetParam(const std::string &_key) const
{
  sdf::ElementPtr odeElem = this->sdf->GetElement("ode");
  GZ_ASSERT(odeElem != NULL, "ODE SDF element does not exist");

  if (_key == "solver_type")
  {
    return odeElem->GetElement("solver")->Get<std::string>("type");
  }
  else if (_key == "cfm")
  {
    return odeElem->GetElement("constraints")->Get<double>("cfm");
  }
  else if (_key == "erp")
    return odeElem->GetElement("constraints")->Get<double>("erp");
  else if (_key == "precon_iters")
    return odeElem->GetElement("solver")->Get<int>("precon_iters");
  else if (_key == "iters")
    return odeElem->GetElement("solver")->Get<int>("iters");
  else if (_key == "sor")
    return odeElem->GetElement("solver")->Get<double>("sor");
  else if (_key == "contact_max_correcting_vel")
    return odeElem->GetElement("constraints")->Get<double>(
        "contact_max_correcting_vel");
  else if (_key == "contact_surface_layer")
    return odeElem->GetElement("constraints")->Get<double>(
        "contact_surface_layer");
  else if (_key == "max_contacts")
    return this->sdf->Get<int>("max_contacts");
  else if (_key == "min_step_size")
    return odeElem->GetElement("solver")->Get<double>("min_step_size");
  else if (_key == "max_step_size")
    return this->GetMaxStepSize();
  else if (_key == "sor_lcp_tolerance")
    return dWorldGetQuickStepTolerance(this->worldId);
  else if (_key == "rms_error_tolerance")
  {
    gzwarn << "please use sor_lcp_tolerance in the future.\n";
    return dWorldGetQuickStepTolerance(this->worldId);
  }
  else if (_key == "rms_error")
    return dWorldGetQuickStepRMSDeltaLambda(this->worldId);
  else if (_key == "constraint_residual")
    return dWorldGetQuickStepRMSConstraintResidual(this->worldId);
  else if (_key == "num_contacts")
    return dWorldGetQuickStepNumContacts(this->worldId);
  else if (_key == "inertia_ratio_reduction")
    return dWorldGetQuickStepInertiaRatioReduction(this->worldId);
  else if (_key == "contact_residual_smoothing")
    return dWorldGetQuickStepContactResidualSmoothing (this->worldId);
  else if (_key == "experimental_row_reordering")
    return dWorldGetQuickStepExperimentalRowReordering (this->worldId);
  else if (_key == "warm_start_factor")
    return dWorldGetQuickStepWarmStartFactor (this->worldId);
  else if (_key == "extra_friction_iterations")
    return dWorldGetQuickStepExtraFrictionIterations (this->worldId);
  else
  {
    gzwarn << _key << " is not supported in ode" << std::endl;
    return 0;
  }
}
