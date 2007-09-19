/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003  
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* Desc: Gazebo input handler
 * Author: Nate Koenig
 * Date: 17 Sep 2007
 * SVN: $Id:$
 */

#ifndef INPUTHANDLER_HH
#define INPUTHANDLER_HH

#include <map>

#include "Vector3.hh"
#include "Vector2.hh"
#include "SingletonT.hh"

namespace gazebo
{
  class InputEvent;

  /// \brief Input Handler
  class InputHandler : public SingletonT<InputHandler>
  {
    /// \brief Constructor
    public: InputHandler ();
  
    /// \brief Destructor
    public: virtual ~InputHandler();
  
    /// \brief Handle an input event
    public: void HandleEvent( const InputEvent *event );

    public: void Update();

    private: void HandleKeyPress( const InputEvent *event );
    private: bool HandleKeyRelease( const InputEvent *event );

    /// \brief Handle a drag event
    private: void HandleDrag( const InputEvent *event );

    private: friend class DestroyerT<InputHandler>;
    private: friend class SingletonT<InputHandler>;

    private: float moveAmount;
    private: float moveScale;
    private: float rotateAmount;

    private: Vector3 directionVec;

    private: bool leftMousePressed;
    private: bool rightMousePressed;
    private: bool middleMousePressed;
    private: Vector2<int> prevMousePos;
    private: std::map<int,int> keys;
  };
  
}
#endif
