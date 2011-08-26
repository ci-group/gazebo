#ifndef SERVER_HH
#define SERVER_HH

#include <string>
#include <vector>

#include <boost/thread.hpp>

#include "common/CommonTypes.hh"
#include "physics/PhysicsTypes.hh"
#include "physics/World.hh"


namespace gazebo
{
  class Master;

  class Server
  {
    public: Server();
    public: virtual ~Server();

    public: bool Load(const std::string &filename);
    public: void Init();
    public: void Run();
    public: void Stop();
    public: void Fini();

    public: void SetParams( const common::StrStr_M &params );

    private: void RunLoop();

    private: bool stop;

    private: Master *master;
    private: boost::thread *masterThread;
  };
}

#endif
