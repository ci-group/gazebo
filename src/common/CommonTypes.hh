#ifndef GAZEBO_COMMON_TYPES_HH
#define GAZEBO_COMMON_TYPES_HH

#include <vector>
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

/////////////////////////////////////////////////////////////////////////////
// Defines
/////////////////////////////////////////////////////////////////////////////
#ifndef NULL
#define NULL 0
#endif

/////////////////////////////////////////////////////////////////////////////
// Macros
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUC__)
#define GAZEBO_DEPRECATED __attribute__((deprecated))
#define GAZEBO_FORCEINLINE __attribute__((always_inline))
#elif defined(MSVC)
#define GAZEBO_DEPRECATED
#define GAZEBO_FORCEINLINE __forceinline
#else
#define GAZEBO_DEPRECATED
#define GAZEBO_FORCEINLINE
#endif


/// \file
/// \ingroup gazebo_common
/// \brief Forward declarations for the common classes
namespace gazebo
{
  class WorldPlugin;
  class ModelPlugin;
  class SensorPlugin;
  class GUIPlugin;
  class ServerPlugin;

  typedef boost::shared_ptr<WorldPlugin> WorldPluginPtr;
  typedef boost::shared_ptr<ModelPlugin> ModelPluginPtr;
  typedef boost::shared_ptr<SensorPlugin> SensorPluginPtr;
  typedef boost::shared_ptr<GUIPlugin> GUIPluginPtr;
  typedef boost::shared_ptr<ServerPlugin> ServerPluginPtr;

  namespace common
  {
    class Param;
    class Time;
    class Image;
    class Mesh;
    class MouseEvent;
    class Animation;

    template <typename T>
    class ParamT;

    typedef std::vector<common::Param*> Param_V;
    typedef std::map<std::string, std::string> StrStr_M;
    typedef boost::shared_ptr<Animation> AnimationPtr;
  }

  namespace event
  {
    class Connection;
    typedef boost::shared_ptr<Connection> ConnectionPtr;
    typedef std::vector<ConnectionPtr> Connection_V;
  }
}

#endif
