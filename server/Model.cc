/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* Desc: Base class for all models.
 * Author: Nathan Koenig and Andrew Howard
 * Date: 8 May 2003
 * SVN: $Id$
 */

//#include <boost/python.hpp>

#include <sstream>
#include <iostream>
#include <float.h>

#include "OgreVisual.hh"
#include "Light.hh"
#include "GraphicsIfaceHandler.hh"
#include "Global.hh"
#include "GazeboError.hh"
#include "GazeboMessage.hh"
#include "XMLConfig.hh"
#include "World.hh"
#include "Simulator.hh"
#include "OgreCreator.hh"
#include "Body.hh"
#include "HingeJoint.hh"
#include "PhysicsEngine.hh"
#include "Controller.hh"
#include "ControllerFactory.hh"
#include "IfaceFactory.hh"
#include "Model.hh"

using namespace gazebo;

uint Model::lightNumber = 0;

////////////////////////////////////////////////////////////////////////////////
// Constructor
Model::Model(Model *parent)
    : Entity(parent)
{
  this->type = MODEL;
  this->modelType = "";
  this->joint = NULL;

  Param::Begin(&this->parameters);
  this->canonicalBodyNameP = new ParamT<std::string>("canonicalBody",
                                                   std::string(),0);

  this->xyzP = new ParamT<Vector3>("xyz", Vector3(0,0,0), 0);
  this->xyzP->Callback(&Entity::SetRelativePosition, (Entity*)this);

  this->rpyP = new ParamT<Quatern>("rpy", Quatern(1,0,0,0), 0);
  this->rpyP->Callback( &Entity::SetRelativeRotation, (Entity*)this);

  this->enableGravityP = new ParamT<bool>("enableGravity", true, 0);
  this->enableGravityP->Callback( &Model::SetGravityMode, this );

  this->enableFrictionP = new ParamT<bool>("enableFriction", true, 0);
  this->enableFrictionP->Callback( &Model::SetFrictionMode, this );

  this->collideP = new ParamT<std::string>("collide", "all", 0);
  this->collideP->Callback( &Model::SetCollideMode, this );

  this->laserFiducialP = new ParamT<int>("laserFiducialId", -1, 0);
  this->laserFiducialP->Callback( &Model::SetLaserFiducialId, this );

  this->laserRetroP = new ParamT<float>("laserRetro", -1, 0);
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
  //std::map< std::string, Body* >::iterator biter;
  std::vector<Entity*>::iterator biter;

  JointContainer::iterator jiter;
  std::map< std::string, Controller* >::iterator citer;

  if (this->light)
  {
    OgreCreator::Instance()->DeleteLight(this->light);
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
  XMLConfigNode *childNode;
  std::string scopedName;
  Pose3d pose;
  Entity* dup;

  this->nameP->Load(node);

  scopedName = this->GetScopedName();

  dup = World::Instance()->GetEntityByName(scopedName);

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
      World::Instance()->DeleteEntity(scopedName.c_str());
    }
  }

  this->staticP->Load(node);

  this->canonicalBodyNameP->Load(node);
  this->xyzP->Load(node);
  this->rpyP->Load(node);
  this->enableGravityP->Load(node);
  this->enableFrictionP->Load(node);
  this->collideP->Load(node);
  this->laserFiducialP->Load(node);
  this->laserRetroP->Load(node);

  this->xmlNode = node;
  this->modelType=node->GetName();

  this->SetStatic( **(this->staticP) );

  // Get the position and orientation of the model (relative to parent)
  pose.Reset();
  pose.pos = **this->xyzP;
  pose.rot = **this->rpyP;

  if (this->IsStatic())
    this->SetRelativePose( pose );

  if (this->modelType == "physical")
    this->LoadPhysical(node);
  else if (this->modelType == "renderable")
    this->LoadRenderable(node);
  else if (this->modelType != "empty")
    gzthrow("Invalid model type[" + this->modelType + "]\n");

  // Set the relative pose of the model
  if (!this->IsStatic())
    this->SetRelativePose( pose );

  // Record the model's initial pose (for reseting)
  this->SetInitPose(pose);

  // Load controllers
  childNode = node->GetChildByNSPrefix("controller");
  while (childNode)
  {
    this->LoadController(childNode);
    childNode = childNode->GetNextByNSPrefix("controller");
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
    Entity *entity = this->children.front();
    if (entity && entity->GetType() == Entity::BODY)
      this->canonicalBodyNameP->SetValue( entity->GetName() );
  }

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
  this->graphicsHandler = new GraphicsIfaceHandler();
  this->graphicsHandler->Load(this->GetScopedName(), this);

  // Get the name of the python module
  /*this->pName.reset(PyString_FromString(node->GetString("python","",0).c_str()));
  //this->pName.reset(PyString_FromString("pioneer2dx"));

  // Import the python module
  if (this->pName)
  {
  this->pModule.reset(PyImport_Import(this->pName));
  Py_DECREF(this->pName);
  }

  // Get the Update function from the module
  if (this->pModule)
  {
  this->pFuncUpdate.reset(PyObject_GetAttrString(this->pModule, "Update"));
  if (this->pFuncUpdate && !PyCallable_Check(this->pFuncUpdate))
  this->pFuncUpdate = NULL;
  }
   */
}

