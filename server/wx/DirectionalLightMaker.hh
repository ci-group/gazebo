#ifndef DIRECTIONALLIGHTMAKER_HH
#define DIRECTIONALLIGHTMAKER_HH

#include "Vector2.hh"
#include "EntityMaker.hh"

namespace gazebo
{
  class DirectionalLightMaker : public EntityMaker
  {
    public: DirectionalLightMaker();
    public: virtual ~DirectionalLightMaker();
  
    public: virtual void Start();
    public: virtual void Stop();
    public: virtual bool IsActive() const;

    public: virtual void MousePushCB(const MouseEvent &event);
    public: virtual void MouseReleaseCB(const MouseEvent &event);
    public: virtual void MouseDragCB(const MouseEvent &event);
  
    private: virtual void CreateTheEntity();
    private: int state;
    private: Vector3 createPos;
    private: std::string lightName;
    private: int index;
  };
}

#endif
