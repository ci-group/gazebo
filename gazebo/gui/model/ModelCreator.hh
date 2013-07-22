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
#ifndef _MODEL_CREATOR_HH_
#define _MODEL_CREATOR_HH_

#include <list>
#include <string>
#include <vector>

#include <boost/unordered/unordered_map.hpp>

#include "gazebo/sdf/sdf.hh"
#include "gazebo/gui/EntityMaker.hh"
#include "gazebo/gui/qt.h"

#include "gazebo/gui/model/JointMaker.hh"

namespace gazebo
{
  namespace msgs
  {
    class Visual;
  }

  namespace gui
  {
    /// \addtogroup gazebo_gui
    /// \{

    /// \class ModelCreator ModelCreator.hh
    /// \brief Create and manage 3D visuals of a model with parts and joints.
    class ModelCreator : public QObject
    {
      Q_OBJECT

      /// \enum Joint types
      /// \brief Unique identifiers for joint types that can be created.
      public: enum PartType
      {
        /// \brief none
        PART_NONE,
        /// \brief Box
        PART_BOX,
        /// \brief Sphere
        PART_SPHERE,
        /// \brief Cylinder
        PART_CYLINDER,
        /// \brief Cylinder
        PART_CUSTOM
      };

      /// \brief Constructor
      public: ModelCreator();

      /// \brief Destructor
      public: virtual ~ModelCreator();

      /// \brief Set the name of the model.
      /// \param[in] _modelName Name of the model to set to.
      public: void SetModelName(const std::string &_modelName);

      /// \brief Get the name of the model.
      public: std::string GetModelName() const;

      /// \brief Finish the model and create the entity on the gzserver.
       public: void FinishModel();

      /// \brief Add a box to the model.
      /// \param[in] _size Size of the box.
      /// \param[in] _pose Pose of the box.
      /// \return Name of the box that has been added.
      public: std::string AddBox(
          const math::Vector3 &_size = math::Vector3::One,
          const math::Pose &_pose = math::Pose::Zero);

      /// \brief Add a sphere to the model.
      /// \param[in] _size Size of the sphere.
      /// \param[in] _pose Pose of the sphere.
      /// \return Name of the sphere that has been added.
      public: std::string AddSphere(double _radius = 0.5,
          const math::Pose &_pose = math::Pose::Zero);

      /// \brief Add a cylinder to the model.
      /// \param[in] _size Size of the cylinder.
      /// \param[in] _pose Pose of the cylinder.
      /// \return Name of the cylinder that has been added.
      public: std::string AddCylinder(double _radius = 0.5,
          double _length = 1.0, const math::Pose &_pose = math::Pose::Zero);

      /// \brief Add a custom part to the model
      /// \param[in] _name Name of the custom part.
      /// \param[in] _scale Scale of the custom part.
      /// \param[in] _pose Pose of the custom part.
      /// \return Name of the custom that has been added.
      public: std::string AddCustom(std::string _name,
          const math::Vector3 &_scale = math::Vector3::One,
          const math::Pose &_pose = math::Pose::Zero);

      /// \brief Add a joint to the model.
      /// \param[in] _type Type of joint to add.
      /// \return Name of the joint that has been added.
      public: void AddJoint(JointMaker::JointType _type);

      /// \brief Remove a part from the model.
      /// \param[in] _partName Name of the part to remove
      public: void RemovePart(const std::string &_partName);

      /// \brief Save model to SDF format.
      /// \param[in] _savePath Path to save the SDF to.
      public: void SaveToSDF(const std::string &_savePath);

      /// \brief Reset the model creator and the SDF.
      public: void Reset();

      /// \brief Get joint maker
      /// \return Joint maker
      public: JointMaker *GetJointMaker() const;

      /// \brief Add a part to the model
      /// \param[in] _type Type of part to be added
      public: void AddPart(PartType _type);

      /// \brief Generate the SDF from model part and joint visuals.
      public: void GenerateSDF();

      /// \brief Mouse event filter callback when mouse button is pressed
      /// \param[in] _event The mouse event.
      /// \return True if the event was handled
      private: bool OnMousePressPart(const common::MouseEvent &_event);

      /// \brief Mouse event filter callback when mouse is moved.
      /// \param[in] _event The mouse event.
      /// \return True if the event was handled
      private: bool OnMouseMovePart(const common::MouseEvent &_event);

      /// \brief Mouse event filter callback when mouse is double clicked.
      /// \param[in] _event The mouse event.
      /// \return True if the event was handled
      private: bool OnMouseDoubleClickPart(const common::MouseEvent &_event);

      // Documentation inherited
      public: virtual bool IsActive() const;

      // Documentation inherited
      private: virtual void CreateTheEntity();

      /// \brief Internal init function.
      private: bool Init();

      /// \brief Create an empty model.
      /// \return Name of the model created.
      private: std::string CreateModel();

      /// \brief Get a template SDF string of a simple model.
      private: std::string GetTemplateSDFString();

      /// \brief Qt signal when the a part has been added.
      Q_SIGNALS: void PartAdded();

      /// \brief Callback received when exiting the editor mode.
//      private: void OnExit();

      /// \brief The model in SDF format.
      private: sdf::SDFPtr modelSDF;

      /// \brief A template SDF of a simple box model.
      private: sdf::SDFPtr modelTemplateSDF;

      /// \brief Name of the model.
      private: std::string modelName;

      /// \brief The root visual of the model.
      private: rendering::VisualPtr modelVisual;

      /// \brief The root visual of the model.
      private: rendering::VisualPtr mouseVisual;

      /// \brief The pose of the model.
      private: math::Pose modelPose;

      /// \brief A list of gui editor events connected to the model creator.
      private: std::vector<event::ConnectionPtr> connections;

      /// \brief Counter for the number of boxes in the model.
      private: int boxCounter;

      /// \brief Counter for the number of cylinders in the model.
      private: int cylinderCounter;

      /// \brief Counter for the number of spheres in the model.
      private: int sphereCounter;

      /// \brief Counter for the number of custom parts in the model.
      private: int customCounter;

      /// \brief Type of part being added.
      private: PartType addPartType;

      /// \brief A map of model part names to and their visuals.
      private: boost::unordered_map<std::string, rendering::VisualPtr> allParts;

      /// \brief Transport node
      private: transport::NodePtr node;

      /// \brief Publisher that publishes msg to the server once the model is
      /// created.
      private: transport::PublisherPtr makerPub;

      /// \brief Publisher that publishes delete entity msg to remove the
      /// editor visual.
      private: transport::PublisherPtr requestPub;

      /// \brief Joint maker.
      private: JointMaker *jointMaker;
    };
    /// \}
  }
}
#endif
