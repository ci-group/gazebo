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
/* Desc: A model state
 * Author: Nate Koenig
 */

#ifndef _MODELSTATE_HH_
#define _MODELSTATE_HH_

#include <vector>
#include <string>
#include <boost/regex.hpp>

#include "gazebo/math/Pose.hh"

#include "gazebo/physics/State.hh"
#include "gazebo/physics/LinkState.hh"
#include "gazebo/physics/JointState.hh"

namespace gazebo
{
  namespace physics
  {
    /// \addtogroup gazebo_physics
    /// \{

    /// \class ModelState ModelState.hh physics/physics.hh
    /// \brief Store state information of a physics::Model object
    ///
    /// This class captures the entire state of a Model at one
    /// specific time during a simulation run.
    ///
    /// State of a Model includes the state of all its child Links and
    /// Joints.
    class ModelState : public State
    {
      /// \brief Default constructor.
      public: ModelState();

      /// \brief Constructor.
      ///
      /// Build a ModelState from an existing Model.
      /// \param[in] _model Pointer to the model from which to gather state
      /// info.
      public: explicit ModelState(const ModelPtr _model);

      /// \brief Constructor
      ///
      /// Build a ModelState from SDF data
      /// \param[in] _sdf SDF data to load a model state from.
      public: explicit ModelState(const sdf::ElementPtr _sdf);

      /// \brief Destructor.
      public: virtual ~ModelState();

      /// \brief Load state from SDF element.
      ///
      /// Load ModelState information from stored data in and SDF::Element
      /// \param[in] _elem Pointer to the SDF::Element containing state info.
      public: virtual void Load(const sdf::ElementPtr _elem);

      /// \brief Get the stored model pose.
      /// \return The math::Pose of the Model.
      public: const math::Pose &GetPose() const;

      /// \brief Return true if the values in the state are zero.
      /// \return True if the values in the state are zero.
      public: bool IsZero() const;

      /// \brief Get the number of link states.
      ///
      /// This returns the number of Links recorded.
      /// \return Number of LinkState recorded.
      public: unsigned int GetLinkStateCount() const;

      /// \brief Get link states based on a regular expression.
      /// \param[in] _regex The regular expression.
      /// \return List of link states whose names match the regular
      /// expression.
      public: std::vector<LinkState> GetLinkStates(
                  const boost::regex &_regex) const;

      /// \brief Get joint states based on a regular expression.
      /// \param[in] _regex The regular expression.
      /// \return List of joint states whose names match the regular
      /// expression.
      public: std::vector<JointState> GetJointStates(
                  const boost::regex &_regex) const;

      /// \brief Get a link state.
      ///
      /// Get a Link State based on an index, where index is in the range of
      /// 0...ModelState::GetLinkStateCount
      /// \param[in] _index Index of the LinkState
      /// \return State of the Link.
      /// \throws common::Exception When _index is out of range.
      public: LinkState GetLinkState(unsigned int _index) const;

      /// \brief Get a link state by Link name
      ///
      /// Searches through all LinkStates. Returns the LinkState with the
      /// matching name, if any.
      /// \param[in] _linkName Name of the LinkState
      /// \return State of the Link.
      /// \throws common::Exception When _linkName is invalid.
      public: LinkState GetLinkState(const std::string &_linkName) const;

      /// \brief Return true if there is a link with the specified name.
      /// \param[in] _linkName Name of the LinkState.
      /// \return True if the link exists in the model.
      public: bool HasLinkState(const std::string &_linkName) const;

      /// \brief Get the link states.
      /// \return A vector of link states.
      public: const std::vector<LinkState> &GetLinkStates() const;

      /// \brief Get the number of joint states.
      ///
      /// Returns the number of JointStates recorded.
      /// \return Number of JointStates.
      public: unsigned int GetJointStateCount() const;

      /// \brief Get a Joint state.
      ///
      /// Return a JointState based on a index, where index is between
      /// 0...ModelState::GetJointStateCount().
      /// \param[in] _index Index of a JointState.
      /// \return State of a Joint.
      /// \throws common::Exception When _index is out of range.
      public: JointState GetJointState(unsigned int _index) const;

      /// \brief Get a Joint state by Joint name.
      ///
      /// Searches through all JointStates. Returns the JointState with the
      /// matching name, if any.
      /// \param[in] _jointName Name of the JointState.
      /// \return State of the Joint.
      /// \throws common::Exception When _jointName is invalid.
      public: JointState GetJointState(const std::string &_jointName) const;

      /// \brief Get the joint states.
      /// \return A vector of joint states.
      public: const std::vector<JointState> &GetJointStates() const;

      /// \brief Return true if there is a joint with the specified name.
      /// \param[in] _jointName Name of the Jointtate.
      /// \return True if the joint exists in the model.
      public: bool HasJointState(const std::string &_jointName) const;

      /// \brief Populate a state SDF element with data from the object.
      /// \param[out] _sdf SDF element to populate.
      public: void FillSDF(sdf::ElementPtr _sdf);

      /// \brief Set the wall time when this state was generated
      /// \param[in] _time The absolute clock time when the State
      /// data was recorded.
      public: virtual void SetWallTime(const common::Time &_time);

      /// \brief Set the real time when this state was generated
      /// \param[in] _time Clock time since simulation was stated.
      public: virtual void SetRealTime(const common::Time &_time);

      /// \brief Set the sim time when this state was generated
      /// \param[in] _time Simulation time when the data was recorded.
      public: virtual void SetSimTime(const common::Time &_time);

      /// \brief Assignment operator
      /// \param[in] _state State value
      /// \return this
      public: ModelState &operator=(const ModelState &_state);

      /// \brief Subtraction operator.
      /// \param[in] _pt A state to substract.
      /// \return The resulting state.
      public: ModelState operator-(const ModelState &_state) const;

      /// \brief Addition operator.
      /// \param[in] _pt A state to substract.
      /// \return The resulting state.
      public: ModelState operator+(const ModelState &_state) const;

      /// \brief Stream insertion operator.
      /// \param[in] _out output stream.
      /// \param[in] _state Model state to output.
      /// \return The stream.
      public: friend std::ostream &operator<<(std::ostream &_out,
                                     const gazebo::physics::ModelState &_state)
      {
        _out << "  <model name='" << _state.GetName() << "'>\n";
        _out << "    <pose>" << _state.pose << "</pose>\n";

        for (std::vector<LinkState>::const_iterator iter =
            _state.linkStates.begin(); iter != _state.linkStates.end();
            ++iter)
        {
          _out << *iter;
        }

        // Output the joint information
        /*for (std::vector<JointState>::const_iterator iter =
            _state.jointStates.begin(); iter != _state.jointStates.end();
            ++iter)
        {
          _out << *iter;
        }*/

        _out << "  </model>\n";

        return _out;
      }

      /// \brief Pose of the model.
      private: math::Pose pose;

      /// \brief All the link states.
      private: std::vector<LinkState> linkStates;

      /// \brief All the joint states.
      private: std::vector<JointState> jointStates;
    };
    /// \}
  }
}
#endif
