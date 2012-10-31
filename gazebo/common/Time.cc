/*
 * Copyright 2011 Nate Koenig
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
/* Desc: Time class
 * Author: Nate Koenig
 * Date: 3 Apr 2007
 */

#include <sys/time.h>
#include <time.h>
#include <math.h>
#include "math/Helpers.hh"
#include "common/Time.hh"
#include "common/Console.hh"

using namespace gazebo;
using namespace common;

Time Time::wallTime;

/////////////////////////////////////////////////
Time::Time()
{
  this->sec = 0;
  this->nsec = 0;
}

/////////////////////////////////////////////////
Time::Time(const Time &_time)
: sec(_time.sec), nsec(_time.nsec)
{
}

/////////////////////////////////////////////////
Time::Time(const struct timeval &_tv)
{
  this->sec = _tv.tv_sec;
  this->nsec = _tv.tv_usec*1000;
}

/////////////////////////////////////////////////
Time::Time(const struct timespec &_tv)
{
  this->sec = _tv.tv_sec;
  this->nsec = _tv.tv_nsec;
}

/////////////////////////////////////////////////
Time::Time(int32_t _sec, int32_t _nsec)
: sec(_sec), nsec(_nsec)
{
  this->Correct();
}

/////////////////////////////////////////////////
Time::Time(double _time)
{
  this->Set(_time);
}

/////////////////////////////////////////////////
Time::~Time()
{
}

/////////////////////////////////////////////////
const Time &Time::GetWallTime()
{
  struct timespec tv;
  clock_gettime(0, &tv);
  wallTime = tv;
  return wallTime;
}

/////////////////////////////////////////////////
void Time::SetToWallTime()
{
  *this = this->GetWallTime();
}

/////////////////////////////////////////////////
void Time::Set(int32_t _sec, int32_t _nsec)
{
  this->sec = _sec;
  this->nsec = _nsec;

  this->Correct();
}

/////////////////////////////////////////////////
void Time::Set(double _seconds)
{
  this->sec = (int32_t)(floor(_seconds));
  this->nsec = (int32_t)(round((_seconds - this->sec) * 1e9));
  this->Correct();
}

/////////////////////////////////////////////////
double Time::Double() const
{
  return (static_cast<double>(this->sec) +
          static_cast<double>(this->nsec)*1e-9);
}

/////////////////////////////////////////////////
float Time::Float() const
{
  return (this->sec + this->nsec * 1e-9f);
}

/////////////////////////////////////////////////
Time Time::MSleep(unsigned int _ms)
{
  Time result;

  // get clock resolution, skip sleep if resolution is larger then
  // requested sleep tiem
  struct timespec resolution;
  clock_getres(CLOCK_REALTIME, &resolution);

  if (Time(resolution) <= Time(0, _ms*1000000))
  {
    struct timespec interval;
    struct timespec remainder;
    interval.tv_sec = _ms / 1000;
    interval.tv_nsec = (_ms % 1000) * 1000000;

    if (nanosleep(&interval, &remainder) == -1)
    {
      result.sec = remainder.tv_sec;
      result.nsec = remainder.tv_nsec;
    }
  }
  else
  {
    /// \TODO: warn user about skipping sleep because clock resolution too big
  }

  return result;
}

/////////////////////////////////////////////////
Time Time::NSleep(unsigned int _ns)
{
  Time result;

  // get clock resolution, skip sleep if resolution is larger then
  // requested sleep tiem
  struct timespec resolution;
  clock_getres(CLOCK_REALTIME, &resolution);

  if (Time(resolution) <= Time(0, _ns))
  {
    struct timespec nsleep;
    struct timespec remainder;
    nsleep.tv_sec = 0;
    nsleep.tv_nsec = _ns;

    if (clock_nanosleep(CLOCK_REALTIME, 0, &nsleep, &remainder) == -1)
    {
      result.sec = remainder.tv_sec;
      result.nsec = remainder.tv_nsec;
    }
  }
  else
  {
    /// \TODO: warn user about skipping sleep because clock resolution too big
  }

  return result;
}

/////////////////////////////////////////////////
Time Time::NSleep(Time _time)
{
  Time result;

  // get clock resolution, skip sleep if resolution is larger then
  // requested sleep tiem
  struct timespec resolution;
  clock_getres(CLOCK_REALTIME, &resolution);

  if (Time(resolution) <= _time)
  {
    struct timespec nsleep;
    struct timespec remainder;
    nsleep.tv_sec = _time.sec;
    nsleep.tv_nsec = _time.nsec;

    if (clock_nanosleep(CLOCK_REALTIME, 0, &nsleep, &remainder) == -1)
    {
      result.sec = remainder.tv_sec;
      result.nsec = remainder.tv_nsec;
    }
  }
  else
  {
    /// \TODO: warn user about skipping sleep because clock resolution too big
  }

  return result;
}

