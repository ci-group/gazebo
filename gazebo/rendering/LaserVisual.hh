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
/* Desc: Laser Visualization Class
 * Author: Nate Koenig
 * Date: 14 Dec 2007
 */

#ifndef _LASERVISUAL_HH_
#define _LASERVISUAL_HH_

#include <string>

#include "rendering/Visual.hh"
#include "msgs/MessageTypes.hh"
#include "transport/TransportTypes.hh"

namespace gazebo
{
  namespace rendering
  {
    /// \addtogroup gazebo_rendering
    /// \{

    class DynamicLines;

    /// \class LaserVisual LaserVisual.hh rendering/rendering.hh
    /// \brief Visualization for laser data.
    class LaserVisual : public Visual
    {
      /// \brief Constructor.
      /// \param[in] _name Name of the visual.
      /// \param[in] _vis Pointer to the parent Visual.
      /// \param[in] _topicName Name of the topic that has laser data.
      public: LaserVisual(const std::string &_name, VisualPtr _vis,
                          const std::string &_topicName);

      /// \brief Destructor.
      public: virtual ~LaserVisual();

      // Documentation inherited from parent.
      public: virtual void SetEmissive(const common::Color &_color);

      /// \brief Callback when laser data is received.
      private: void OnScan(ConstLaserScanPtr &_msg);

      /// \brief Pointer to a node that handles communication.
      private: transport::NodePtr node;

      /// \brief Subscription to the laser data.
      private: transport::SubscriberPtr laserScanSub;

      /// \brief Renders the laser data.
      private: DynamicLines *rayFan;
    };
    /// \}
  }
}
#endif
