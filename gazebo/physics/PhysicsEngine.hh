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
/* Desc: The base class for all physics engines
 * Author: Nate Koenig
 */

#ifndef _PHYSICSENGINE_HH_
#define _PHYSICSENGINE_HH_

#include <boost/thread/recursive_mutex.hpp>
#include <string>

#include "gazebo/transport/TransportTypes.hh"
#include "gazebo/msgs/msgs.hh"

#include "gazebo/physics/PhysicsTypes.hh"

namespace gazebo
{
  namespace physics
  {
    class ContactManager;

    /// \addtogroup gazebo_physics
    /// \{

    /// \class PhysicsEngine PhysicsEngine.hh physics/physics.hh
    /// \brief Base class for a physics engine.
    class PhysicsEngine
    {
      /// \brief Default constructor.
      /// \param[in] _world Pointer to the world.
      public: explicit PhysicsEngine(WorldPtr _world);

      /// \brief Destructor.
      public: virtual ~PhysicsEngine();

      /// \brief Load the physics engine.
      /// \param[in] _sdf Pointer to the SDF parameters.
      public: virtual void Load(sdf::ElementPtr _sdf);

      /// \brief Initialize the physics engine.
      public: virtual void Init() = 0;

      /// \brief Finilize the physics engine.
      public: virtual void Fini();

      /// \brief Rest the physics engine.
      public: virtual void Reset() {}

      /// \brief Init the engine for threads.
      public: virtual void InitForThread() = 0;

      /// \brief Update the physics engine collision.
      public: virtual void UpdateCollision() = 0;

      /// \brief Set the random number seed for the physics engine.
      /// \param[in] _seed The random number seed.
      public: virtual void SetSeed(uint32_t _seed);

      /// \brief Set the simulation update rate.
      /// \param[in] _value Value of the update rate.
      public: void SetUpdateRate(double _value);

      /// \brief Get the simulation update rate.
      /// \return Update rate.
      public: double GetUpdateRate();

      /// \brief Get the simulation update period.
      /// \return Simulation update period.
      public: double GetUpdatePeriod();

      /// \brief Set the simulation step time.
      /// \param[in] _value Value of the step time.
      public: virtual void SetStepTime(double _value) = 0;

      /// \brief Get the simulation step time.
      /// \return Simulation step time.
      public: virtual double GetStepTime() = 0;

      /// \brief Update the physics engine.
      public: virtual void UpdatePhysics() {}

      /// \brief Create a new body.
      /// \param[in] _parent Parent model for the link.
      public: virtual LinkPtr CreateLink(ModelPtr _parent) = 0;

      /// \brief Create a collision.
      /// \param[in] _shapeType Type of collision to create.
      /// \param[in] _link Parent link.
      public: virtual CollisionPtr CreateCollision(
                  const std::string &_shapeType, LinkPtr _link) = 0;

      /// \brief Create a collision.
      /// \param[in] _shapeType Type of collision to create.
      /// \param[in] _linkName Name of the parent link.
      public: CollisionPtr CreateCollision(const std::string &_shapeType,
                                           const std::string &_linkName);

      /// \brief Create a physics::Shape object.
      /// \param[in] _shapeType Type of shape to create.
      /// \param[in] _collision Collision parent.
      public: virtual ShapePtr CreateShape(const std::string &_shapeType,
                                           CollisionPtr _collision) = 0;

      /// \brief Create a new joint.
      /// \param[in] _type Type of joint to create.
      /// \param[in] _parent Model parent.
      public: virtual JointPtr CreateJoint(const std::string &_type,
                                           ModelPtr _parent) = 0;

      /// \brief Return the gavity vector.
      /// \return The gavity vector.
      public: virtual math::Vector3 GetGravity() const;

