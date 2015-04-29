/*
 * Copyright (C) 2014-2015 Open Source Robotics Foundation
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

/// \def GAZEBO_VISIBLE
/// Use to represent "symbol visible" if supported

/// \def GAZEBO_HIDDEN
/// Use to represent "symbol hidden" if supported

#if defined BUILDING_STATIC_LIBS
  #define GAZEBO_VISIBLE
  #define GZ_COMMON_VISIBLE
  #define GZ_MATH_VISIBLE
  #define GZ_TRANSPORT_VISIBLE
  #define GZ_MSGS_VISIBLE
  #define GZ_SELECTION_BUFFER_VISIBLE
  #define GZ_RENDERING_VISIBLE
  #define GZ_UTIL_VISIBLE
  #define GZ_RENDERING_DEFERRED_VISIBLE
  #define GZ_PHYSICS_VISIBLE
  #define GZ_PHYSICS_ODE_VISIBLE
  #define GZ_GUI_VISIBLE
  #define GZ_GUI_BUILDING_VISIBLE
  #define GZ_GUI_MODEL_VISIBLE
  #define GZ_GUI_TERRAIN_VISIBLE
  #define GZ_GUI_VIEWERS_VISIBLE
  #define GAZEBO_HIDDEN
#else
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

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_COMMON
      #ifdef __GNUC__
        #define GZ_COMMON_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_COMMON_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_COMMON_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_COMMON_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_COMMON_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_COMMON_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_COMMON_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_COMMON_VISIBLE
      #define GZ_COMMON_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_MATH
      #ifdef __GNUC__
       #define GZ_MATH_VISIBLE __attribute__ ((dllexport))
      #else
       #define GZ_MATH_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_MATH_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_MATH_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_MATH_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_MATH_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_MATH_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_MATH_VISIBLE
      #define GZ_MATH_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_TRANSPORT
      #ifdef __GNUC__
        #define GZ_TRANSPORT_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_TRANSPORT_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
       #define GZ_TRANSPORT_VISIBLE __attribute__ ((dllimport))
      #else
       #define GZ_TRANSPORT_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_TRANSPORT_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_TRANSPORT_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_TRANSPORT_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_TRANSPORT_VISIBLE
      #define GZ_TRANSPORT_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_MSGS
      #ifdef __GNUC__
       #define GZ_MSGS_VISIBLE __attribute__ ((dllexport))
      #else
       #define GZ_MSGS_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_MSGS_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_MSGS_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_MSGS_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_MSGS_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_MSGS_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_MSGS_VISIBLE
      #define GZ_MSGS_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_SELECTION_BUFF
      #ifdef __GNUC__
        #define GZ_SELECTION_BUFF_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_SELECTION_BUFF_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_SELECTION_BUFF_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_SELECTION_BUFF_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_SELECTION_BUFF_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_SELECTION_BUFF_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_SELECTION_BUFF_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_SELECTION_BUFF_VISIBLE
      #define GZ_SELECTION_BUFF_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_RENDERING
      #ifdef __GNUC__
        #define GZ_RENDERING_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_RENDERING_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_RENDERING_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_RENDERING_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_RENDERING_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_RENDERING_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_RENDERING_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_RENDERING_VISIBLE
      #define GZ_RENDERING_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_UTIL
      #ifdef __GNUC__
        #define GZ_UTIL_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_UTIL_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_UTIL_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_UTIL_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_UTIL_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_UTIL_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_UTIL_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_UTIL_VISIBLE
      #define GZ_UTIL_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_RENDERING_DEFERRED
      #ifdef __GNUC__
        #define GZ_RENDERING_DEFERRED_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_RENDERING_DEFERRED_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_RENDERING_DEFERRED_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_RENDERING_DEFERRED_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_RENDERING_DEFERRED_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_RENDERING_DEFERRED_VISIBLE __attribute__ ((visibility (\
              "default")))
      #define GZ_RENDERING_DEFERRED_HIDDEN __attribute__ ((visibility (\
              "hidden")))
    #else
      #define GZ_RENDERING_DEFERRED_VISIBLE
      #define GZ_RENDERING_DEFERRED_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_PHYSICS
      #ifdef __GNUC__
        #define GZ_PHYSICS_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_PHYSICS_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_PHYSICS_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_PHYSICS_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_PHYSICS_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_PHYSICS_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_PHYSICS_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_PHYSICS_VISIBLE
      #define GZ_PHYSICS_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_PHYSICS_ODE
      #ifdef __GNUC__
        #define GZ_PHYSICS_ODE_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_PHYSICS_ODE_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_PHYSICS_ODE_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_PHYSICS_ODE_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_PHYSICS_ODE_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_PHYSICS_ODE_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_PHYSICS_ODE_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_PHYSICS_ODE_VISIBLE
      #define GZ_PHYSICS_ODE_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_GUI
      #ifdef __GNUC__
        #define GZ_GUI_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_GUI_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_GUI_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_GUI_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_GUI_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_GUI_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_GUI_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_GUI_VISIBLE
      #define GZ_GUI_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_GUI_BUILDING
      #ifdef __GNUC__
        #define GZ_GUI_BUILDING_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_GUI_BUILDING_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_GUI_BUILDING_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_GUI_BUILDING_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_GUI_BUILDING_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_GUI_BUILDING_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_GUI_BUILDING_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_GUI_BUILDING_VISIBLE
      #define GZ_GUI_BUILDING_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_GUI_MODEL
      #ifdef __GNUC__
        #define GZ_GUI_MODEL_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_GUI_MODEL_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_GUI_MODEL_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_GUI_MODEL_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_GUI_MODEL_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_GUI_MODEL_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_GUI_MODEL_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_GUI_MODEL_VISIBLE
      #define GZ_GUI_MODEL_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_GUI_TERRAIN
      #ifdef __GNUC__
        #define GZ_GUI_TERRAIN_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_GUI_TERRAIN_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_GUI_TERRAIN_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_GUI_TERRAIN_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_GUI_TERRAIN_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_GUI_TERRAIN_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_GUI_TERRAIN_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_GUI_TERRAIN_VISIBLE
      #define GZ_GUI_TERRAIN_HIDDEN
    #endif
  #endif

  #if defined _WIN32 || defined __CYGWIN__
    #ifdef BUILDING_DLL_GZ_GUI_VIEWERS
      #ifdef __GNUC__
        #define GZ_GUI_VIEWERS_VISIBLE __attribute__ ((dllexport))
      #else
        #define GZ_GUI_VIEWERS_VISIBLE __declspec(dllexport)
      #endif
    #else
      #ifdef __GNUC__
        #define GZ_GUI_VIEWERS_VISIBLE __attribute__ ((dllimport))
      #else
        #define GZ_GUI_VIEWERS_VISIBLE __declspec(dllimport)
      #endif
    #endif
    #define GZ_GUI_VIEWERS_HIDDEN
  #else
    #if __GNUC__ >= 4
      #define GZ_GUI_VIEWERS_VISIBLE __attribute__ ((visibility ("default")))
      #define GZ_GUI_VIEWERS_HIDDEN  __attribute__ ((visibility ("hidden")))
    #else
      #define GZ_GUI_VIEWERS_VISIBLE
      #define GZ_GUI_VIEWERS_HIDDEN
    #endif
  #endif
// BUILDING_STATIC_LIBS
#endif

// _GAZEBO_VISIBLE_HH_
#endif
