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
#ifndef _GAZEBO_MODELDATABSE_HH_
#define _GAZEBO_MODELDATABSE_HH_

#include <string>
#include <map>
#include <list>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include "gazebo/common/SingletonT.hh"

/// \brief The file name of model XML configuration.
#define GZ_MODEL_MANIFEST_FILENAME "model.config"

/// \brief The file name of model database XML configuration.
#define GZ_MODEL_DB_MANIFEST_FILENAME "database.config"

namespace gazebo
{
  namespace common
  {
    /// \addtogroup gazebo_common Common
    /// \{

    /// \class ModelDatabase ModelDatabase.hh common/common.hh
    /// \brief Connects to model database, and has utility functions to find
    /// models.
    class ModelDatabase : public SingletonT<ModelDatabase>
    {
      /// \brief Constructor. This will update the model cache
      private: ModelDatabase();

      /// \brief Destructor
      private: virtual ~ModelDatabase();

      /// \brief Returns the the global model database URI.
      /// \return the URI.
      public: std::string GetURI();

      /// \brief Returns the dictionary of all the model names
      ///
      /// This is a blocking call. Which means it will wait for the
      /// ModelDatabase to download the model list.
      /// \return a map of model names, indexed by their full URI.
      public: std::map<std::string, std::string> GetModels();

      /// \brief Get the dictionary of all model names via a callback.
      ///
      /// This is the non-blocking version of ModelDatabase::GetModels
      /// \param[in] _func Callback function that receives the list of
      /// models.
      public: void GetModels(boost::function<
                  void (const std::map<std::string, std::string> &)> _func);

      /// \brief Get the name of a model based on a URI.
      ///
      /// The URI must be fully qualified:
      /// http://gazebosim.org/gazebo_models/ground_plane or
      /// models://gazebo_models
      /// \param[in] _uri the model uri
      /// \return the model's name.
      public: std::string GetModelName(const std::string &_uri);

      /// \brief Return the gz_model_manifest.xml file as a string.
      /// \return the gz_model_manifest file from the model database.
      public: std::string GetModelManifest(const std::string &_uri);

      /// \brief Return the gz_model_db.xml file as a string.
      /// \return the gz_model_db file from the model database.
      public: std::string GetDBManifest(const std::string &_uri);

      /// \brief Deprecated.
      /// \sa ModelDatabase::GetModelManifest
      public: std::string GetManifest(const std::string &_uri)
              GAZEBO_DEPRECATED;

      /// \brief Get the local path to a model.
      ///
      /// Get the path to a model based on a URI. If the model is on
      /// a remote server, then the model fetched and installed locally.
      /// param[in] _uri the model uri
      /// \return path to a model directory
      public: std::string GetModelPath(const std::string &_uri);

      /// \brief Get a model's SDF file based on a URI.
      ///
      /// Get a model file based on a URI. If the model is on
      /// a remote server, then the model fetched and installed locally.
      /// \param[in] _uri The URI of the model
      /// \return The full path and filename to the SDF file
      public: std::string GetModelFile(const std::string &_uri);

      /// \brief Download all dependencies for a give model path
      ///
      /// Look's in the model's manifest file (_path/gz_model_manifest.xml)
      /// for all models listed in the <depend> block, and downloads the
      /// models if necessary.
      /// \param[in] _path Path to a model.
      public: void DownloadDependencies(const std::string &_path);

      /// \brief Returns true if the model exists on the database.
      ///
      /// \param[in] _modelName URI of the model (eg:
      /// model://my_model_name).
      /// \return True if the model was found.
      public: bool HasModel(const std::string &_modelName);

      /// \brief A helper function that uses CURL to get a manifest file.
      /// \param[in] _uri URI of a manifest XML file.
      /// \return The contents of the manifest file.
      private: std::string GetManifestImpl(const std::string &_uri);

      /// \brief Used by a thread to update the model cache.
      private: void UpdateModelCache();

      /// \brief Used by ModelDatabase::UpdateModelCache,
      /// no one else should use this function.
      private: bool UpdateModelCacheImpl();

      /// \brief A dictionary of all model names indexed by their uri.
      private: std::map<std::string, std::string> modelCache;

      /// \brief True to stop the background thread
      private: bool stop;

      /// \brief Cache update mutex
      private: boost::mutex updateMutex;

      /// \brief Thread to update the model cache.
      private: boost::thread *updateCacheThread;

      /// \brief Condition variable for the updateCacheThread.
      private: boost::condition_variable updateCacheCondition;

      /// \def CallbackFunc
      /// \brief Boost function that is used to passback the model cache.
      private: typedef boost::function<
               void (const std::map<std::string, std::string> &)> CallbackFunc;

      /// \brief List of all callbacks set from the
      /// ModelDatabase::GetModels function.
      private: std::list<CallbackFunc> callbacks;

      /// \brief Handy trick to automatically call a singleton's
      /// constructor.
      private: static ModelDatabase *myself;

      /// \brief Singleton implementation
      private: friend class SingletonT<ModelDatabase>;
    };
  }
}
#endif
