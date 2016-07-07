/*
 * Copyright (C) 2016 Open Source Robotics Foundation
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <mutex>
#include <string>
#include <vector>
#include <sdf/sdf.hh>
#include <ignition/math/Filter.hh>
#include <gazebo/common/Assert.hh>
#include <gazebo/common/Plugin.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/sensors/sensors.hh>
#include <gazebo/transport/transport.hh>
#include "plugins/ArduCopterPlugin.hh"

using namespace gazebo;

GZ_REGISTER_MODEL_PLUGIN(ArduCopterPlugin)

/// \brief Obtains a parameter from sdf.
/// \param[in] sdf Pointer to the sdf object.
/// \param[in] name Name of the parameter.
/// \param[out] param Param Variable to write the parameter to.
/// \param[in] default_value Default value, if the parameter not available.
/// \param[in] verbose If true, gzerror if the parameter is not available.
template<class T>
bool getSdfParam(sdf::ElementPtr _sdf, const std::string &_name,
  T &_param, const T &_defaultValue, const bool &_verbose = false)
{
  if (_sdf->HasElement(_name))
  {
    _param = _sdf->GetElement(_name)->Get<T>();
    return true;
  }

  _param = _defaultValue;
  if (_verbose)
  {
    gzerr << "[ArduCopterPlugin] Please specify a value for parameter ["
      << _name << "].\n";
  }
  return false;
}

/// \brief A servo packet.
struct ServoPacket
{
  /// \brief Motor speed data.
  float motorSpeed[4];
};

struct fdmPacket
{
  double timestamp;
  double imuAngularVelocityRPY[3];
  double imuLinearAccelerationXYZ[3];
  double imuOrientationQuat[4];
  double velocityXYZ[3];
  double positionXYZ[3];
};

/// \brief Rotor class
class Rotor
{
  /// \brief Constructor
  public: Rotor()
  {
    // most of these coefficients are not used yet.
    this->rotorVelocitySlowdownSim = this->kDefaultRotorVelocitySlowdownSim;
    this->frequencyCutoff = this->kDefaultFrequencyCutoff;
    this->samplingRate = this->kDefaultSamplingRate;

    this->pid.Init(0.1, 0, 0, 0, 0, 1.0, -1.0);
  }

  /// \brief rotor id
  public: int id = 0;

  /// \brief Max rotor propeller RPM.
  public: double maxRpm = 838.0;

  /// \brief Next command to be applied to the propeller
  public: double cmd = 0;

  /// \brief Velocity PID for motor control
  public: common::PID pid;

  /// \brief Control propeller joint.
  public: std::string jointName;

  /// \brief Control propeller joint.
  public: physics::JointPtr joint;

  /// \brief Control propeller link.
  public: std::string linkName;

  /// \brief Control propeller link.
  public: physics::LinkPtr link;

  /// \brief direction multiplier for this rotor
  public: double multiplier = 1;

  /// \brief unused coefficients
  public: double rotorVelocitySlowdownSim;
  public: double frequencyCutoff;
  public: double samplingRate;
  public: ignition::math::OnePole<double> velocityFilter;

  public: static constexpr double kDefaultRotorVelocitySlowdownSim = 10.0;
  public: static constexpr double kDefaultFrequencyCutoff = 5.0;
  public: static constexpr double kDefaultSamplingRate = 0.2;
};

// Private data class
class gazebo::ArduCopterPluginPrivate
{
  /// \brief Bind to an adress and port
  /// \param[in] _address Address to bind to.
  /// \param[in] _port Port to bind to.
  /// \return True on success.
  public: bool Bind(const char *_address, const uint16_t _port)
  {
    struct sockaddr_in sockaddr;
    this->MakeSockAddr(_address, _port, sockaddr);

    if (bind(this->handle, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) != 0)
    {
      return false;
    }
    return true;
  }

  /// \brief Make a socket
  /// \param[in] _address Socket address.
  /// \param[in] _port Socket port
  /// \param[out] _sockaddr New socket address structure.
  public: void MakeSockAddr(const char *_address, const uint16_t _port,
    struct sockaddr_in &_sockaddr)
  {
    memset(&_sockaddr, 0, sizeof(_sockaddr));

    #ifdef HAVE_SOCK_SIN_LEN
      _sockaddr.sin_len = sizeof(_sockaddr);
    #endif

    _sockaddr.sin_port = htons(_port);
    _sockaddr.sin_family = AF_INET;
    _sockaddr.sin_addr.s_addr = inet_addr(_address);
  }

  /// \brief Receive data
  /// \param[out] _buf Buffer that receives the data.
  /// \param[in] _size Size of the buffer.
  /// \param[in] _timeoutMS Milliseconds to wait for data.
  public: ssize_t Recv(void *_buf, const size_t _size, uint32_t _timeoutMs)
  {
    fd_set fds;
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(this->handle, &fds);

    tv.tv_sec = _timeoutMs / 1000;
    tv.tv_usec = (_timeoutMs % 1000) * 1000UL;

    if (select(this->handle+1, &fds, NULL, NULL, &tv) != 1)
    {
        return -1;
    }

    return recv(this->handle, _buf, _size, 0);
  }

  /// \brief Pointer to the update event connection.
  public: event::ConnectionPtr updateConnection;

  /// \brief Pointer to the model;
  public: physics::ModelPtr model;

  /// \brief array of propellers
  public: std::vector<Rotor> rotors;

  /// \brief keep track of controller update sim-time.
  public: gazebo::common::Time lastControllerUpdateTime;

  /// \brief Controller update mutex.
  public: std::mutex mutex;

  /// \brief Socket handle
  public: int handle;

  /// \brief Pointer to the link on which the IMU sensor is attached.
  public: physics::LinkPtr imuLink;

  /// \brief Pointer to an IMU sensor
  public: sensors::ImuSensorPtr imuSensor;
};

////////////////////////////////////////////////////////////////////////////////
ArduCopterPlugin::ArduCopterPlugin()
  : dataPtr(new ArduCopterPluginPrivate)
{
  // socket
  this->dataPtr->handle = socket(AF_INET, SOCK_DGRAM /*SOCK_STREAM*/, 0);
  fcntl(this->dataPtr->handle, F_SETFD, FD_CLOEXEC);
  int one = 1;
  setsockopt(this->dataPtr->handle, IPPROTO_TCP, TCP_NODELAY,
      &one, sizeof(one));

  if (!this->dataPtr->Bind("127.0.0.1", 9002))
  {
    gzerr << "failed to bind with 127.0.0.1:9002, aborting plugin.\n";
    return;
  }

  setsockopt(this->dataPtr->handle, SOL_SOCKET, SO_REUSEADDR,
      &one, sizeof(one));

  fcntl(this->dataPtr->handle, F_SETFL,
      fcntl(this->dataPtr->handle, F_GETFL, 0) | O_NONBLOCK);
}

