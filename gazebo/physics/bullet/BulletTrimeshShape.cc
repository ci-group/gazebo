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
/* Desc: Trimesh shape
 * Author: Nate Keonig
 * Date: 21 May 2009
 */

#include "common/Mesh.hh"

#include "physics/bullet/BulletTypes.hh"
#include "physics/bullet/BulletCollision.hh"
#include "physics/bullet/BulletPhysics.hh"
#include "physics/bullet/BulletTrimeshShape.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
BulletTrimeshShape::BulletTrimeshShape(CollisionPtr _parent)
  : TrimeshShape(_parent)
{
}


//////////////////////////////////////////////////
BulletTrimeshShape::~BulletTrimeshShape()
{
}

//////////////////////////////////////////////////
void BulletTrimeshShape::Load(sdf::ElementPtr _sdf)
{
  TrimeshShape::Load(_sdf);
}

//////////////////////////////////////////////////
void BulletTrimeshShape::Init()
{
  TrimeshShape::Init();

  BulletCollisionPtr bParent =
    boost::shared_static_cast<BulletCollision>(this->collisionParent);

  float *vertices = NULL;
  int *indices = NULL;

  btTriangleMesh *mTriMesh = new btTriangleMesh();

  unsigned int numVertices = this->mesh->GetVertexCount();
  unsigned int numIndices = this->mesh->GetIndexCount();

  // Get all the vertex and index data
  this->mesh->FillArrays(&vertices, &indices);

  // Scale the vertex data
  for (unsigned int j = 0;  j < numVertices; j++)
  {
    vertices[j*3+0] = vertices[j*3+0] * this->sdf->GetValueVector3("scale").x;
    vertices[j*3+1] = vertices[j*3+1] * this->sdf->GetValueVector3("scale").y;
    vertices[j*3+2] = vertices[j*3+2] * this->sdf->GetValueVector3("scale").z;
  }

  // Create the Bullet trimesh
  for (unsigned int j = 0; j < numIndices; j += 3)
  {
    btVector3 bv0(vertices[indices[j]*3+0],
                  vertices[indices[j]*3+1],
                  vertices[indices[j]*3+2]);

    btVector3 bv1(vertices[indices[j+1]*3+0],
                  vertices[indices[j+1]*3+1],
                  vertices[indices[j+1]*3+2]);

    btVector3 bv2(vertices[indices[j+2]*3+0],
                  vertices[indices[j+2]*3+1],
                  vertices[indices[j+2]*3+2]);

    mTriMesh->addTriangle(bv0, bv1, bv2);
  }

  bParent->SetCollisionShape(new btConvexTriangleMeshShape(mTriMesh, true));

  delete [] vertices;
  delete [] indices;
}