      /// \brief Set the gavity vector.
      /// \param[in] _gravity New gravity vector.
      public: virtual void SetGravity(
                  const gazebo::math::Vector3 &_gravity) = 0;

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief Access functions to set ODE parameters.
      /// \param[in] _cfm Constraint force mixing.
      public: virtual void SetWorldCFM(double _cfm);

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief Access functions to set ODE parameters.
      /// \param[in] _erp Error reduction parameter.
      public: virtual void SetWorldERP(double _erp);

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief Access functions to set ODE parameters.
      /// \param[in] _autoDisable True to enable auto disabling of bodies.
      public: virtual void SetAutoDisableFlag(bool _autoDisable);

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief Access functions to set ODE parameters.
      /// \param[in] _iter Number of iterations.
      public: virtual void SetSORPGSPreconIters(unsigned int _iters);

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief Access functions to set ODE parameters.
      /// \param[in] _iter Number of iterations.
      public: virtual void SetSORPGSIters(unsigned int _iters);

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief Access functions to set ODE parameters.
      /// \param[in] _w SORPGSW value.
      public: virtual void SetSORPGSW(double _w);

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief Access functions to set ODE parameters.
      /// \param[in] _vel Max correcting velocity.
      public: virtual void SetContactMaxCorrectingVel(double _vel);

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief Access functions to set ODE parameters.
      /// \param[in] _layerDepth Surface layer depth
      public: virtual void SetContactSurfaceLayer(double _layerDepth);

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief access functions to set ODE parameters
      /// \param[in] _maxContacts Maximum number of contacts.
      public: virtual void SetMaxContacts(double _maxContacts);

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief Get World CFM.
      /// \return World CFM.
      public: virtual double GetWorldCFM() {return 0;}

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief Get World ERP.
      /// \return World ERP.
      public: virtual double GetWorldERP() {return 0;}

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief access functions to set ODE parameters..
      /// \return Auto disable flag.
      public: virtual bool GetAutoDisableFlag() {return 0;}

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief access functions to set ODE parameters.
      /// \return SORPGS precondition iterations.
      public: virtual int GetSORPGSPreconIters() {return 0;}

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map
      /// \brief access functions to set ODE parameters.
      /// \return SORPGS iterations.
      public: virtual int GetSORPGSIters() {return 0;}

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map.
      /// \brief access functions to set ODE parameters
      /// \return SORPGSW value.
      public: virtual double GetSORPGSW() {return 0;}

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map.
      /// \brief access functions to set ODE parameters.
      /// \return Max correcting velocity.
      public: virtual double GetContactMaxCorrectingVel() {return 0;}

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map.
      /// \brief access functions to set ODE parameters.
      /// \return Contact suerface layer depth.
      public: virtual double GetContactSurfaceLayer() {return 0;}

      /// \TODO: Remove this function, and replace it with a more generic
      /// property map.
      /// \brief access functions to set ODE parameters.
      /// \return Maximum number of allows contacts.
      public: virtual int GetMaxContacts() {return 0;}

      /// \brief Debug print out of the physic engine state.
      public: virtual void DebugPrint() const = 0;

      /// \brief Get a pointer to the contact manger.
      /// \return Pointer to the contact manager.
      public: ContactManager *GetContactManager() const;

      /// \brief returns a pointer to the PhysicsEngine#physicsUpdateMutex.
      /// \return Pointer to the physics mutex.
      public: boost::recursive_mutex *GetPhysicsUpdateMutex() const
              {return this->physicsUpdateMutex;}

      /// \brief virtual callback for gztopic "~/request".
      /// \param[in] _msg Request message.
      protected: virtual void OnRequest(ConstRequestPtr &_msg);

      /// \brief virtual callback for gztopic "~/physics".
      /// \param[in] _msg Physics message.
      protected: virtual void OnPhysicsMsg(ConstPhysicsPtr &_msg);

      /// \brief Pointer to the world.
      protected: WorldPtr world;

      /// \brief Our SDF values.
      protected: sdf::ElementPtr sdf;

      /// \brief Node for communication.
      protected: transport::NodePtr node;

      /// \brief Response publisher.
      protected: transport::PublisherPtr responsePub;

      /// \brief Subscribe to the physics topic.
      protected: transport::SubscriberPtr physicsSub;

      /// \brief Subscribe to the request topic.
      protected: transport::SubscriberPtr requestSub;

      /// \brief Mutex to protect the update cycle.
      protected: boost::recursive_mutex *physicsUpdateMutex;

      /// \brief Class that handles all contacts generated by the physics
      /// engine.
      protected: ContactManager *contactManager;

      /// \brief Store the value of the updateRate parameter in double form.
      /// To improve efficiency.
      private: double updateRateDouble;
    };
    /// \}
  }
}
#endif