/////////////////////////////////////////////////
ArduCopterPlugin::~ArduCopterPlugin()
{
}

/////////////////////////////////////////////////
void ArduCopterPlugin::Load(physics::ModelPtr _model, sdf::ElementPtr _sdf)
{
  GZ_ASSERT(_model, "ArduCopterPlugin _model pointer is null");
  GZ_ASSERT(_sdf, "ArduCopterPlugin _sdf pointer is null");

  this->dataPtr->model = _model;

  // per rotor
  if (_sdf->HasElement("rotor"))
  {
    sdf::ElementPtr rotorSDF = _sdf->GetElement("rotor");

    while (rotorSDF)
    {
      Rotor rotor;

      if (rotorSDF->HasAttribute("id"))
      {
        rotor.id = rotorSDF->GetAttribute("id")->Get(rotor.id);
      }
      else
      {
        gzwarn << "id attribute not specified, check motorNumber.\n";
        if (rotorSDF->HasElement("motorNumber"))
        {
          rotor.id = rotorSDF->Get<int>("motorNumber");
        }
        else
        {
          gzwarn << "motorNumber not specified, use order parsed.\n";
          rotor.id = this->dataPtr->rotors.size();
        }
      }

      if (rotorSDF->HasElement("jointName"))
      {
        rotor.jointName = rotorSDF->Get<std::string>("jointName");
      }
      else
      {
        gzerr << "Please specify a jointName,"
          << " where the rotor is attached.\n";
      }

      // Get the pointer to the joint.
      rotor.joint = _model->GetJoint(rotor.jointName);
      if (rotor.joint == nullptr)
      {
        gzerr << "Couldn't find specified joint ["
            << rotor.jointName << "]. This plugin will not run.\n";
        return;
      }

      if (rotorSDF->HasElement("linkName"))
        rotor.linkName = rotorSDF->Get<std::string>("linkName");
      else
      {
        gzerr << "Please specify a linkName for the rotor. Aborting plugin.\n";
        return;
      }

      rotor.link = _model->GetLink(rotor.linkName);
      if (rotor.link == nullptr)
      {
        gzerr << "Couldn't find specified link ["
            << rotor.linkName << "]. This plugin will not run\n";
        return;
      }

      if (rotorSDF->HasElement("turningDirection"))
      {
        std::string turningDirection = rotorSDF->Get<std::string>(
            "turningDirection");
        // special cases mimic from rotors_gazebo_plugins
        if (turningDirection == "cw")
          rotor.multiplier = -1;
        else if (turningDirection == "ccw")
          rotor.multiplier = 1;
        else
        {
          gzdbg << "not string, check turningDirection as float\n";
          rotor.multiplier = rotorSDF->Get<double>("turningDirection");
        }
      }
      else
      {
        rotor.multiplier = 1;
        gzerr << "Please specify a turning"
          << " direction multiplier ('cw' or 'ccw'). Default 'ccw'.\n";
      }

      getSdfParam<double>(rotorSDF, "rotorVelocitySlowdownSim",
          rotor.rotorVelocitySlowdownSim, 1);

      getSdfParam<double>(rotorSDF, "frequencyCutoff",
          rotor.frequencyCutoff, rotor.frequencyCutoff);
      getSdfParam<double>(rotorSDF, "samplingRate",
          rotor.samplingRate, rotor.samplingRate);

      // use gazebo::math::Filter
      rotor.velocityFilter.Fc(rotor.frequencyCutoff, rotor.samplingRate);

      // initialize filter to zero value
      rotor.velocityFilter.Set(0.0);

      // note to use this
      // rotorVelocityFiltered = velocityFilter.Process(rotorVelocityRaw);

      // Overload the PID parameters if they are available.
      double param;
      getSdfParam<double>(rotorSDF, "vel_p_gain", param, rotor.pid.GetPGain());
      rotor.pid.SetPGain(param);

      getSdfParam<double>(rotorSDF, "vel_i_gain", param, rotor.pid.GetIGain());
      rotor.pid.SetIGain(param);

      getSdfParam<double>(rotorSDF, "vel_d_gain", param,  rotor.pid.GetDGain());
      rotor.pid.SetDGain(param);

      getSdfParam<double>(rotorSDF, "vel_i_max", param, rotor.pid.GetIMax());
      rotor.pid.SetIMax(param);

      getSdfParam<double>(rotorSDF, "vel_i_min", param, rotor.pid.GetIMin());
      rotor.pid.SetIMin(param);

      getSdfParam<double>(rotorSDF, "vel_cmd_max", param,
          rotor.pid.GetCmdMax());
      rotor.pid.SetCmdMax(param);

      getSdfParam<double>(rotorSDF, "vel_cmd_min", param,
          rotor.pid.GetCmdMin());
      rotor.pid.SetCmdMin(param);

      // set pid initial command
      rotor.pid.SetCmd(0.0);

      this->dataPtr->rotors.push_back(rotor);
      rotorSDF = rotorSDF->GetNextElement("rotor");
    }
  }

  std::string imuLinkName;
  getSdfParam<std::string>(_sdf, "imuLinkName", imuLinkName, "iris/imu_link");
  this->dataPtr->imuLink = this->dataPtr->model->GetLink(imuLinkName);

  // Get sensors
  this->dataPtr->imuSensor = std::dynamic_pointer_cast<sensors::ImuSensor>
    (sensors::SensorManager::Instance()->GetSensor(
      this->dataPtr->model->GetWorld()->GetName()
      + "::" + this->dataPtr->model->GetScopedName()
      + "::iris::iris/imu_link::imu_sensor"));

  if (!this->dataPtr->imuSensor)
    gzerr << "imu_sensor not found\n" << "\n";

  // Controller time control.
  this->dataPtr->lastControllerUpdateTime = 0;

  // Listen to the update event. This event is broadcast every simulation
  // iteration.
  this->dataPtr->updateConnection = event::Events::ConnectWorldUpdateBegin(
      boost::bind(&ArduCopterPlugin::Update, this, _1));

  gzlog << "ArduCopter ready to fly. The force will be with you" << std::endl;
}

