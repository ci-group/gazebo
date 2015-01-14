/*
 * Copyright 2015 Open Source Robotics Foundation
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

#ifndef _PART_COLLISION_TAB_HH_
#define _PART_COLLISION_TAB_HH_

#include <string>

#include "gazebo/math/Pose.hh"

#include "gazebo/gui/qt.h"
#include "gazebo/gui/model/ModelData.hh"

namespace gazebo
{

  namespace gui
  {
    class ConfigWidget;

    /// \addtogroup gazebo_gui
    /// \{

    /// \class CollisionConfigData PartCollisionConfig.hh
    /// \brief A class of widgets used for configuring collision properties.
    class CollisionConfigData
    {
      /// \brief Unique ID of this collision config.
      public: int id;

      /// \brief Name of the collision.
      public: std::string name;

      /// \brief config widget for configuring collision properties.
      public: ConfigWidget *configWidget;

      /// \brief Tree item associated with the configWidget.
      public: QTreeWidgetItem *treeItem;
    };

    /// \class PartCollisionConfig PartCollisionConfig.hh
    /// \brief A tab for configuring collision properties of a part.
    class PartCollisionConfig : public QWidget
    {
      Q_OBJECT

      /// \brief Constructor
      public: PartCollisionConfig();

      /// \brief Destructor
      public: ~PartCollisionConfig();

      /// \brief Add a collision widget to the tab.
      /// \param[in] _name Name of collision added.
      public: void AddCollision(const std::string &_name,
          const msgs::Collision *_collisionMsg = NULL);

      /// \brief Update a collision widget from a collision msg.
      /// \param[in] _name Name of collision to be updated.
      /// \param[in] _collisionMsg Msg used to update the collision widget
      /// values.
      public: void UpdateCollision(const std::string &_name,
          const msgs::Collision *_collisionMsg);

      /// \brief Reset the collision tab.
      public: void Reset();

      /// \brief Get the number of collisions.
      /// \return Number of collisions.
      public: unsigned int GetCollisionCount() const;

      /// \brief Get the msg containing all collision data.
      /// \param[in] _name Name of collision.
      /// \return Collision msg.
      public: msgs::Collision *GetData(const std::string &_name) const;

      /// \brief Set the geometry size a collision
      /// \param[in] _size Size to set the geometry to.
      public: void SetGeometrySize(const std::string &_name,
          const math::Vector3 &_size);

      /// \brief Qt signal emitted when a collision is removed.
      /// \param[in] _name Name of collision removed.
      Q_SIGNALS: void CollisionRemoved(const std::string &_name);

      /// \brief Qt signal emitted when a collision is added.
      Q_SIGNALS: void CollisionAdded(const std::string &_name);

      /// \brief Qt callback when a collision is to be added.
      /// \param[in] _name Name of collision added.
      private slots: void OnAddCollision();

      /// \brief Qt callback when a collision is to be removed.
      /// \param[in] _item Item to be removed.
      private slots: void OnRemoveCollision(int);

      /// \brief Received item selection user input.
      /// \param[in] _item Item selected.
      /// \param[in] _column Column index.
      private slots: void OnItemSelection(QTreeWidgetItem *_item, int _column);

      /// \brief Map of id to collision config widget.
      private: std::map<int, CollisionConfigData *> configs;

      /// \brief Widget that display collisions' properties.
      private: QTreeWidget *collisionsTreeWidget;

      /// \brief Counter for the number of collisions.
      private: int counter;

      /// \brief Qt signal mapper for mapping remove button signals.
      private:  QSignalMapper *signalMapper;

      /// \brief A map of collision items to their id.
      private: std::map<int, QTreeWidgetItem *> collisionItems;
    };
    /// \}
  }
}
#endif
