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

#ifndef _JOINTMAKER_HH_
#define _JOINTMAKER_HH_

#include <string>
#include <vector>
#include <boost/unordered/unordered_map.hpp>

#include <sdf/sdf.hh>

#include "gazebo/common/MouseEvent.hh"
#include "gazebo/common/CommonTypes.hh"
#include "gazebo/math/Vector3.hh"
#include "gazebo/rendering/RenderTypes.hh"
#include "gazebo/gui/qt.h"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace gui
  {
    class JointData;
    class JointInspector;

    /// \addtogroup gazebo_gui
    /// \{

    /// \class JointMaker JointMaker.hh
    /// \brief Joint visualization
    class GAZEBO_VISIBLE JointMaker : public QObject
    {
      Q_OBJECT

      /// \enum Joint types
      /// \brief Unique identifiers for joint types that can be created.
      public: enum JointType
      {
        /// \brief none
        JOINT_NONE,
        /// \brief Fixed joint
        JOINT_FIXED,
        /// \brief Slider joint
        JOINT_SLIDER,
        /// \brief Hinge joint
        JOINT_HINGE,
        /// \brief Hinge2 joint
        JOINT_HINGE2,
        /// \brief Screw joint
        JOINT_SCREW,
        /// \brief Universal joint
        JOINT_UNIVERSAL,
        /// \brief Revolute joint
        JOINT_BALL
      };

      /// \brief Constructor
      public: JointMaker();

      /// \brief Destructor
      public: virtual ~JointMaker();

      /// \brief Reset the joint maker;
      public: void Reset();

      /// \brief Create a joint
      /// \param[_type] Type of joint to be created
      public: void CreateJoint(JointType _type);

      /// \brief Update callback on PreRender.
      public: void Update();

      /// \brief Remove joint by name
      /// \param[in] _jointName Name of joint to be removed.
      public: void RemoveJoint(const std::string &_jointName);

      /// \brief Remove all joints connected to part
      /// \param[in] _partName Name of joint to be removed.
      public: void RemoveJointsByPart(const std::string &_partName);

      /// \brief Generate SDF for all joints.
      public: void GenerateSDF();

      /// \brief Generate SDF for all joints.
      public: sdf::ElementPtr GetSDF() const;

      /// \brief Get the axis count for joint type.
      /// \param[in] _type Type of joint.
      public: static int GetJointAxisCount(JointMaker::JointType _type);

      /// \brief Get the joint type in string.
      /// \param[in] _type Type of joint.
      /// \return Joint type in string.
      public: static std::string GetTypeAsString(JointMaker::JointType _type);

      /// \brief Get state
      /// \return State of JointType if joint creation is in process, otherwise
      /// JOINT_NONE
      public: JointMaker::JointType GetState() const;

      /// \brief Stop the process of adding joint to the model.
      public: void Stop();

      /// \brief Mouse event filter callback when mouse button is pressed.
      /// \param[in] _event The mouse event.
      /// \return True if the event was handled
      private: bool OnMousePress(const common::MouseEvent &_event);

      /// \brief Mouse event filter callback when mouse button is released.
      /// \param[in] _event The mouse event.
      /// \return True if the event was handled
      private: bool OnMouseRelease(const common::MouseEvent &_event);

      /// \brief Mouse event filter callback when mouse is moved.
      /// \param[in] _event The mouse event.
      /// \return True if the event was handled
      private: bool OnMouseMove(const common::MouseEvent &_event);

      /// \brief Mouse event filter callback when mouse is double clicked.
      /// \param[in] _event The mouse event.
      /// \return True if the event was handled
      private: bool OnMouseDoubleClick(const common::MouseEvent &_event);

      /// \brief Helper method to create hotspot visual for mouse interaction.
      private: void CreateHotSpot();

      /// \brief Open joint inspector.
      /// \param[in] _name Name of joint.
      private: void OpenInspector(const std::string &_name);

      /// \brief Qt signal when the joint creation process has ended.
      Q_SIGNALS: void JointAdded();

      /// \brief Qt Callback to open joint inspector
      private slots: void OnOpenInspector();

      /// \brief Type of joint to create
      private: JointMaker::JointType jointType;

      /// \brief Visual that is currently hovered over by the mouse
      private: rendering::VisualPtr hoverVis;

      /// \brief Visual that is previously hovered over by the mouse
      private: rendering::VisualPtr prevHoverVis;

      /// \brief Currently selected visual
      private: rendering::VisualPtr selectedVis;

      /// \brief Visual that is currently being inspected.
      private: rendering::VisualPtr inspectVis;

      /// \brief All joints created by joint maker.
      private: boost::unordered_map<std::string, JointData *> joints;

      /// \brief Joint currently being created.
      private: JointData *mouseJoint;

      /// \brief All the event connections.
      private: std::vector<event::ConnectionPtr> connections;

      /// \brief Flag set to true when a joint has been connected.
      private: bool newJointCreated;

      /// \brief A map of joint type to its corresponding material.
      private: boost::unordered_map<JointMaker::JointType, std::string>
          jointMaterials;

      /// \brief The SDF element pointer to the model that contains the joints.
      private: sdf::ElementPtr modelSDF;

      /// \brief Counter for the number of joints in the model.
      private: int jointCounter;

      /// \brief Qt action for opening the joint inspector.
      private: QAction *inspectAct;
    };
    /// \}


    /// \class JointData JointData.hh
    /// \brief Helper class to store joint data
    class GAZEBO_VISIBLE JointData : public QObject
    {
      Q_OBJECT

      /// \brief Visual of the dynamic line
      public: rendering::VisualPtr visual;

      /// \brieft Visual of the hotspot
      public: rendering::VisualPtr hotspot;

      /// \brief Parent visual the joint is connected to.
      public: rendering::VisualPtr parent;

      /// \brief Child visual the joint is connected to.
      public: rendering::VisualPtr child;

      /// \brief Visual line used to represent joint connecting parent and child
      public: rendering::DynamicLines *line;

      /// \brief Type of joint.
      public: JointMaker::JointType type;

      /// \brief Joint axis direction.
      public: math::Vector3 axis[2];

      /// \brief Joint lower limit.
      public: double lowerLimit[2];

      /// \brief Joint upper limit.
      public: double upperLimit[2];

      /// \brief Joint anchor point.
      public: math::Vector3 anchor;

      /// \brief True if the joint visual needs update.
      public: bool dirty;

      /// \brief Inspector for configuring joint properties.
      public: JointInspector *inspector;

      /// \brief Qt Callback when joint inspector configurations are to be
      /// applied.
      private slots: void OnApply();
    };
    /// \}
  }
}
#endif