/////////////////////////////////////////////////
void ArduCopterPlugin::Update(const common::UpdateInfo &/*_info*/)
{
  std::lock_guard<std::mutex> lock(this->dataPtr->mutex);

  gazebo::common::Time curTime = this->dataPtr->model->GetWorld()->GetSimTime();

  // Update the control surfaces and publish the new state.
  if (curTime > this->dataPtr->lastControllerUpdateTime)
  {
    this->SendState();
    this->MotorCommand();
    this->UpdatePIDs((curTime -
          this->dataPtr->lastControllerUpdateTime).Double());
  }

  this->dataPtr->lastControllerUpdateTime = curTime;
}

/////////////////////////////////////////////////
void ArduCopterPlugin::UpdatePIDs(const double _dt)
{
  // Position PID for rotors
  for (size_t i = 0; i < this->dataPtr->rotors.size(); ++i)
  {
    double velTarget = this->dataPtr->rotors[i].multiplier *
      this->dataPtr->rotors[i].cmd /
      this->dataPtr->rotors[i].rotorVelocitySlowdownSim;
    double vel = this->dataPtr->rotors[i].joint->GetVelocity(0);
    double error = vel - velTarget;
    double force = this->dataPtr->rotors[i].pid.Update(error, _dt);
    this->dataPtr->rotors[i].joint->SetForce(0, force);
  }
}