/////////////////////////////////////////////////
Time &Time::operator =(const struct timeval &_tv)
{
  this->sec = _tv.tv_sec;
  this->nsec = _tv.tv_usec*1000;

  return *this;
}

/////////////////////////////////////////////////
Time &Time::operator =(const struct timespec &_tv)
{
  this->sec = _tv.tv_sec;
  this->nsec = _tv.tv_nsec;

  return *this;
}

/////////////////////////////////////////////////
Time &Time::operator =(const Time &_time)
{
  this->sec = _time.sec;
  this->nsec = _time.nsec;

  return *this;
}

/////////////////////////////////////////////////
Time Time::operator +(const struct timeval &_tv) const
{
  Time t(this->sec + _tv.tv_sec, this->nsec + _tv.tv_usec*1000);
  t.Correct();
  return t;
}

/////////////////////////////////////////////////
Time Time::operator +(const struct timespec &_tv) const
{
  Time t(this->sec + _tv.tv_sec, this->nsec + _tv.tv_nsec);
  t.Correct();
  return t;
}

/////////////////////////////////////////////////
const Time &Time::operator +=(const struct timeval &_tv)
{
  this->sec += _tv.tv_sec;
  this->nsec += _tv.tv_usec*1000;
  this->Correct();
  return *this;
}

/////////////////////////////////////////////////
const Time &Time::operator +=(const struct timespec &_tv)
{
  this->sec += _tv.tv_sec;
  this->nsec += _tv.tv_nsec;
  this->Correct();
  return *this;
}

/////////////////////////////////////////////////
Time Time::operator +(const Time &_time) const
{
  Time t(this->sec + _time.sec, this->nsec + _time.nsec);
  t.Correct();

  return t;
}

/////////////////////////////////////////////////
const Time &Time::operator +=(const Time &_time)
{
  this->sec += _time.sec;
  this->nsec += _time.nsec;
  this->Correct();
  return *this;
}

/////////////////////////////////////////////////
Time Time::operator -(const struct timeval &_tv) const
{
  Time t(this->sec - _tv.tv_sec, this->nsec - _tv.tv_usec*1000);
  t.Correct();

  return t;
}

/////////////////////////////////////////////////
const Time &Time::operator -=(const struct timeval &_tv)
{
  this->sec -= _tv.tv_sec;
  this->nsec -= _tv.tv_usec*1000;
  this->Correct();
  return *this;
}

/////////////////////////////////////////////////
Time Time::operator -(const struct timespec &_tv) const
{
  Time t(this->sec - _tv.tv_sec, this->nsec - _tv.tv_nsec);
  t.Correct();

  return t;
}

/////////////////////////////////////////////////
const Time &Time::operator -=(const struct timespec &_tv)
{
  this->sec -= _tv.tv_sec;
  this->nsec -= _tv.tv_nsec;
  this->Correct();
  return *this;
}

/////////////////////////////////////////////////
Time Time::operator -(const Time &_time) const
{
  Time t(this->sec - _time.sec, this->nsec - _time.nsec);
  t.Correct();
  return t;
}

/////////////////////////////////////////////////
const Time &Time::operator -=(const Time &_time)
{
  this->sec -= _time.sec;
  this->nsec -= _time.nsec;
  this->Correct();
  return *this;
}

/////////////////////////////////////////////////
Time Time::operator *(const struct timeval &_tv) const
{
  Time t2(_tv.tv_sec, _tv.tv_usec*1000);
  Time t(this->Double() * t2.Double());
  t.Correct();
  return t;
}

/////////////////////////////////////////////////
const Time &Time::operator *=(const struct timeval &_tv)
{
  Time t2(_tv.tv_sec, _tv.tv_usec*1000);
  this->Set(this->Double() * t2.Double());
  this->Correct();
  return *this;
}

/////////////////////////////////////////////////
Time Time::operator *(const struct timespec &_tv) const
{
  Time t2(_tv.tv_sec, _tv.tv_nsec);
  Time t(this->Double() * t2.Double());
  t.Correct();
  return t;
}

/////////////////////////////////////////////////
const Time &Time::operator *=(const struct timespec &_tv)
{
  Time t2(_tv.tv_sec, _tv.tv_nsec);
  this->Set(this->Double() * t2.Double());
  this->Correct();
  return *this;
}

