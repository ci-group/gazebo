/*
 * Copyright (C) 2012-2014 Open Source Robotics Foundation
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

#ifndef _COLLADALOADER_PRIVATE_HH_
#define _COLLADALOADER_PRIVATE_HH_

#include <map>
#include <string>
#include <vector>

#include "gazebo/math/MathTypes.hh"

class TiXmlElement;

namespace gazebo
{
  namespace common
  {
    class Material;

    /// \brief Private data for the ColladaLoader class
    class  ColladaLoaderPrivate
    {
      /// \brief scaling factor
      public: double meter;

      /// \brief COLLADA file name
      public: std::string filename;

      /// \brief material dictionary indexed by name
      public: std::map<std::string, std::string> materialMap;

      /// \brief root xml element of COLLADA data
      public: TiXmlElement *colladaXml;

      /// \brief directory of COLLADA file name
      public: std::string path;

      /// \brief Name of the current node.
      public: std::string currentNodeName;

      /// \brief Map of collada POSITION ids to list of vectors.
      public: std::map<std::string, std::vector<math::Vector3> > positionIds;

      /// \brief Map of collada NORMAL ids to list of normals.
      public: std::map<std::string, std::vector<math::Vector3> > normalIds;

      /// \brief Map of collada TEXCOORD ids to list texture coordinates.
      public: std::map<std::string, std::vector<math::Vector2d> >texcoordIds;

      /// \brief Map of collada  Material ids to Gazebo materials.
      public: std::map<std::string, Material *> materialIds;
    };

    /// \brief Helper data structure for loading collada geometries.
    class GeometryIndices
    {
      /// \brief Index of a vertex in the collada <p> element
      public: int vertexIndex;

      /// \brief Index of a normal in the collada <p> element
      public: int normalIndex;

      /// \brief Index of a texture coordinate in the collada <p> element
      public: int texcoordIndex;

      /// \brief Index of a vertex in the Gazebo mesh
      public: int mappedIndex;
    };
  }
}
#endif