////////////////////////////////////////////////////////////////////////////////
// Save the model in XML format
void Model::Save(std::string &prefix, std::ostream &stream)
{
  std::string p = prefix + "  ";
  std::string typeName;
  //std::map<std::string, Body* >::iterator bodyIter;
  std::vector<Entity* >::iterator bodyIter;
  std::map<std::string, Controller* >::iterator contIter;
  JointContainer::iterator jointIter;

  this->xyzP->SetValue( this->GetRelativePose().pos );
  this->rpyP->SetValue( this->GetRelativePose().rot );

  if (this->modelType=="renderable")
    typeName = "renderable";
  else if (this->modelType=="physical")
    typeName = "physical";

  stream << prefix << "<model:" << typeName;
  stream << " name=\"" << this->nameP->GetValue() << "\">\n"; 
  stream << prefix << "  " << *(this->xyzP) << "\n";
  stream << prefix << "  " << *(this->rpyP) << "\n";
  stream << prefix << "  " << *(this->enableGravityP) << "\n";
  stream << prefix << "  " << *(this->enableFrictionP) << "\n";
  stream << prefix << "  " << *(this->collideP) << "\n";

  if (this->modelType == "physical")
  {
    stream << prefix << "  " << *(this->staticP) << "\n";

    for (bodyIter=this->children.begin(); bodyIter!=this->children.end(); bodyIter++)
    {
      stream << "\n";
      Entity *entity = *bodyIter;
      if (entity && entity->GetType() == Entity::BODY)
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
  }
  else
  {
    if (this->light)
      this->light->Save(p, stream);
  }

  if (this->parentBodyNameP && this->myBodyNameP)
  {
    stream << prefix << "  <attach>\n";
    stream << prefix << "    " << *(this->parentBodyNameP) << "\n";
    stream << prefix << "    " << *(this->myBodyNameP) << "\n";
    stream << prefix << "  </attach>\n";
  }

  // Save all child models
  std::vector< Entity* >::iterator eiter;
  for (eiter = this->children.begin(); eiter != this->children.end(); eiter++)
  {
    if (*eiter && (*eiter)->GetType() == Entity::MODEL)
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
  std::vector<Entity* >::iterator biter;
  std::map<std::string, Controller* >::iterator contIter;

  this->graphicsHandler->Init();

  for (biter = this->children.begin(); biter!=this->children.end(); biter++)
  {
    if (*biter)
    {
      if ((*biter)->GetType() == Entity::BODY)
        ((Body*)*biter)->Init();
      else if ((*biter)->GetType() == Entity::MODEL)
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
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    cb->SetCanonicalModel(this);
}

////////////////////////////////////////////////////////////////////////////////
// Update the model
void Model::Update()
{
  if (this->controllers.size() == 0 && this->IsStatic())
    return;

  //DiagnosticTimer timer("Model[" + this->GetName() + "] Update ");

  std::vector<Entity*>::iterator bodyIter;
  std::map<std::string, Controller* >::iterator contIter;
  JointContainer::iterator jointIter;

  this->updateSignal();

  {
    //DiagnosticTimer timer("Model[" + this->GetName() + "] Bodies Update ");

    for (bodyIter=this->children.begin(); 
         bodyIter!=this->children.end(); bodyIter++)
    {
      if (*bodyIter && (*bodyIter)->GetType() == Entity::BODY)
      {
        Body *body = (Body*)(*bodyIter);
#ifdef USE_THREADPOOL
        World::Instance()->threadPool->schedule(
            boost::bind(&Body::Update,body));
#else
        body->Update();
#endif
      }
    }
  }

  {
    //DiagnosticTimer timer("Model[" + this->GetName() + "] Controllers Update ");
    for (contIter=this->controllers.begin();
        contIter!=this->controllers.end(); contIter++)
    {
      if (contIter->second)
      {
#ifdef USE_THREADPOOL
        World::Instance()->threadPool->schedule(boost::bind(&Controller::Update,(contIter->second)));
#else
        contIter->second->Update();
#endif
      }
    }
  }


  if (World::Instance()->GetShowJoints())
  {
    //DiagnosticTimer timer("Model[" + this->GetName() + "] Joints Update ");
    for (jointIter = this->joints.begin(); 
         jointIter != this->joints.end(); jointIter++)
    {
#ifdef USE_THREADPOOL
      World::Instance()->threadPool->schedule(
          boost::bind(&Joint::Update,*jointIter));
#else
      (*jointIter)->Update();
#endif
    }
  }

  {
    //DiagnosticTimer timer("Model[" + this->GetName() + "] Children Update ");
    this->UpdateChild();
  }
}

void Model::OnPoseChange()
{
  /// set the pose of the model, which is the pose of the canonical body
  /// this updates the relativePose of the Model Entity
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    this->SetAbsPose(cb->GetAbsPose(),false);

}

////////////////////////////////////////////////////////////////////////////////
// Remove a child
void Model::RemoveChild(Entity *child)
{
  JointContainer::iterator jiter;

  if (child->GetType() == Entity::BODY)
  {
    bool done = false;

    while (!done)
    {
      done = true;

      for (jiter = this->joints.begin(); jiter != this->joints.end(); jiter++)
      {
        if (!(*jiter))
          continue;

        if ((*jiter)->GetJointBody(0)->GetName() == child->GetName() ||
            (*jiter)->GetJointBody(1)->GetName() == child->GetName() ||
            (*jiter)->GetJointBody(0)->GetName() == (*jiter)->GetJointBody(1)->GetName())
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

  std::vector<Entity*>::iterator iter;
  for (iter =this->children.begin(); iter != this->children.end(); iter++)
    if (*iter && (*iter)->GetType() == Entity::BODY)
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
  std::vector<Entity* >::iterator biter;
  std::map<std::string, Controller* >::iterator contIter;

  for (contIter = this->controllers.begin();
       contIter != this->controllers.end(); contIter++)
  {
    contIter->second->Fini();
  }

  for (biter=this->children.begin(); biter != this->children.end(); biter++)
  {
    if (*biter && (*biter)->GetType() == Entity::BODY)
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

  std::vector< Entity* >::iterator iter;
  for (iter = this->children.begin(); iter != this->children.end(); iter++)
  {
    if (*iter && (*iter)->GetType() == Entity::MODEL)
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
  std::vector< Entity* >::iterator biter;
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
    if (*biter && (*biter)->GetType() == Entity::BODY)
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
// Get the name of the model
const std::string &Model::GetModelType() const
{
  return this->modelType;
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
  std::vector<Entity* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->GetType() == Entity::BODY)
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
  std::vector<Entity* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->GetType() == Entity::BODY)
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
  std::vector<Entity* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->GetType() == Entity::BODY)
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
  std::vector<Entity* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->GetType() == Entity::BODY)
    {
      body = (Body*)*iter;
      body->SetEnabled(true);
      body->SetAngularAccel( accel );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Get the linear velocity of the model
Vector3 Model::GetLinearVel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetLinearVel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the angular velocity of the model
Vector3 Model::GetAngularVel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetAngularVel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the linear acceleration of the model
Vector3 Model::GetLinearAccel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetLinearAccel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the angular acceleration of the model
Vector3 Model::GetAngularAccel() const
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetAngularAccel();
  else // return 0 vector if model has no body
    return Vector3(0,0,0);
}

////////////////////////////////////////////////////////////////////////////////
/// Get the size of the bounding box
void Model::GetBoundingBox(Vector3 &min, Vector3 &max) const
{
  Vector3 bbmin, bbmax;
  std::vector<Entity* >::const_iterator iter;

  min.Set(FLT_MAX, FLT_MAX, FLT_MAX);
  max.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX);

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->GetType() == Entity::BODY)
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
  // Create a new body
  return World::Instance()->GetPhysicsEngine()->CreateBody(this);
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
  Joint::Type jtype;

  // Create a Hinge Joint
  if (node->GetName() == "hinge")
    jtype = Joint::HINGE;
  else if (node->GetName() == "ball")
    jtype = Joint::BALL;
  else if (node->GetName() == "slider")
    jtype = Joint::SLIDER;
  else if (node->GetName() == "hinge2")
    jtype = Joint::HINGE2;
  else if (node->GetName() == "universal")
    jtype = Joint::UNIVERSAL;
  else
  {
    gzthrow("Uknown joint[" + node->GetName() + "]\n");
  }

  joint = World::Instance()->GetPhysicsEngine()->CreateJoint(jtype);

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
  std::vector< Entity* >::const_iterator biter;

  for (biter=this->children.begin(); biter != this->children.end(); biter++)
  {
    if ( *biter && (*biter)->GetType() == Entity::BODY)
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
  std::vector< Entity* >::const_iterator biter;

  for (biter=this->children.begin(); biter != this->children.end(); biter++)
  {
    if (*biter && (*biter)->GetType() == Entity::BODY)
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
  std::vector< Entity* >::const_iterator biter;

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

  if (this->parent->GetType() == Entity::MODEL)
    parentModel = (Model*)this->parent;

  if (parentModel == NULL)
    gzthrow("Parent cannot be NULL when attaching two models");

  this->joint =World::Instance()->GetPhysicsEngine()->CreateJoint(Joint::HINGE);

  Body *myBody = this->GetBody(**(this->myBodyNameP));
  Body *pBody = parentModel->GetBody(**(this->parentBodyNameP));

  if (myBody == NULL)
    gzthrow("No canonical body set.");

  if (pBody == NULL)
    gzthrow("Parent has no canonical body");

  this->joint->Attach(myBody, pBody);
  this->joint->SetAnchor(0, myBody->GetAbsPose().pos );
  this->joint->SetAxis(0, Vector3(0,1,0) );
  this->joint->SetHighStop(0,Angle(0));
  this->joint->SetLowStop(0,Angle(0));
}

////////////////////////////////////////////////////////////////////////////////
/// Get the canonical body. Used for connected Model heirarchies
Body * Model::GetCanonicalBody() const
{
  if (!this->children.empty())
  {
    std::vector<Entity*>::const_iterator iter;
    Body *body = NULL;
    for (iter = this->children.begin(); iter != this->children.end(); iter++)
      if ((*iter)->GetName() == **this->canonicalBodyNameP)
      {
        body = (Body*)(*iter);
        break;
      }

    return body;
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
/// Set the gravity mode of the model
void Model::SetGravityMode( const bool &v )
{
  Body *body;
  std::vector<Entity* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {

    if (*iter && (*iter)->GetType() == Entity::BODY)
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
  std::vector<Entity* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if ((*iter) && (*iter)->GetType() == Entity::BODY)
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
  std::vector<Entity* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->GetType() == Entity::BODY)
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
  std::vector<Entity* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->GetType() == Entity::BODY)
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

  std::vector<Entity* >::iterator iter;

  for (iter=this->children.begin(); iter!=this->children.end(); iter++)
  {
    if (*iter && (*iter)->GetType() == Entity::BODY)
    {
      body = (Body*)*iter;
      body->SetLaserRetro( retro );
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Load a renderable model (like a light source).
void Model::LoadRenderable(XMLConfigNode *node)
{
  XMLConfigNode *childNode = NULL;

  // We still need a canonical body so that this model can be attached to
  // others
  Body *body = this->CreateBody();
  char lightNumBuf[8];
  sprintf(lightNumBuf, "%d", lightNumber++);
  body->SetName(this->GetName() + "_RenderableBody_" + lightNumBuf);

  if (Simulator::Instance()->GetRenderEngineEnabled() && 
      (childNode = node->GetChild("light")))
  {
    this->light = OgreCreator::Instance()->CreateLight(body);
    this->light->Load(childNode);
  }

}

////////////////////////////////////////////////////////////////////////////////
// Load a physical model
void Model::LoadPhysical(XMLConfigNode *node)
{
  XMLConfigNode *childNode = NULL;

  // Load the bodies
  if (node->GetChildByNSPrefix("body"))
    childNode = node->GetChildByNSPrefix("body");
  else
    childNode = node->GetChild("body");

  while (childNode)
  {
    this->LoadBody(childNode);

    if (childNode->GetNextByNSPrefix("body"))
      childNode = childNode->GetNextByNSPrefix("body");
    else
      childNode = childNode->GetNext("body");
  }

  // Load the joints
  childNode = node->GetChildByNSPrefix("joint");

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
      childNode = childNode->GetNextByNSPrefix("joint");
      continue;
    }
    childNode = childNode->GetNextByNSPrefix("joint");
  }
}


////////////////////////////////////////////////////////////////////////////////
/// Get the list of model interfaces 
/// e.g "pioneer2dx_model1::laser::laser_iface0->laser"
void Model::GetModelInterfaceNames(std::vector<std::string>& list) const
{
  std::vector< Entity* >::const_iterator biter;
  std::map<std::string, Controller* >::const_iterator contIter;

  for (contIter=this->controllers.begin();
      contIter!=this->controllers.end(); contIter++)
  {
    contIter->second->GetInterfaceNames(list);	

  }

  for (biter=this->children.begin(); biter != this->children.end(); biter++)
  {
    if (*biter && (*biter)->GetType() == Entity::BODY)
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
Pose3d Model::GetAbsPose()
{
  Body* cb = this->GetCanonicalBody();
  if (cb != NULL)
    return cb->GetAbsPose();
  else 
    return Pose3d();
}
