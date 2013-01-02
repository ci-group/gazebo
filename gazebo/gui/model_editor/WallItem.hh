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

#ifndef _WALL_ITEM_HH_
#define _WALL_ITEM_HH_

#include "gazebo/gui/qt.h"

namespace gazebo
{
  namespace gui
  {
    class PolylineItem;

    class CornerGrabber;

    class LineSegmentItem;

    class BuildingItem;

    class WallItem : public PolylineItem, public BuildingItem
    {
        public: WallItem(QPointF _start, QPointF _end);

        public: ~WallItem();

        public: double GetHeight();

        public: void SetHeight(double _height);

        public: WallItem *Clone();

        public: void Update();

        private: bool cornerEventFilter(CornerGrabber *_corner,
            QEvent *_event);

        private: bool segmentEventFilter(LineSegmentItem *_segment,
            QEvent *_event);

        private: void WallChanged();

        private: double wallThickness;

        private: double wallHeight;

        private: double scale;
    };
  }
}

#endif
