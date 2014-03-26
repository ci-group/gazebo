/*
 * Copyright 2014 Open Source Robotics Foundation
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
#ifndef _GAZEBO_VISIBLE_HH_
#define _GAZEBO_VISIBLE_HH_

/** \def GAZEBO_VISIBLE
 * Use to represent "symbol visible" if supported
 */

/** \def GAZEBO_HIDDEN
 * Use to represent "symbol hidden" if supported
 */

#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define GAZEBO_VISIBLE __attribute__ ((dllexport))
    #else
      #define GAZEBO_VISIBLE __declspec(dllexport)
    #endif
  #else
    #ifdef __GNUC__
      #define GAZEBO_VISIBLE __attribute__ ((dllimport))
    #else
      #define GAZEBO_VISIBLE __declspec(dllimport)
    #endif
  #endif
  #define GAZEBO_HIDDEN
#else
  #if __GNUC__ >= 4
    #define GAZEBO_VISIBLE __attribute__ ((visibility ("default")))
    #define GAZEBO_HIDDEN  __attribute__ ((visibility ("hidden")))
  #else
    #define GAZEBO_VISIBLE
    #define GAZEBO_HIDDEN
  #endif
#endif

#endif /* GAZEBO_VISIBLE_HH */
