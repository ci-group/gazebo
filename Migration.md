## Gazebo 2.0 to 3.0

### New Deprecations

1. **gazebo/physics/Joint.hh**
    + ***Deprecation*** virtual void ApplyDamping()
    + ***Replacement*** virtual void ApplyStiffnessDamping()
    ---
    + ***Deprecation*** double GetDampingCoefficient() const
    + ***Replacement*** double GetDamping(int _index)
1. **gazebo/physics/ode/ODEJoint.hh**
    + ***Deprecation*** void CFMDamping()
    + ***Replacement*** void ApplyImplicitStiffnessDamping()

### Modifications
1. **gazebo/transport/ConnectionManager.hh** 
    + ***Removed:*** bool ConnectionManager::Init(const std::string &_masterHost, unsigned int _masterPort) `ABI change`
    + ***Replacement:*** bool ConnectionManager::Init(const std::string &_masterHost, unsigned int _masterPort, uint32_t _timeoutIterations = 30)
    + ***Note:*** No changes to downstream code required. A third parameter has been added that specifies the number of timeout iterations. This parameter has a default value of 30.
1. **gazebo/transport/TransportIface.hh**
    + ***Removed:*** bool init(const std::string &_masterHost = "", unsigned int _masterPort = 0) `ABI change`
    + ***Replacement:*** bool init(const std::string &_masterHost = "", unsigned int _masterPort = 0, uint32_t _timeoutIterations = 30);
    + ***Note:*** No changes to downstream code required. A third parameter has been added that specifies the number of timeout iterations. This parameter has a default value of 30.
1. **gazebo/transport/Publication.hh**
    + ***Removed:*** void Publish(MessagePtr _msg, boost::function<void(uint32_t)> _cb, uint32_t _id) `ABI change`
    + ***Replacement:*** int Publish(MessagePtr _msg, boost::function<void(uint32_t)> _cb, uint32_t _id)
    + ***Note:*** Only the return type changed.

1. **gazebo/common/ModelDatabase.hh** `API change`
    + ***Removed:*** void ModelDatabase::GetModels(boost::function<void (const std::map<std::string, std::string> &)> _func)
    + ***Replacement:*** event::ConnectionPtr ModelDatabase::GetModels(boost::function<void (const std::map<std::string, std::string> &)> _func)
    + ***Note:*** The replacement function requires that the returned connection shared pointer remain valid in order to receive the GetModels callback. Reset the shared pointer to stop receiving GetModels callback.

### Additions

1. **gazebo/physics/Joint.hh**
    + virtual void SetStiffness(int _index, double _stiffness) = 0
    + virtual void SetStiffnessDamping(unsigned int _index, double _stiffness, double _damping, double _reference = 0) = 0

### Deletions

## Gazebo 1.9 to 2.0

### New Deprecations

1. **gazebo/physics/World.hh**
    + ***Deprecation*** void World::StepWorld(int _steps)
    + ***Replacement*** void World::Step(unsigned int _steps)
1. **gazebo/sensors/SensorsIface.hh**
    + ***Deprecation*** std::string sensors::create_sensor(sdf::ElementPtr _elem, const std::string &_worldName,const std::string &_parentName)
    + ***Replacement*** std::string sensors::create_sensor(sdf::ElementPtr _elem, const std::string &_worldName, const std::string &_parentName, uint32_t _parentId)
1. **gazebo/sensors/Sensor.hh**
    + ***Deprecation*** void Sensor::SetParent(const std::string &_name)
    + ***Replacement*** void Sensor::SetParent(const std::string &_name, uint32_t _id)
1. **gazebo/sensors/SensorManager.hh**
    + ***Deprecation*** std::string CreateSensor(sdf::ElementPtr _elem, const std::string &_worldName,  const std::string &_parentName)
    + ***Replacement*** std::string CreateSensor(sdf::ElementPtr _elem, const std::string &_worldName, const std::string &_parentName, uint32_t _parentId)
