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
/* Desc: Base class for all models.
 * Author: Nathan Koenig and Andrew Howard
 * Date: 8 May 2003
 * SVN: $Id$
 */

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <sstream>
#include <iostream>
#include <float.h>

#include "Events.hh"
#include "Visual.hh"
#include "Light.hh"
#include "GraphicsIfaceHandler.hh"
#include "Global.hh"
#include "GazeboError.hh"
#include "GazeboMessage.hh"
#include "XMLConfig.hh"
#include "World.hh"
#include "Body.hh"
#include "HingeJoint.hh"
#include "PhysicsEngine.hh"
#include "Controller.hh"
#include "ControllerFactory.hh"
#include "IfaceFactory.hh"
#include "Model.hh"

using namespace gazebo;
using namespace physics;


uint Model::lightNumber = 0;

class BodyUpdate_TBB
{
  public: BodyUpdate_TBB(std::vector<Body*> *bodies) : bodies(bodies) {}

  public: void operator() (const tbb::blocked_range<size_t> &r) const
  {
    for (size_t i=r.begin(); i != r.end(); i++)
    {
      (*this->bodies)[i]->Update();
    }
  }

  private: std::vector<Body*> *bodies;
};


////////////////////////////////////////////////////////////////////////////////
// Constructor
Model::Model(Common *parent)
    : Entity(parent)
{
  this->AddType(MODEL);

  this->joint = NULL;

  Param::Begin(&this->parameters);
  this->canonicalBodyNameP = new ParamT<std::string>("canonicalBody",
                                                   std::string(),0);

  this->xyzP = new ParamT<Vector3>("xyz", Vector3(0,0,0), 0);
  this->xyzP->Callback(&Entity::SetRelativePosition, (Entity*)this);

  this->rpyP = new ParamT<Quatern>("rpy", Quatern(1,0,0,0), 0);
  this->rpyP->Callback( &Entity::SetRelativeRotation, (Entity*)this);

  this->enableGravityP = new ParamT<bool>("enable_gravity", true, 0);
  this->enableGravityP->Callback( &Model::SetGravityMode, this );

  this->enableFrictionP = new ParamT<bool>("enable_friction", true, 0);
  this->enableFrictionP->Callback( &Model::SetFrictionMode, this );

  this->collideP = new ParamT<std::string>("collide", "all", 0);
  this->collideP->Callback( &Model::SetCollideMode, this );

  this->laserFiducialP = new ParamT<int>("laser_fiducial_id", -1, 0);
  this->laserFiducialP->Callback( &Model::SetLaserFiducialId, this );

  this->laserRetroP = new ParamT<float>("laser_retro", -1, 0);
  this->laserRetroP->Callback( &Model::SetLaserRetro, this );
  Param::End();

  this->graphicsHandler = NULL;
  this->parentBodyNameP = NULL;
  this->myBodyNameP = NULL;

  this->light = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
Model::~Model()
{
  /*std::vector<Common*>::iterator eiter;
  for (eiter =this->children.begin(); eiter != this->children.end();)
    if (*eiter && (*eiter)->HasType(BODY))
    {
      delete (*eiter);
      *eiter = NULL;
      this->children.erase(eiter); // effectively remove child
    }
    else
      eiter++;
      */

  JointContainer::iterator jiter;
  std::map< std::string, Controller* >::iterator citer;

  if (this->light)
  {
    delete this->light;
  }

  if (this->graphicsHandler)
  {
    delete this->graphicsHandler;
    this->graphicsHandler = NULL;
  }

  for (jiter = this->joints.begin(); jiter != this->joints.end(); jiter++)
    if (*jiter)
      delete *jiter;
  this->joints.clear();

  for (citer = this->controllers.begin();
       citer != this->controllers.end(); citer++)
  {
    if (citer->second)
    {
      delete citer->second;
      citer->second = NULL;
    }
  }
  this->controllers.clear();

  if (this->parentBodyNameP)
  {
    delete this->parentBodyNameP;
    this->parentBodyNameP = NULL;
  }

  if (this->myBodyNameP)
  {
    delete this->myBodyNameP;
    this->myBodyNameP = NULL;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Load the model
void Model::Load(XMLConfigNode *node, bool removeDuplicate)
{
  Entity::Load(node);

  XMLConfigNode *childNode;
  std::string scopedName;
  Pose3d pose;
  Common* dup;

  this->staticP->Load(node);

  scopedName = this->GetScopedName();

  dup = Common::GetByName(scopedName);

  // Look for existing models by the same name
  if(dup != NULL && dup != this)
  {
    if(!removeDuplicate)
    {
      gzthrow("Duplicate model name[" + scopedName + "]\n");
    }
    else
    {
      // Delete the existing one (this should only be reached when called
      // via the factory interface).
      event::Events::deleteEntitySignal(scopedName.c_str());
    }
  }

  this->canonicalBodyNameP->Load(node);

  if ( (childNode = node->GetChild("origin")) != NULL)
  {
    this->xyzP->Load(childNode);
    this->rpyP->Load(childNode);
  }


  this->enableGravityP->Load(node);
  this->enableFrictionP->Load(node);
  this->collideP->Load(node);
  this->laserFiducialP->Load(node);
  this->laserRetroP->Load(node);

  this->SetStatic( **(this->staticP) );

  // Get the position and orientation of the model (relative to parent)
  pose.Reset();
  pose.pos = **this->xyzP;
  pose.rot = **this->rpyP;

  this->SetRelativePose( pose );

  this->LoadPhysical(node);

  // Record the model's initial pose (for reseting)
  this->SetInitPose(pose);

  // Load controllers
  childNode = node->GetChild("controller");
  while (childNode)
  {
    this->LoadController(childNode);
    childNode = childNode->GetNext("controller");
  }

  // Create a default body if one does not exist in the XML file
  if (this->children.size() <= 0)
  {
    std::ostringstream bodyName;

    bodyName << this->GetName() << "_body";

    // Create an empty body for the model
    Body *body = this->CreateBody();
    body->SetName(bodyName.str());

    this->canonicalBodyNameP->SetValue( bodyName.str() );
  }


  if (this->canonicalBodyNameP->GetValue().empty())
  {
    /// FIXME: Model::pose is set to the pose of first body
    ///        seems like there should be a warning for users
    for (unsigned int i=0; i < this->children.size(); i++)
    {
      if (this->children[i]->HasType(BODY))
      {
        this->canonicalBodyNameP->SetValue( this->children[i]->GetName() );
        break;
      }
    }
  }

  this->canonicalBody = (Body*)this->GetChild(**this->canonicalBodyNameP);

  // This must be placed after creation of the bodies
  // Static variable overrides the gravity
  if (**this->staticP == false)
    this->SetGravityMode( **this->enableGravityP );

  //global fiducial and retro id
  if (**this->laserFiducialP != -1.0 )
    this->SetLaserFiducialId(**this->laserFiducialP);

  if (**this->laserRetroP != -1.0)
    this->SetLaserRetro(**this->laserRetroP);

  // Create the graphics iface handler
  this->graphicsHandler = new GraphicsIfaceHandler(this->GetWorld());
  this->graphicsHandler->Load(this->GetScopedName(), this);
}

////////////////////////////////////////////////////////////////////////////////
// Save the model in XML format
void Model::Save(std::string &prefix, std::ostream &stream)
{
  std::string p = prefix + "  ";
  std::string typeName;
  //std::map<std::string, Body* >::iterator bodyIter;
  std::vector<Common* >::iterator bodyIter;
  std::map<std::string, Controller* >::iterator contIter;
  JointContainer::iterator jointIter;

  this->xyzP->SetValue( this->GetRelativePose().pos );
  this->rpyP->SetValue( this->GetRelativePose().rot );


  stream << prefix << "<model";
  stream << " name=\"" << this->nameP->GetValue() << "\">\n"; 
  stream << prefix << "  " << *(this->xyzP) << "\n";
  stream << prefix << "  " << *(this->rpyP) << "\n";
  stream << prefix << "  " << *(this->enableGravityP) << "\n";
  stream << prefix << "  " << *(this->enableFrictionP) << "\n";
  stream << prefix << "  " << *(this->collideP) << "\n";

  stream << prefix << "  " << *(this->staticP) << "\n";

  for (bodyIter=this->children.begin(); bodyIter!=this->children.end(); bodyIter++)
  {
    stream << "\n";
    Entity *entity = (Entity*)*bodyIter;
    if (entity && entity->HasType(BODY))
    {
      Body *body = (Body*)(entity);
      body->Save(p, stream);
    }
  }

  // Save all the joints
  for (jointIter = this->joints.begin(); jointIter != this->joints.end(); 
      jointIter++)
    if (*jointIter)
      (*jointIter)->Save(p, stream);

  // Save all the controllers
  for (contIter=this->controllers.begin();
      contIter!=this->controllers.end(); contIter++)
  {
    if (contIter->second)
      contIter->second->Save(p, stream);
  }

  if (this->parentBodyNameP && this->myBodyNameP)
  {
    stream << prefix << "  <attach>\n";
    stream << prefix << "    " << *(this->parentBodyNameP) << "\n";
    stream << prefix << "    " << *(this->myBodyNameP) << "\n";
    stream << prefix << "  </attach>\n";
  }

  // Save all child models
  std::vector< Common* >::iterator eiter;
  for (eiter = this->children.begin(); eiter != this->children.end(); eiter++)
  {
    if (*eiter && (*eiter)->HasType(MODEL))
    {
      Model *cmodel = (Model*)*eiter;
      cmodel->Save(p, stream);
    }
  }

  stream << prefix << "</model:" << typeName << ">\n";
}

////////////////////////////////////////////////////////////////////////////////
// Initialize the model
void Model::Init()
{
  std::vector<Common* >::iterator biter;
  std::map<std::string, Controller* >::iterator contIter;

  this->graphicsHandler->Init();

  for (biter = this->children.begin(); biter!=this->children.end(); biter++)
  {
    if (*biter)
    {
      if ((*biter)->HasType(BODY))
        ((Body*)*biter)->Init();
      else if ((*biter)->HasType(MODEL))
        ((Model*)*biter)->Init();
    }
  }

  for (contIter=this->controllers.begin();
       contIter!=this->controllers.end(); contIter++)
  {
    contIter->second->Init();
  }

  this->InitChild();

  // this updates the relativePose of the Model Entity
  // set model pointer of the canonical body so when body updates,
  // one can callback to the model
  //Body* cb = this->GetCanonicalBody();
  //if (cb != NULL)
    //cb->SetCanonicalModel(this);
}

////////////////////////////////////////////////////////////////////////////////
// Update the model
void Model::Update()
{
  if (this->controllers.size() == 0 && this->IsStatic())
    return;

  if (!this->IsStatic())
  {
    tbb::parallel_for( tbb::blocked_range<size_t>(0, this->bodies.size(), 10),
        BodyUpdate_TBB(&this->bodies) );
  }

  this->contacts.clear();

  std::map<std::string, Controller* >::iterator contIter;
  for (contIter=this->controllers.begin();
      contIter!=this->controllers.end(); contIter++)
  {
    if (contIter->second)
    {
      contIter->second->Update();
    }
  }

  // NATY: Make this work without renderstate
  /*if (RenderState::GetShowJoints())
  {
    JointContainer::iterator jointIter;
    for (jointIter = this->joints.begin(); 
         jointIter != this->joints.end(); jointIter++)
    {
      (*jointIter)->Update();
    }
  }*/

  this->UpdateChild();
}

void Model::OnPoseChange()
{
  /// set the pose of the model, which is the pose of the canonical body
  /// this updates the relativePose of the Model Entity
  //Body* cb = this->GetCanonicalBody();
  //if (cb != NULL)
    //this->SetWorldPose(cb->GetWorldPose(),false);
}

////////////////////////////////////////////////////////////////////////////////
// Remove a child
void Model::RemoveChild(Entity *child)
{
  JointContainer::iterator jiter;

  if (child->HasType(BODY))
  {
    bool done = false;

    while (!done)
    {
      done = true;

      for (jiter = this->joints.begin(); jiter != this->joints.end(); jiter++)
      {
        if (!(*jiter))
          continue;

        Body *jbody0 = (*jiter)->GetJointBody(0);
        Body *jbody1 = (*jiter)->GetJointBody(1);

        if (!jbody0 || !jbody1 || jbody0->GetName() == child->GetName() ||
            jbody1->GetName() == child->GetName() ||
            jbody0->GetName() == jbody1->GetName())
        {
          Joint *joint = *jiter;
          this->joints.erase( jiter );
          done = false;
          delete joint;
          break;
        }
      }
    }
  }

  Entity::RemoveChild(child);

  std::vector<Common*>::iterator iter;
  for (iter =this->children.begin(); iter != this->children.end(); iter++)
    if (*iter && (*iter)->HasType(BODY))
      ((Body*)*iter)->SetEnabled(true);

}

////////////////////////////////////////////////////////////////////////////////
/// Primarily used to update the graphics interfaces
void Model::GraphicsUpdate()
{
  if (this->graphicsHandler)
  {
    this->graphicsHandler->Update();
  }
}


////////////////////////////////////////////////////////////////////////////////
// Finalize the model
void Model::Fini()
{
  std::vector<Common* >::iterator biter;
  std::map<std::string, Controller* >::iterator contIter;

  for (contIter = this->controllers.begin();
       contIter != this->controllers.end(); contIter++)
  {
    contIter->second->Fini();
  }

  for (biter=this->children.begin(); biter != this->children.end(); biter++)
  {
    if (*biter && (*biter)->HasType(BODY))
    {
      Body *body = (Body*)*biter;
      body->Fini();
    }
  }

  if (this->graphicsHandler)
  {
    delete this->graphicsHandler;
    this->graphicsHandler = NULL;
  }

  std::vector< Common* >::iterator iter;
  for (iter = this->children.begin(); iter != this->children.end(); iter++)
  {
    if (*iter && (*iter)->HasType(MODEL))
    {
      Model *m = (Model*)*iter;
      m->Fini();
    }
  }

  this->FiniChild();
}

////////////////////////////////////////////////////////////////////////////////
// Reset the model
void Model::Reset()
{
  JointContainer::iterator jiter;
  std::vector< Common* >::iterator biter;
  std::map<std::string, Controller* >::iterator citer;
  Vector3 v(0,0,0);

  this->SetRelativePose(this->initPose);  // this has to be relative for nested models to work

  for (citer=this->controllers.begin(); citer!=this->controllers.end(); citer++)
  {
    citer->second->Reset();
  }

  for (jiter=this->joints.begin(); jiter!=this->joints.end(); jiter++)
  {
    (*jiter)->Reset();
  }

  for (biter=this->children.begin(); biter != this->children.end(); biter++)
  {
    if (*biter && (*biter)->HasType(BODY))
    {
      Body *body = (Body*)*biter;
      body->SetLinearVel(v);
      body->SetAngularVel(v);
      body->SetForce(v);
      body->SetTorque(v);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Set the initial pose
void Model::SetInitPose(const Pose3d &pose)
{
  this->initPose = pose;
}

////////////////////////////////////////////////////////////////////////////////
// Get the initial pose
const Pose3d &Model::GetInitPose() const
{
  return this->initPose;
}

////////////////////////////////////////////////////////////////////////////////
/// Set the linear velocity of the model
void Model::SetLinearVel( const Vector3 &vel )
{
  Body *body;
  std::vector<Common* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->HasType(BODY))
    {
      body = (Body*)*iter;
      body->SetEnabled(true);
      body->SetLinearVel( vel );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Set the angular velocity of the model
void Model::SetAngularVel( const Vector3 &vel )
{
  Body *body;
  std::vector<Common* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->HasType(BODY))
    {
      body = (Body*)*iter;
      body->SetEnabled(true);
      body->SetAngularVel( vel );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Set the linear acceleration of the model
void Model::SetLinearAccel( const Vector3 &accel )
{
  Body *body;
  std::vector<Common* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->HasType(BODY))
    {
      body = (Body*)*iter;
      body->SetEnabled(true);
      body->SetLinearAccel( accel );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Set the angular acceleration of the model
void Model::SetAngularAccel( const Vector3 &accel )
{
  Body *body;
  std::vector<Common* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->HasType(BODY))
    {
      body = (Body*)*iter;
      body->SetEnabled(true);
      body->SetAngularAccel( accel );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Get the linear velocity of the model
Vector3 Model::GetRelativeLinearVel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetRelativeLinearVel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the linear velocity of the entity in the world frame
Vector3 Model::GetWorldLinearVel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetWorldLinearVel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the angular velocity of the model
Vector3 Model::GetRelativeAngularVel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetRelativeAngularVel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the angular velocity of the model in the world frame
Vector3 Model::GetWorldAngularVel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetWorldAngularVel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}


////////////////////////////////////////////////////////////////////////////////
/// Get the linear acceleration of the model
Vector3 Model::GetRelativeLinearAccel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetRelativeLinearAccel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the linear acceleration of the model in the world frame
Vector3 Model::GetWorldLinearAccel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetWorldLinearAccel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the angular acceleration of the model
Vector3 Model::GetRelativeAngularAccel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetRelativeAngularAccel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the angular acceleration of the model in the world frame
Vector3 Model::GetWorldAngularAccel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetWorldAngularAccel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the size of the bounding box
void Model::GetBoundingBox(Vector3 &min, Vector3 &max) const
{
  Vector3 bbmin, bbmax;
  std::vector<Common* >::const_iterator iter;

  min.Set(FLT_MAX, FLT_MAX, FLT_MAX);
  max.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->HasType(BODY))
    {
      Body *body = (Body*)*iter;
      body->GetBoundingBox(bbmin, bbmax);
      min.x = std::min(bbmin.x, min.x);
      min.y = std::min(bbmin.y, min.y);
      min.z = std::min(bbmin.z, min.z);

      max.x = std::max(bbmax.x, max.x);
      max.y = std::max(bbmax.y, max.y);
      max.z = std::max(bbmax.z, max.z);
    }
  }
}
 
////////////////////////////////////////////////////////////////////////////////
// Create and return a new body
Body *Model::CreateBody()
{
  Body *body = this->GetWorld()->GetPhysicsEngine()->CreateBody(this);
  this->bodies.push_back(body);

  // Create a new body
  return body;
}

////////////////////////////////////////////////////////////////////////////////
/// Get the number of joints
unsigned int Model::GetJointCount() const
{
  return this->joints.size();
}

////////////////////////////////////////////////////////////////////////////////
/// Get a joing by index
Joint *Model::GetJoint( unsigned int index ) const
{
  if (index >= this->joints.size())
    gzthrow("Invalid joint index[" << index << "]\n");

  return this->joints[index];
}

////////////////////////////////////////////////////////////////////////////////
// Get a joint by name
Joint *Model::GetJoint(std::string name)
{
  JointContainer::iterator iter;

  for (iter = this->joints.begin(); iter != this->joints.end(); iter++)
    if ( (*iter)->GetName() == name)
      return (*iter);

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Load a new body helper function
void Model::LoadBody(XMLConfigNode *node)
{
  if (!node)
    gzthrow("Trying to load a body with NULL XML information");

  // Create a new body
  Body *body = this->CreateBody();

  // Load the body using the config node. This also loads all of the
  // bodies geometries
  body->Load(node);
}

////////////////////////////////////////////////////////////////////////////////
// Load a new joint helper function
void Model::LoadJoint(XMLConfigNode *node)
{
  if (!node)
    gzthrow("Trying to load a joint with NULL XML information");

  Joint *joint;

  std::string type = node->GetString("type","hinge",1);
  std::cout << "Load Joint[" << type << "]\n";

  joint = this->GetWorld()->GetPhysicsEngine()->CreateJoint( type );

  joint->SetModel(this);

  // Load each joint
  joint->Load(node);

  if (this->GetJoint( joint->GetName() ) != NULL)
    gzthrow( "can't have two joint with the same name");

  this->joints.push_back( joint );
}

////////////////////////////////////////////////////////////////////////////////
/// Load a controller helper function
void Model::LoadController(XMLConfigNode *node)
{
  if (!node)
    gzthrow( "node parameter is NULL" );

  Controller *controller=0;
  std::ostringstream stream;

  // Get the controller's type
  std::string controllerType = node->GetName();

  // Get the unique name of the controller
  std::string controllerName = node->GetString("name",std::string(),1);
  
  // See if the controller is in a plugin
  std::string pluginName = node->GetString("plugin","",0);
  if (pluginName != "")
	  ControllerFactory::LoadPlugin(pluginName, controllerType);

  // Create the controller based on it's type
  controller = ControllerFactory::NewController(controllerType, this);

  if (controller)
  {
    // Load the controller
    try
    {
      controller->Load(node);
    }
    catch (GazeboError e)
    {
      gzerr(0) << "Error Loading Controller[" <<  controllerName
      << "]\n" << e << std::endl;
      delete controller;
      controller = NULL;
      return;
    }

    // Store the controller
    this->controllers[controllerName] = controller;
  }
  else
  {
    gzmsg(0) << "Unknown controller[" << controllerType << "]\n";
  }
}

////////////////////////////////////////////////////////////////////////////////
// Return the default body
Body *Model::GetBody()
{
  return dynamic_cast<Body*>(this->children.front());
}

////////////////////////////////////////////////////////////////////////////////
/// Get a sensor by name
Sensor *Model::GetSensor(const std::string &name) const
{
  Sensor *sensor = NULL;
  std::vector< Common* >::const_iterator biter;

  for (biter=this->children.begin(); biter != this->children.end(); biter++)
  {
    if ( *biter && (*biter)->HasType(BODY))
    {
      Body *body = (Body*)*biter;
      if ((sensor = body->GetSensor(name)) != NULL)
        break;
    }
  }

  return sensor;
}
 
////////////////////////////////////////////////////////////////////////////////
/// Get a geom by name
Geom *Model::GetGeom(const std::string &name) const
{
  Geom *geom = NULL;
  std::vector< Common* >::const_iterator biter;

  for (biter=this->children.begin(); biter != this->children.end(); biter++)
  {
    if (*biter && (*biter)->HasType(BODY))
    {
      Body *body = (Body*)*biter;
      if ((geom = body->GetGeom(name)) != NULL)
        break;
    }
  }

  return geom;
}

////////////////////////////////////////////////////////////////////////////////
/// Get a body by name
Body *Model::GetBody(const std::string &name)
{
  std::vector< Common* >::const_iterator biter;

  if (name == "canonical")
    return this->GetCanonicalBody();

  for (biter=this->children.begin(); biter != this->children.end(); biter++)
  {
    if ((*biter)->GetName() == name)
      return (Body*)*biter;
  }
 
  return NULL;
}


////////////////////////////////////////////////////////////////////////////////
// Detach from parent model
void Model::Detach()
{
  if (this->joint)
    delete this->joint;
  this->joint = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Attach this model to its parent
void Model::Attach(XMLConfigNode *node)
{
  Model *parentModel = NULL;

  Param::Begin(&this->parameters);
  this->parentBodyNameP = new ParamT<std::string>("parentBody","canonical",1);
  this->myBodyNameP = new ParamT<std::string>("myBody",this->canonicalBodyNameP->GetValue(),1);
  Param::End();

  if (node)
  {
    this->parentBodyNameP->Load(node);
    this->myBodyNameP->Load(node);
  }

  if (this->parent->HasType(MODEL))
    parentModel = (Model*)this->parent;

  if (parentModel == NULL)
    gzthrow("Parent cannot be NULL when attaching two models");

  this->joint =this->GetWorld()->GetPhysicsEngine()->CreateJoint("hinge");

  Body *myBody = this->GetBody(**(this->myBodyNameP));
  Body *pBody = parentModel->GetBody(**(this->parentBodyNameP));

  if (myBody == NULL)
    gzthrow("No canonical body set.");

  if (pBody == NULL)
    gzthrow("Parent has no canonical body");

  this->joint->Attach(myBody, pBody);
  this->joint->SetAnchor(0, myBody->GetWorldPose().pos );
  this->joint->SetAxis(0, Vector3(0,1,0) );
  this->joint->SetHighStop(0,Angle(0));
  this->joint->SetLowStop(0,Angle(0));
}

////////////////////////////////////////////////////////////////////////////////
/// Get the canonical body. Used for connected Model heirarchies
Body * Model::GetCanonicalBody() const
{
  return this->canonicalBody;
}

////////////////////////////////////////////////////////////////////////////////
/// Set the gravity mode of the model
void Model::SetGravityMode( const bool &v )
{
  Body *body;
  std::vector<Common* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {

    if (*iter && (*iter)->HasType(BODY))
    {
      body = (Body*)*iter;
      body->SetGravityMode( v );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Set the gravity mode of the model
void Model::SetFrictionMode( const bool &v )
{
  Body *body;
  std::vector<Common* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if ((*iter) && (*iter)->HasType(BODY))
    {
      body = (Body*)*iter;
      body->SetFrictionMode( v );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Set the collide mode of the model
void Model::SetCollideMode( const std::string &m )
{
  Body *body;
  std::vector<Common* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->HasType(BODY))
    {
      body = (Body*)*iter;
      body->SetCollideMode( m );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Set the Fiducial Id of the model
void Model::SetLaserFiducialId( const int &id )
{
  Body *body;
  std::vector<Common* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->HasType(BODY))
    {
      body = (Body*)(*iter);
      body->SetLaserFiducialId( id );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Get the Fiducial Id of the Model
int Model::GetLaserFiducialId( )
{
 //this is not correct if geoms set their own Fiducial. 
 //you can not expect it to be correct in that case anyway ...
  return **this->laserFiducialP; 
}

////////////////////////////////////////////////////////////////////////////////
/// Set the Laser retro property of the model
void Model::SetLaserRetro( const float &retro )
{
  Body *body;

  std::vector<Common* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->HasType(BODY))
    {
      body = (Body*)*iter;
      body->SetLaserRetro( retro );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Load a physical model
void Model::LoadPhysical(XMLConfigNode *node)
{
  XMLConfigNode *childNode = NULL;

  // Load the bodies
  if (node->GetChild("link"))
    childNode = node->GetChild("link");
  else
    childNode = node->GetChild("link");

  while (childNode)
  {
    this->LoadBody(childNode);

    if (childNode->GetNext("link"))
      childNode = childNode->GetNext("link");
    else
      childNode = childNode->GetNext("link");
  }

  // Load the joints
  childNode = node->GetChild("joint");

  while (childNode)
  {
    try
    {
      this->LoadJoint(childNode);
    }
    catch (GazeboError e)
    {
      gzerr(0) << "Error Loading Joint[" << childNode->GetString("name", std::string(), 0) << "]\n";
      gzerr(0) <<  e << std::endl;
      childNode = childNode->GetNext("joint");
      continue;
    }
    childNode = childNode->GetNext("joint");
  }
}


////////////////////////////////////////////////////////////////////////////////
/// Get the list of model interfaces 
/// e.g "pioneer2dx_model1::laser::laser_iface0->laser"
void Model::GetModelInterfaceNames(std::vector<std::string>& list) const
{
  std::vector< Common* >::const_iterator biter;
  std::map<std::string, Controller* >::const_iterator contIter;

  for (contIter=this->controllers.begin();
      contIter!=this->controllers.end(); contIter++)
  {
    contIter->second->GetInterfaceNames(list);	

  }

  for (biter=this->children.begin(); biter != this->children.end(); biter++)
  {
    if (*biter && (*biter)->HasType(BODY))
    {
      const Body *body = (Body*)*biter;
      body->GetInterfaceNames(list);
    }
  }
}


////////////////////////////////////////////////////////////////////////////////
/// Return Model Absolute Pose
/// this is redundant as long as Model's relativePose is maintained
/// which is done in Model::OnPoseChange and during Model initialization
Pose3d Model::GetWorldPose()
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetWorldPose();
  else 
    return Pose3d();
}

////////////////////////////////////////////////////////////////////////////////
/// Add an occurance of a contact to this geom
void Model::StoreContact(const Geom *geom, const Contact &contact)
{
  this->contacts[geom->GetName()].push_back( contact.Clone() );
}

////////////////////////////////////////////////////////////////////////////////
// Get the number of contacts for a geom
unsigned int Model::GetContactCount(const Geom *geom) const
{
  std::map<std::string, std::vector<Contact> >::const_iterator iter;
  iter = this->contacts.find( geom->GetName() );

  if (iter != this->contacts.end())
    return iter->second.size();
  else
    gzerr(0) << "Invalid contact index\n";

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Retreive a contact
Contact Model::RetrieveContact(const Geom *geom, unsigned int i) const
{
  std::map<std::string, std::vector<Contact> >::const_iterator iter;
  iter = this->contacts.find( geom->GetName() );

  if (iter != this->contacts.end() && i < iter->second.size())
    return iter->second[i];
  else
    gzerr(0) << "Invalid contact index\n";

  return Contact();
}
