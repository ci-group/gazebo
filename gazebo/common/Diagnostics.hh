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
/* Desc: A diagnostic class
 * Author: Nate Koenig
 * Date: 2 Feb 2011
 */

#ifndef _DIAGNOSTICMANAGER_HH_
#define _DIAGNOSTICMANAGER_HH_

#include <map>
#include <string>

#include "common/SingletonT.hh"
#include "common/Timer.hh"

namespace gazebo
{
  namespace common
  {
    /// \addtogroup gazebo_common Common
    /// \{

    /// \brief Create an instance of common::DiagnosticManager
    #define DIAG_TIMER(name) DiagnosticManager::Instance()->CreateTimer(name);

    class DiagnosticTimer;
    typedef boost::shared_ptr< DiagnosticTimer > DiagnosticTimerPtr;

    /// \brief A diagnostic manager class
    class DiagnosticManager : public SingletonT<DiagnosticManager>
    {
      /// \brief Constructor
      private: DiagnosticManager();

      /// \brief Destructor
      private: virtual ~DiagnosticManager();

      /// \brief Create a new timer instance
      /// \param[in] _name Name of the timer.
      /// \return A pointer to the new diagnostic timer
      public: DiagnosticTimerPtr CreateTimer(const std::string &_name);

      /// \brief A diagnostic timer has started
      /// \param[in] _timer The timer to start
      public: void TimerStart(DiagnosticTimer *_timer);

      /// \brief A diagnostic timer has stopped
      /// \param[in] _time The timer to stop
      public: void TimerStop(DiagnosticTimer *_timer);

      /// \brief Get the number of timers
      /// \return The number of timers
      public: int GetTimerCount() const;

      /// \brief Get the time of a timer instance
      /// \param[in] _index The index of a timer instance
      /// \return Time of the specified timer
      public: Time GetTime(int _index) const;

      /// \brief Get a time based on a label
      /// \param[in] _label Name of the timer instance
      /// \return Time of the specified timer
      public: Time GetTime(const std::string &_label) const;

      /// \brief Get a label for a timer
      /// \param[in] _index Index of a timer instance
      /// \return Label of the specified timer
      public: std::string GetLabel(int _index) const;

      /// \brief Set whether timers are enabled
      /// \param[in] _e True = timers are enabled
      public: void SetEnabled(bool _e) {this->enabled = _e;}

      /// \brief Get whether the timers are enabled
      /// \return TRue if the timers are enabled
      public: inline bool GetEnabled() const {return this->enabled;}

      /// \brief always false, has not effect
      private: bool enabled;

      /// \brief dictionary of timers index by name
      private: std::map<std::string, Time> timers;

      // Singleton implementation
      private: friend class SingletonT<DiagnosticManager>;
    };

    /// \brief A timer designed for diagnostics
    class DiagnosticTimer : public Timer
    {
      /// \brief Constructor
      /// \param[in] _name Name of the timer
      public: DiagnosticTimer(const std::string &_name) : Timer()
              {
                this->Start();
                this->name = _name;
                this->diagManager->TimerStart(this);
              }

      /// \brief Destructor
      public: virtual ~DiagnosticTimer()
              {
                this->diagManager->TimerStop(this);
              }

      /// \brief Get the name of the timer
      /// \return The name of timer
      public: inline const std::string GetName() const
              { return this->name; }

      /// \brief not used
      private: std::string name;

      /// \brief singleton instance
      private: static DiagnosticManager *diagManager;
    };
    /// \}
  }
}
#endif
