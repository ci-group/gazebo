/*
 * Copyright 2013 Open Source Robotics Foundation
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
/* Desc: ODE Trimesh shape
 * Author: Nate Koenig
 * Date: 16 Oct 2009
 */

#include "gazebo/common/Mesh.hh"
#include "gazebo/common/Exception.hh"
#include "gazebo/common/Console.hh"

#include "gazebo/physics/ode/ODECollision.hh"
#include "gazebo/physics/ode/ODEPhysics.hh"
#include "gazebo/physics/ode/ODEMeshShape.hh"

using namespace gazebo;
using namespace physics;


//////////////////////////////////////////////////
ODEMeshShape::ODEMeshShape(CollisionPtr _parent) : MeshShape(_parent)
{
  this->odeData = NULL;
  this->vertices = NULL;
  this->indices = NULL;
}

//////////////////////////////////////////////////
ODEMeshShape::~ODEMeshShape()
{
  delete [] this->vertices;
  delete [] this->indices;
  dGeomTriMeshDataDestroy(this->odeData);
}

//////////////////////////////////////////////////
void ODEMeshShape::Update()
{
  ODECollisionPtr ocollision =
    boost::dynamic_pointer_cast<ODECollision>(this->collisionParent);

  /// FIXME: use below to update trimesh geometry for collision without
  // using above Ogre codes
  // tell the tri-tri collider the current transform of the trimesh --
  // this is fairly important for good results.

  // Fill in the (4x4) matrix.
  dReal *matrix = this->transform + (this->transformIndex * 16);
  const dReal *Pos = dGeomGetPosition(ocollision->GetCollisionId());
  const dReal *Rot = dGeomGetRotation(ocollision->GetCollisionId());

  matrix[ 0 ] = Rot[ 0 ];
  matrix[ 1 ] = Rot[ 1 ];
  matrix[ 2 ] = Rot[ 2 ];
  matrix[ 3 ] = 0;
  matrix[ 4 ] = Rot[ 4 ];
  matrix[ 5 ] = Rot[ 5 ];
  matrix[ 6 ] = Rot[ 6 ];
  matrix[ 7 ] = 0;
  matrix[ 8 ] = Rot[ 8 ];
  matrix[ 9 ] = Rot[ 9 ];
  matrix[10 ] = Rot[10 ];
  matrix[11 ] = 0;
  matrix[12 ] = Pos[ 0 ];
  matrix[13 ] = Pos[ 1 ];
  matrix[14 ] = Pos[ 2 ];
  matrix[15 ] = 1;

  // Flip to other matrix.
  this->transformIndex = !this->transformIndex;

  dGeomTriMeshSetLastTransform(ocollision->GetCollisionId(),
      *reinterpret_cast<dMatrix4*>(this->transform +
                                   this->transformIndex * 16));
}

//////////////////////////////////////////////////
void ODEMeshShape::Load(sdf::ElementPtr _sdf)
{
  MeshShape::Load(_sdf);
}

//////////////////////////////////////////////////
void ODEMeshShape::Init()
{
  MeshShape::Init();
  if (!this->mesh)
    return;

  ODECollisionPtr pcollision =
    boost::static_pointer_cast<ODECollision>(this->collisionParent);

  /// This will hold the vertex data of the triangle mesh
  if (this->odeData == NULL)
    this->odeData = dGeomTriMeshDataCreate();

  unsigned int numVertices = this->submesh ? this->submesh->GetVertexCount() :
    this->mesh->GetVertexCount();

  unsigned int numIndices = this->submesh ? this->submesh->GetIndexCount() :
    this->mesh->GetIndexCount();

  this->vertices = NULL;
  this->indices = NULL;

  // Get all the vertex and index data
  if (!this->submesh)
    this->mesh->FillArrays(&this->vertices, &this->indices);
  else
    this->submesh->FillArrays(&this->vertices, &this->indices);

  // Scale the vertex data
  for (unsigned int j = 0;  j < numVertices; j++)
  {
    this->vertices[j*3+0] = this->vertices[j*3+0] *
      this->sdf->Get<math::Vector3>("scale").x;
    this->vertices[j*3+1] = this->vertices[j*3+1] *
      this->sdf->Get<math::Vector3>("scale").y;
    this->vertices[j*3+2] = this->vertices[j*3+2] *
      this->sdf->Get<math::Vector3>("scale").z;
  }

  // Build the ODE triangle mesh
  dGeomTriMeshDataBuildSingle(this->odeData,
      this->vertices, 3*sizeof(this->vertices[0]), numVertices,
      this->indices, numIndices, 3*sizeof(this->indices[0]));

  if (pcollision->GetCollisionId() == NULL)
  {
    pcollision->SetSpaceId(dSimpleSpaceCreate(pcollision->GetSpaceId()));
    pcollision->SetCollision(dCreateTriMesh(pcollision->GetSpaceId(),
          this->odeData, 0, 0, 0), true);
  }
  else
  {
    dGeomTriMeshSetData(pcollision->GetCollisionId(), this->odeData);
  }

  memset(this->transform, 0, 32*sizeof(dReal));
  this->transformIndex = 0;
}
