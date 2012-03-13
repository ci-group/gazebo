/*
 * Copyright 2011 Nate Koenig & Andrew Howard
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
#ifndef GAZEBO_CEGUI_H_
#define GAZEBO_CEGUI_H_

// This disables warning messages for OGRE
#pragma GCC system_header

#include "gazebo_config.h"

#ifdef HAVE_CEGUI
#include "CEGUI/CEGUI.h"
#include "CEGUI/CEGUIEventArgs.h"
#include "CEGUI/RendererModules/Ogre/CEGUIOgreRenderer.h"
#endif

#endif
