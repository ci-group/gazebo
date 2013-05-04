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
/* Desc: Trimesh shape
 * Author: Nate Koenig
 * Date: 16 Oct 2009
 */

#include "common/CommonIface.hh"
#include "common/MeshManager.hh"
#include "common/Mesh.hh"
#include "common/Exception.hh"

#include "physics/World.hh"
#include "physics/PhysicsEngine.hh"
#include "physics/Collision.hh"
#include "physics/TrimeshShape.hh"

using namespace gazebo;
using namespace physics;

//////////////////////////////////////////////////
TrimeshShape::TrimeshShape(CollisionPtr _parent)
  : Shape(_parent)
{
  this->submesh = NULL;
  this->AddType(Base::TRIMESH_SHAPE);
}


//////////////////////////////////////////////////
TrimeshShape::~TrimeshShape()
{
}

//////////////////////////////////////////////////
void TrimeshShape::Init()
{
  std::string filename;

  this->mesh = NULL;
  common::MeshManager *meshManager = common::MeshManager::Instance();

  filename = common::find_file(this->sdf->GetValueString("uri"));

  if (filename == "__default__" || filename.empty())
  {
    gzerr << "No mesh specified\n";
    return;
  }

  if ((this->mesh = meshManager->Load(filename)) == NULL)
    gzerr << "Unable to load mesh from file[" << filename << "]\n";

  if (this->submesh)
    delete this->submesh;
  this->submesh = NULL;

  if (this->sdf->HasElement("submesh"))
  {
    sdf::ElementPtr submeshElem = this->sdf->GetElement("submesh");
    this->submesh = new common::SubMesh(
      this->mesh->GetSubMesh(submeshElem->GetValueString("name")));

    if (!this->submesh)
      gzthrow("Unable to get submesh with name[" +
          submeshElem->GetValueString("name") + "]");

    // Center the submesh if specified in SDF.
    if (submeshElem->HasElement("center") &&
        submeshElem->GetValueBool("center"))
    {
      this->submesh->Center();
    }
  }
}

//////////////////////////////////////////////////
void TrimeshShape::SetScale(const math::Vector3 &_scale)
{
  this->sdf->GetElement("scale")->Set(_scale);
}

//////////////////////////////////////////////////
math::Vector3 TrimeshShape::GetSize() const
{
  return this->sdf->GetValueVector3("scale");
}

//////////////////////////////////////////////////
std::string TrimeshShape::GetFilename() const
{
  return this->GetMeshURI();
}

//////////////////////////////////////////////////
std::string TrimeshShape::GetMeshURI() const
{
  return this->sdf->GetValueString("uri");
}

//////////////////////////////////////////////////
void TrimeshShape::SetFilename(const std::string &_filename)
{
  this->SetMesh(_filename);
}

//////////////////////////////////////////////////
void TrimeshShape::SetMesh(const std::string &_uri,
                           const std::string &_submesh,
                           bool _center)
{
  if (_uri.find("://") == std::string::npos)
  {
    gzerr << "Invalid URI[" << _uri
          << "]. Must use a URI, like file://" << _uri << "\n";
    return;
  }

  this->sdf->GetElement("uri")->Set(_uri);
  this->sdf->GetElement("submesh")->GetElement("name")->Set(_submesh);
  this->sdf->GetElement("submesh")->GetElement("center")->Set(_center);

  this->Init();
}

//////////////////////////////////////////////////
void TrimeshShape::FillMsg(msgs::Geometry &_msg)
{
  _msg.set_type(msgs::Geometry::MESH);
  _msg.mutable_mesh()->CopyFrom(msgs::MeshFromSDF(this->sdf));
}

//////////////////////////////////////////////////
void TrimeshShape::ProcessMsg(const msgs::Geometry &_msg)
{
  this->SetScale(msgs::Convert(_msg.mesh().scale()));
  this->SetMesh(_msg.mesh().filename(),
      _msg.mesh().has_submesh() ? _msg.mesh().submesh() : std::string(),
      _msg.mesh().has_center_submesh() ? _msg.mesh().center_submesh() :  false);
}
