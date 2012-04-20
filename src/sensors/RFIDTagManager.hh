/* Copyright (C)
 *     Jonas Mellin & Zakiruz Zaman
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
 */
/* Desc: RFID Tag Manager
 * Author: Jonas Mellin & Zakiruz Zaman 
 * Date: 6th December 2011
 */

#ifndef __RFID_TAG_MANAGER__
#define __RFID_TAG_MANAGER__

#include <vector>
#include "common/SingletonT.hh"
#include "physics/physics.h"
#include "RFIDTag.hh"

namespace gazebo
{
  namespace sensors
  {
    class RFIDTagManager : public SingletonT<RFIDTagManager>
    {
      public: void AddTaggedModel(RFIDTag *_model);

      public: std::vector<RFIDTag*> GetTags()
              {return this->taggedModels;}

      private: std::vector<RFIDTag*> taggedModels;

      private: friend class SingletonT<RFIDTagManager>;
    };
  }
}
#endif
