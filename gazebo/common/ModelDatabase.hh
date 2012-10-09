/*
 * Copyright 2011 Nate Koenig
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
#ifndef _GAZEBO_MODELDATABSE_HH_
#define _GAZEBO_MODELDATABSE_HH_

#include <string>
#include <map>

namespace gazebo
{
  namespace common
  {
    /// \addtogroup gazebo_common Common
    /// \{

    /// \class ModelDatabase ModelDatabase.hh common/ModelDatabase.hh
    /// \brief Connects to model database, and has utility functions to find
    /// models
    class ModelDatabase
    {
      public: static std::string GetURI();

      public: static std::map<std::string, std::string> GetModels();

      /// \brief Return the manifest.xml file as a string.
      /// \return The manifest file from the model database.
      public: static std::string GetManifest();

      /// \brief Get the local path to a model.
      ///
      /// Get the path to a model based on a URI. If the model is on
      /// a remote server, then the model fetched and installed locally.
      /// \return Path to a model directory
      public: static std::string GetModelPath(const std::string &_uri);

      /// \brief Get a model's SDF file based on a URI
      ///
      /// Get a model file based on a URI. If the model is on
      /// a remote server, then the model fetched and installed locally.
      /// \param _uri The URI of the model
      /// \return The full path and filename to the SDF file
      public: static std::string GetModelFile(const std::string &_uri);

      /// \brief Download all dependencies for a give model path
      ///
      /// Look's in the model's manifest file (_path/manifest.xml) for all
      /// models listed in the <depend> block, and downloads the models if
      /// necessary.
      /// \param[in] _path Path to a model.
      public: static void DownloadDependencies(const std::string &_path);

      /// \brief Returns true if the model exists on the database.
      ///
      /// \param[in] _modelName URI of the model (eg:
      /// model://my_model_name).
      /// \return True if the model was found.
      public: static bool HasModel(const std::string &_modelName);
    };
  }
}

#endif