/////////////////////////////////////////////////
void ArduCopterPlugin::MotorCommand()
{
  ServoPacket pkt;
  // we re-send the servo packet every 0.1 seconds until we get a
  // reply. This allows us to cope with some packet loss to the FDM
  while (this->dataPtr->Recv(&pkt, sizeof(pkt), 100) != sizeof(pkt))
  {
    // send_servos(input);
    gazebo::common::Time::NSleep(100);
  }

  for (unsigned i = 0; i < this->dataPtr->rotors.size(); ++i)
  {
    // std::cout << i << ": " << pkt.motorSpeed[i] << "\n";
    this->dataPtr->rotors[i].cmd = this->dataPtr->rotors[i].maxRpm *
      pkt.motorSpeed[i];
  }
}

/////////////////////////////////////////////////
void ArduCopterPlugin::SendState()
{
  // send_fdm
  fdmPacket pkt;

  pkt.timestamp = this->dataPtr->model->GetWorld()->GetSimTime().Double();

  // asssumed that the imu orientation is:
  //   x forward
  //   y right
  //   z down

  // get linear acceleration in body frame
  ignition::math::Vector3d linearAccel =
    this->dataPtr->imuSensor->LinearAcceleration();

  // rotate gravity into imu frame, subtract it
  // math::Vector3 gravity =
  //   this->dataPtr->model->GetWorld()->GetPhysicsEngine()->GetGravity();
  // linearAccel = linearAccel -
  //   this->dataPtr->imuLink.GetWorldPose().rot.RotateVectorReverse(gravity);

  // copy to pkt
  pkt.imuLinearAccelerationXYZ[0] = linearAccel.X();
  pkt.imuLinearAccelerationXYZ[1] = linearAccel.Y();
  pkt.imuLinearAccelerationXYZ[2] = linearAccel.Z();
  // gzerr << "lin accel [" << linearAccel << "]\n";

  // get angular velocity in body frame
  ignition::math::Vector3d angularVel =
    this->dataPtr->imuSensor->AngularVelocity();

  // copy to pkt
  pkt.imuAngularVelocityRPY[0] = angularVel.X();
  pkt.imuAngularVelocityRPY[1] = angularVel.Y();
  pkt.imuAngularVelocityRPY[2] = angularVel.Z();

  // get orientation with offset added
  // ignition::math::Quaterniond worldQ =
  // this->dataPtr->imuSensor->Orientation();
  // pkt.imuOrientationQuat[0] = worldQ.W();
  // pkt.imuOrientationQuat[1] = worldQ.X();
  // pkt.imuOrientationQuat[2] = worldQ.Y();
  // pkt.imuOrientationQuat[3] = worldQ.Z();

  // get inertial pose and velocity
  // position of the quadrotor in world frame
  // this position is used to calcualte bearing and distance
  // from starting location, then use that to update gps position.
  // The algorithm looks something like below (from ardupilot helper
  // libraries):
  //   bearing = to_degrees(atan2(position.y, position.x));
  //   distance = math.sqrt(self.position.x**2 + self.position.y**2)
  //   (self.latitude, self.longitude) = util.gps_newpos(
  //    self.home_latitude, self.home_longitude, bearing, distance)
  // where xyz is in the NED directions.
  // Gazebo world xyz is assumed to be N, -E, -D, so flip some stuff
  // around.
  // orientation of the quadrotor in world NED frame -
  // assuming the world NED frame has xyz mapped to NED,
  // imuLink is NED - z down

  // gazeboToNED brings us from gazebo model: x-forward, y-right, z-down
  // to the aerospace convention: x-forward, y-left, z-up
  ignition::math::Pose3d gazeboToNED(0, 0, 0, IGN_PI, 0 ,0);

  // model world pose brings us to model, x-forward, y-left, z-up
  // adding gazeboToNED gets us to the x-forward, y-right, z-down
  ignition::math::Pose3d worldToModel = gazeboToNED +
    this->dataPtr->model->GetWorldPose().Ign();

  // get transform from world NED to Model frame
  ignition::math::Pose3d NEDToModel = worldToModel - gazeboToNED;

  // gzerr << "ned to model [" << NEDToModel << "]\n";

  // N
  pkt.positionXYZ[0] = NEDToModel.Pos().X();

  // E
  pkt.positionXYZ[1] = NEDToModel.Pos().Y();

  // D
  pkt.positionXYZ[2] = NEDToModel.Pos().Z();

  // This is the rotation from world NED frame to the quadrotor
  // frame.
  // gzerr << "imu [" << worldToModel.rot.GetAsEuler() << "]\n";
  // gzerr << "ned [" << gazeboToNED.rot.GetAsEuler() << "]\n";
  // gzerr << "rot [" << NEDToModel.rot.GetAsEuler() << "]\n";
  pkt.imuOrientationQuat[0] = NEDToModel.Rot().W();
  pkt.imuOrientationQuat[1] = NEDToModel.Rot().X();
  pkt.imuOrientationQuat[2] = NEDToModel.Rot().Y();
  pkt.imuOrientationQuat[3] = NEDToModel.Rot().Z();

  // Get NED velocity in body frame *
  // or...
  // Get model velocity in NED frame
  ignition::math::Vector3d velGazeboWorldFrame =
    this->dataPtr->model->GetLink()->GetWorldLinearVel().Ign();
  ignition::math::Vector3d velNEDFrame =
    gazeboToNED.Rot().RotateVectorReverse(velGazeboWorldFrame);
  pkt.velocityXYZ[0] = velNEDFrame.X();
  pkt.velocityXYZ[1] = velNEDFrame.Y();
  pkt.velocityXYZ[2] = velNEDFrame.Z();

  struct sockaddr_in sockaddr;
  this->dataPtr->MakeSockAddr("127.0.0.1", 9003, sockaddr);

  ::sendto(this->dataPtr->handle, &pkt, sizeof(pkt), 0,
    (struct sockaddr *)&sockaddr, sizeof(sockaddr));
}