/////////////////////////////////////////////////
Time Time::operator *(const Time &_time) const
{
  Time t(this->Double() * _time.Double());
  t.Correct();
  return t;
}

/////////////////////////////////////////////////
const Time &Time::operator *=(const Time &_time)
{
  this->Set(this->Double() * _time.Double());
  this->Correct();
  return *this;
}

/////////////////////////////////////////////////
Time Time::operator /(const struct timeval &_tv) const
{
  return (*this) / Time(_tv);
}

/////////////////////////////////////////////////
const Time &Time::operator /=(const struct timeval &_tv)
{
  *this = *this / Time(_tv);
  return *this;
}

/////////////////////////////////////////////////
Time Time::operator /(const struct timespec &_tv) const
{
  return (*this) / Time(_tv);
}

/////////////////////////////////////////////////
const Time &Time::operator /=(const struct timespec &_tv)
{
  *this = *this / Time(_tv);
  return *this;
}

/////////////////////////////////////////////////
Time Time::operator /(const Time &_time) const
{
  Time result(*this);

  if (math::equal(_time.Double(), 0.0))
    gzerr << "Time divide by zero\n";
  else
    result.Set(this->Double() / _time.Double());

  return result;
}

/////////////////////////////////////////////////
const Time &Time::operator /=(const Time &_time)
{
  *this = *this / _time;
  return *this;
}

/////////////////////////////////////////////////
bool Time::operator ==(const struct timeval &_tv) const
{
  return *this == Time(_tv);
}

/////////////////////////////////////////////////
bool Time::operator ==(const struct timespec &_tv) const
{
  return *this == Time(_tv);
}

/////////////////////////////////////////////////
bool Time::operator ==(const Time &_time) const
{
  return this->sec == _time.sec && this->nsec == _time.nsec;
}

/////////////////////////////////////////////////
bool Time::operator ==(double _time) const
{
  return *this == Time(_time);
}

/////////////////////////////////////////////////
bool Time::operator!=(const struct timeval &_tv) const
{
  return !(*this == _tv);
}

/////////////////////////////////////////////////
bool Time::operator!=(const struct timespec &_tv) const
{
  return !(*this == _tv);
}

/////////////////////////////////////////////////
bool Time::operator!=(const Time &_time) const
{
  return !(*this == _time);
}

/////////////////////////////////////////////////
bool Time::operator!=(double _time) const
{
  return !(*this == _time);
}

/////////////////////////////////////////////////
bool Time::operator<(const struct timeval &_tv) const
{
  return *this < Time(_tv);
}

/////////////////////////////////////////////////
bool Time::operator<(const struct timespec &_tv) const
{
  return *this < Time(_tv);
}

/////////////////////////////////////////////////
bool Time::operator<(const Time &_time) const
{
  return this->sec < _time.sec ||
    (this->sec == _time.sec && this->nsec < _time.nsec);
}

/////////////////////////////////////////////////
bool Time::operator<(double _time) const
{
  return *this < Time(_time);
}

/////////////////////////////////////////////////
bool Time::operator<=(const struct timeval &_tv) const
{
  return *this <= Time(_tv);
}

/////////////////////////////////////////////////
bool Time::operator<=(const struct timespec &_tv) const
{
  return *this <= Time(_tv);
}

/////////////////////////////////////////////////
bool Time::operator<=(const Time &_time) const
{
  return !(_time < *this);
}

/////////////////////////////////////////////////
bool Time::operator<=(double _time) const
{
  return *this <= Time(_time);
}

/////////////////////////////////////////////////
bool Time::operator>(const struct timeval &_tv) const
{
  return *this > Time(_tv);
}

/////////////////////////////////////////////////
bool Time::operator>(const struct timespec &_tv) const
{
  return *this > Time(_tv);
}

/////////////////////////////////////////////////
bool Time::operator>(const Time &_time) const
{
  return _time < *this;
}

/////////////////////////////////////////////////
bool Time::operator>(double _time) const
{
  return *this > Time(_time);
}

/////////////////////////////////////////////////
bool Time::operator>=(const struct timeval &_tv) const
{
  return *this >= Time(_tv);
}

/////////////////////////////////////////////////
bool Time::operator>=(const struct timespec &_tv) const
{
  return *this >= Time(_tv);
}

/////////////////////////////////////////////////
bool Time::operator>=(const Time &_time) const
{
  return !(*this < _time);
}

/////////////////////////////////////////////////
bool Time::operator>=(double _time) const
{
  return *this >= Time(_time);
}
