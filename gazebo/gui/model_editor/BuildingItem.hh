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

#ifndef _BUILDING_ITEM_HH_
#define _BUILDING_ITEM_HH_

namespace gazebo
{
  namespace gui
  {
    class BuildingItem
    {
        /// \brief Constructor
        public: BuildingItem();

        /// \brief Destructor
        public: ~BuildingItem();

        /// \brief Get the level in which this building item is located
        public: int GetLevel() const;

        /// \brief Set the level of this building item
        /// \param[in] _level level number
        public: void SetLevel(int _level);

        /// \brief Get the base height of this level relative to ground plane
        public: double GetLevelBaseHeight() const;

        /// \brief Set the base height of this level relative to gound plan
        /// \param[in] _height base height
        public: void SetLevelBaseHeight(double _height);

        /// \brief Level of this building item
        protected: int level;

        /// \brief Base height of the level
        protected: double levelBaseHeight;
    };
  }
}

#endif