1. **gazebo/sensors/Collision.hh**
    + ***Deprecation*** void Collision::SetContactsEnabled(bool _enable)
    + ***Replacement*** Use [ContactManager](http://gazebosim.org/api/2.0.0/classgazebo_1_1physics_1_1ContactManager.html).
    ---
    + ***Deprecation*** bool Colliion::GetContactsEnabled() const
    + ***Replacement*** Use [ContactManager](http://gazebosim.org/api/2.0.0/classgazebo_1_1physics_1_1ContactManager.html).
    ---
    + ***Deprecation*** void AddContact(const Contact &_contact)
    + ***Replacement*** Use [ContactManager](http://gazebosim.org/api/2.0.0/classgazebo_1_1physics_1_1ContactManager.html).

### Modifications

1. File rename: `gazebo/common/Common.hh` to `gazebo/common/CommonIface.hh`
1. File rename: `gazebo/physics/Physics.hh` to `gazebo/physics/PhysicsIface.hh`
1. File rename: `gazebo/rendering/Rendering.hh` to `gazebo/rendering/RenderingIface.hh`
1. File rename: `gazebo/sensors/Sensors.hh` to `gazebo/sensors/SensorsIface.hh`
1. File rename: `gazebo/transport/Transport.hh` to `gazebo/transport/TransportIface.hh`
1. File rename: `gazebo/gui/Gui.hh` to `gazebo/gui/GuiIface.hh`
1. File rename: `<model>/manifest.xml` to `<model>/model.config`
1. File rename: `<model_database>/manifest.xml` to `<model_database>/database.config`
1. **gazebo/msgs/physics.proto**
    + ***Removed*** optional double dt
    + ***Replacement*** optional double min_step_size
    ---
    + ***Removed*** optional double update_rate
    + ***Replacement*** optional double real_time_update_rate
1. **gazebo/physics/ModelState.hh**
    + ***Removed*** LinkState ModelState::GetLinkState(int _index) `API change`
    + ***Replacement*** LinkState ModelState::GetLinkState(const std::string &_linkName) const
1. **gazebo/physics/PhyscisEngine.hh**
    + ***Removed*** void PhysicsEngine::SetUpdateRate(double _value) `API change`
    + ***Replacement*** void PhyscisEngine::SetRealTimeUpdateRate(double _rate)
    ---
    + ***Removed*** double PhysicsEngine::GetUpdateRate() `API change`
    + ***Replacement*** double PhysicsEngine::GetRealTimeUpdateRate() const
    ---
    + ***Removed*** void PhysicsEngine::SetStepTime(double _value) `API change`
    + ***Replacement*** void PhysicsEngine::SetMaxStepSize(double _stepSize)
    ---
    + ***Removed*** double PhysicsEngine::GetStepTime() `API change`
    + ***Replacement*** double PhysicsEngine::GetMaxStepSize() const
1. **gazebo/physics/Joint.hh**
    + ***Removed:*** Joint::Load(LinkPtr _parent, LinkPtr _child, const math::Vector3 &_pos) `API chance`
    + ***Replacement:*** Joint::Load(LinkPtr _parent, LinkPtr _child, const math::Pose &_pose)
1. **gazebo/common/Events.hh**
    + ***Removed:*** Events::ConnectWorldUpdateStart(T _subscriber) `API change`
    + ***Replacement*** ConnectionPtr Events::ConnectWorldUpdateBegin(T _subscriber)
    ---
    + ***Removed:*** Events::DisconnectWorldUpdateStart(T _subscriber) `API change`
    + ***Replacement*** ConnectionPtr Events::DiconnectWorldUpdateBegin(T _subscriber)
1. **gazebo/physics/Link.hh**
    + ***Removed*** void Link::RemoveChildJoint(JointPtr _joint) `API change`
    + ***Replacement*** void Link::RemoveChildJoint(const std::string &_jointName)
    ---
    + ***Removed*** void Link::RemoveParentJoint(const std::string &_jointName) `API change`
    + ***Replacement*** void Link::RemoveParentJoint(const std::string &_jointName)
1. **gazebo/physics/MeshShape.hh**
    + ***Removed*** std::string MeshShape::GetFilename() const `API change`
    + ***Replacement*** std::string MeshShape::GetURI() const
    ---
    + ***Removed*** void MeshShape::SetFilename() const `API change`
    + ***Replacement*** std::string MeshShape::SetMesh(const std::string &_uri, const std::string &_submesh = "", bool _center = false) const
1. **gazebo/common/Time.hh**
    + ***Removed*** static Time::NSleep(Time _time) `API change`
    + ***Replacement*** static Time NSleep(unsigned int _ns)
