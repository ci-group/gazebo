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

#include "physics/physics.h"
#include "transport/transport.h"
#include "plugins/VehiclePlugin.hh"

using namespace gazebo;
GZ_REGISTER_MODEL_PLUGIN(VehiclePlugin)

/////////////////////////////////////////////////
VehiclePlugin::VehiclePlugin()
{
  this->wheels.resize(4);
}

/////////////////////////////////////////////////
void VehiclePlugin::Load(physics::ModelPtr _model, sdf::ElementPtr _sdf)
{
  this->model = _model;
  this->physics = this->model->GetWorld()->GetPhysicsEngine();

  this->chassis = this->model->GetLink(_sdf->GetElement("chassis"));
  this->wheels[0] = this->model->GetLink(_sdf->GetElement("front_left"));
  this->wheels[1] = this->model->GetLink(_sdf->GetElement("front_right"));
  this->wheels[2] = this->model->GetLink(_sdf->GetElement("back_left"));
  this->wheels[3] = this->model->GetLink(_sdf->GetElement("back_right"));

  if (!this->chassis)
  {
    gzerr << "Unable to find chassis[" << _sdf->GetElement("chassis") << "]\n";
    return;
  }

  if (!this->wheels[0])
  {
    gzerr << "Unable to find front_left wheel["
          << _sdf->GetElement("front_left") << "]\n";
    return;
  }

  if (!this->wheels[1])
  {
    gzerr << "Unable to find front_right wheel["
          << _sdf->GetElement("front_right") << "]\n";
    return;
  }

  if (!this->wheels[2])
  {
    gzerr << "Unable to find back_left wheel["
          << _sdf->GetElement("back_left") << "]\n";
    return;
  }

  if (!this->wheels[3])
  {
    gzerr << "Unable to find back_right wheel["
          << _sdf->GetElement("back_right") << "]\n";
    return;
  }

  std::cout << "Chassis[" << this->chassis->GetScopedName() << "]\n";
  for (int i = 0; i < 4; ++i)
    std::cout << "Wheel[" << this->wheels[i]->GetScopedName() << "]\n";

  this->connections.push_back(event::Events::ConnectWorldUpdateStart(
          boost::bind(&GripperPlugin::OnUpdate, this)));
}

/////////////////////////////////////////////////
void VehiclePlugin::OnUpdate()
{
  double AERO_LOAD = -0.1;

  this->velocity = this->chassis->GetWorldLinearVel();

  //  aerodynamics
  this->chassis->AddForce(math::Vector3(0, 0,
        AERO_LOAD * this->velocity.GetSquaredLength()));

  // Sway bars
  math::Vector3 bodyPoint;
  math::Vector3 hingePoint;
  math::Vector3 axis;

  double displacement;

  for (int ix = 0; ix < 4; ++ix)
  {
    hingePoint = this->hinges[ix]->GetAnchor(0);
    // dJointGetHinge2Anchor(hinges_[ix], &hingePoint.x);

    bodyPoint = this->hinges[ix]->GetAnchor(1);
    // dJointGetHinge2Anchor2(hinges_[ix], &bodyPoint.x);

    axis = this->hinges[ix]->GetGlobalAxis(0);
    //dJointGetHinge2Axis1(hinges_[ix], &axis.x);

    displacement = (hingePoint - bodyPoint) / axis;
    float amt = displacement * SWAY_FORCE;
    if( displacement > 0 ) {
      if( amt > 15 ) {
        amt = 15;
      }
      dBodyAddForce( wheelBody_[ix], -axis.x * amt, -axis.y * amt, -axis.z * amt );
      dReal const * wp = dBodyGetPosition( wheelBody_[ix] );
      dBodyAddForceAtPos( chassisBody_, axis.x*amt, axis.y*amt, axis.z*amt, wp[0], wp[1], wp[2] );
      dBodyAddForce( wheelBody_[ix^1], axis.x * amt, axis.y * amt, axis.z * amt );
      wp = dBodyGetPosition( wheelBody_[ix] );
      dBodyAddForceAtPos( chassisBody_, -axis.x*amt, -axis.y*amt, -axis.z*amt, wp[0], wp[1], wp[2] );
    }
  }
}
