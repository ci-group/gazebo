/*
 * Copyright (C) 2014 Open Source Robotics Foundation
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
#ifndef _MODEL_DATA_HH_
#define _MODEL_DATA_HH_

#include <string>
#include <vector>

#include "gazebo/rendering/Visual.hh"
#include "gazebo/physics/PhysicsTypes.hh"

#include "gazebo/gui/model/PartInspector.hh"

namespace boost
{
  class recursive_mutex;
}

namespace gazebo
{
  namespace gui
  {
    class ModelData
    {
      /// \brief Get a template SDF string of a simple model.
      /// \return Template SDF string of a simple model.
      public: static std::string GetTemplateSDFString();
    };

    /// \class CollisionData CollisionData.hh
    /// \brief Helper class to store collision data
    class CollisionData
    {
      /// \brief Constructor
      public: CollisionData();

      /// \brief Name of collision.
      public: std::string name;

      /// \brief Pose of collision.
      public: math::Vector3 pose;

      /// \brief SDF representing the visual data.
      public: sdf::ElementPtr collisionSDF;
    };


    /// \class PartData PartData.hh
    /// \brief Helper class to store part data
    class PartData : public QObject
    {
      Q_OBJECT

      /// \brief Constructor
      public: PartData();

      /// \brief Destructor
      public: ~PartData();

      /// \brief Get the name of the part.
      /// \return Name of part.
      public: std::string GetName() const;

      /// \brief Set the name of the part.
      /// \param[in] _name Name of part.
      public: void SetName(const std::string &_name);

      /// \brief Get the pose of the part.
      /// \return Pose of part.
      public: math::Pose GetPose() const;

      /// \brief Set the pose of the part.
      /// \param[in] _pose Pose of part.
      public: void SetPose(const math::Pose &_pose);

      /// \brief Add a visual to the part.
      /// \param[in] _visual Visual to be added.
      public: void AddVisual(rendering::VisualPtr _visual);

      /// \brief Add a collision to the part.
      /// \param[in] _collision Visual representing the collision.
      public: void AddCollision(rendering::VisualPtr _collisionVis);

      /// \brief Update callback on PreRender.
      public: void Update();

      /// \brief SDF representing the part data.
      public: sdf::ElementPtr partSDF;

      /// \brief Visual representing this part.
      public: rendering::VisualPtr partVisual;

      /// \brief Visuals of the part.
      //public: std::map<std::string, VisualData *> visuals;
      //public: std::vector<rendering::VisualPtr> visuals;
      public: std::map<rendering::VisualPtr, msgs::Visual> visuals;

      /// \brief Msgs for updating visuals.
      public: std::vector<msgs::Visual *> visualUpdateMsgs;

      /// \brief Msgs for updating collision visuals.
      public: std::vector<msgs::Collision *> collisionUpdateMsgs;

      /// \brief Collisions of the part.
      public: std::map<rendering::VisualPtr, msgs::Collision> collisions;

      /// \brief Inspector for configuring part properties.
      public: PartInspector *inspector;

      /// \brief Qt Callback when part inspector configurations are to be
      /// applied.
      private slots: void OnApply();

      /// \brief Qt callback when a new visual is to be added.
      /// \param[in] _name Name of visual.
      private slots: void OnAddVisual(const std::string &_name);

      /// \brief Qt callback when a new collision is to be added.
      /// \param[in] _name Name of collision.
      private slots: void OnAddCollision(const std::string &_name);

      /// \brief Qt callback when a visual is to be removed.
      /// \param[in] _name Name of visual.
      private slots: void OnRemoveVisual(const std::string &_name);

      /// \brief Qt callback when a collision is to be removed.
      /// \param[in] _name Name of collision.
      private slots: void OnRemoveCollision(const std::string &_name);

      /// \brief All the event connections.
      private: std::vector<event::ConnectionPtr> connections;

      /// \brief Mutex to protect the list of joints
      private: boost::recursive_mutex *updateMutex;
    };
  }
}

#endif
