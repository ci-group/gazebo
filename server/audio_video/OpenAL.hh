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
/* Desc: OpenAL Server
 * Author: Nathan Koenig
 * Date: 20 Jan 2008
 * SVN: $Id:$
 */

#ifndef OPENAL_HH
#define OPENAL_HH

#include <stdint.h>
#include <string>

#include "Vector3.hh"

struct ALCcontext_struct;
struct ALCdevice_struct;

namespace gazebo
{

  class OpenALIface;
  class XMLConfigNode;
  class OpenALSource;
  
  class OpenAL
  {
    /// \brief Constructor
    private: OpenAL();
  
    /// \brief Destructor
    private: virtual ~OpenAL();

    /// \brief Load the OpenAL server
    public: void Load(XMLConfigNode *node);

    // \brief Get an instance of this class
    public: static OpenAL *Instance();
  
    /// \brief Initialize
    public: void Init();

    /// \brief Finalize
    public: void Fini();

    /// \brief Create an OpenALSource object from XML node
    /// \param node The XML config node containing the auiod information
    /// \return A pointer to an OpenALSource object
    public: OpenALSource *CreateSource( XMLConfigNode *node );

    /// \brief Set the listener position
    public: void SetListenerPos( const Vector3 pos );
  
    /// \brief Set the listener velocity
    public: void SetListenerVel( const Vector3 vel );
  
    /// \brief Set the listener orientation
    public: void SetListenerOrient(float cx, float cy, float cz,
                                        float ux, float uy, float uz);

    private: ALCcontext_struct *context;
    private: ALCdevice_struct *audioDevice;
  
    private: static OpenAL *myself;
  };

  /// \brief OpenAL Source. This can be thought of as a speaker.
  class OpenALSource
  {
    /// \brief Constructor
    public: OpenALSource();

    /// \brief Destructor
    public: virtual ~OpenALSource();

    /// \brief Load the source from xml
    public: void Load(XMLConfigNode *node);

    /// \brief Set the position of the source
    public: int SetPos(const Vector3 &pos);
  
    /// \brief Set the velocity of the source
    public: int SetVel(const Vector3 &vel);
  
    /// \brief Set the pitch of the source
    public: int SetPitch(float p);
  
    /// \brief Set the pitch of the source
    public: int SetGain(float g);
  
    /// \brief Set whether the source loops the audio 
    public: int SetLoop(bool state);
 
    /// \brief Play a sound
    public: void Play();

    /// \brief Pause a sound
    public: void Pause();

    /// \brief Stop a sound
    public: void Stop();

    /// \brief Rewind the sound to the beginning
    public: void Rewind();

    /// \brief Is the audio playing
    public: bool IsPlaying();

    /// \brief Fill the OpenAL audio buffer from PCM data
    public: void FillBufferFromPCM(uint8_t *pcmData, unsigned int dataCount, 
                                   int sampleRate );

    /// \brief Fill the OpenAL audio buffer with data from a sound file
    public: void FillBufferFromFile( const std::string &audioFile );

    // OpenAL source index
    private: unsigned int alSource;

    // OpenAL buffer index
    private: unsigned int alBuffer;
  };
}

#endif
