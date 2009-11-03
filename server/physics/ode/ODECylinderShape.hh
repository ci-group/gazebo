#ifndef ODECYLINDERSHAPE_HH
#define ODECYLINDERSHAPE_HH

#include "Mass.hh"
#include "Vector2.hh"
#include "ODEPhysics.hh"
#include "CylinderShape.hh"

namespace gazebo
{
  class ODECylinderShape : public CylinderShape
  {
    public: ODECylinderShape(Geom *parent) : CylinderShape(parent) {}
    public: virtual ~ODECylinderShape() {}
    public: void SetSize(const Vector2<double> &size)
            {
              CylinderShape::SetSize(size);
              PhysicsEngine *physics = World::Instance()->GetPhysicsEngine();
              ODEGeom *oParent = (ODEGeom*)(this->parent);

              dMass odeMass;
              Pose3d rpose;
    
              Mass mass = this->parent->GetMass();
  
              // Initialize mass matrix
              dMassSetCylinderTotal(&odeMass, mass.GetAsDouble(), 3, 
                                    size.x, size.y);
              rpose = this->parent->GetRelativePose();
              dMassTranslate(&odeMass, rpose.pos.x, rpose.pos.y, rpose.pos.z);
 
              physics->ConvertMass(&mass, &odeMass);
  
              this->parent->SetMass(mass);

              oParent->SetGeom( dCreateCylinder( 0, size.x, size.y ), true );
            }
  };
}
#endif
