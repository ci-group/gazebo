/*
 * Copyright 2011 Nate Koenig & Andrew Howard
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
#include <math.h>
#include "math/Helpers.hh"
#include "common/Time.hh"

using namespace gazebo;
using namespace common;


Time Time::wallTime;

// Constructors
Time::Time()
{
  this->sec = 0;
  this->nsec = 0;
}

  Time::Time(const Time &_time)
: sec(_time.sec), nsec(_time.nsec)
{
}

Time::Time(const struct timeval &_tv)
{
  this->sec = _tv.tv_sec;
  this->nsec = _tv.tv_usec*1000;
}

  Time::Time(int32_t _sec, int32_t _nsec)
: sec(_sec), nsec(_nsec)
{
  this->Correct();
}

Time::Time(double _time)
{
  this->Set(_time);
}

// Destructor
Time::~Time()
{
}

const Time &Time::GetWallTime()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  wallTime = tv;
  return wallTime;
}

void Time::SetToWallTime()
{
  *this = this->GetWallTime();
}

void Time::Set(int32_t _sec, int32_t _nsec)
{
  this->sec = _sec;
  this->nsec = _nsec;

  this->Correct();
}

void Time::Set(double _seconds)
{
  this->sec = (int32_t)(floor(_seconds));
  this->nsec = (int32_t)(round((_seconds - this->sec) * 1e9));
  this->Correct();
}

double Time::Double() const
{
  return (static_cast<double>(this->sec) +
          static_cast<double>(this->nsec)*1e-9);
}

/// Millisecond sleep
Time Time::MSleep(unsigned int _ms)
{
  Time result;

  struct timespec interval;
  struct timespec remainder;
  interval.tv_sec = _ms / 1000;
  interval.tv_nsec = (_ms % 1000) * 1000000;

  if (nanosleep(&interval, &remainder) == -1)
  {
    result.sec = remainder.tv_sec;
    result.nsec = remainder.tv_nsec;
  }

  return result;
}

// Equal opeators
Time &Time::operator =(const struct timeval &tv)
{
  this->sec = tv.tv_sec;
  this->nsec = tv.tv_usec*1000;

  return *this;
}

Time &Time::operator =(const Time &time)
{
  this->sec = time.sec;
  this->nsec = time.nsec;

  return *this;
}

// Addition operators
Time Time::operator +(const struct timeval &tv) const
{
  Time t(this->sec + tv.tv_sec, this->nsec + tv.tv_usec*1000);
  t.Correct();
  return t;
}

const Time &Time::operator +=(const struct timeval &tv)
{
  this->sec += tv.tv_sec;
  this->nsec += tv.tv_usec*1000;
  this->Correct();
  return *this;
}

Time Time::operator +(const Time &time) const
{
  Time t(this->sec + time.sec, this->nsec + time.nsec);
  t.Correct();

  return t;
}

const Time &Time::operator +=(const Time &time)
{
  this->sec += time.sec;
  this->nsec += time.nsec;
  this->Correct();
  return *this;
}

// Subtraction operators
Time Time::operator -(const struct timeval &tv) const
{
  Time t(this->sec-tv.tv_sec, this->nsec-tv.tv_usec*1000);
  t.Correct();

  return t;
}

const Time &Time::operator -=(const struct timeval &tv)
{
  this->sec -= tv.tv_sec;
  this->nsec -= tv.tv_usec*1000;
  this->Correct();
  return *this;
}

Time Time::operator -(const Time &time) const
{
  Time t(this->sec-time.sec, this->nsec-time.nsec);
  t.Correct();
  return t;
}

const Time &Time::operator -=(const Time &time)
{
  this->sec -= time.sec;
  this->nsec -= time.nsec;
  this->Correct();
  return *this;
}

// Multiplication operators
Time Time::operator *(const struct timeval &tv) const
{
  Time t(this->sec * tv.tv_sec, this->nsec * tv.tv_usec*1000);
  t.Correct();
  return t;
}

const Time &Time::operator *=(const struct timeval &tv)
{
  this->sec *= tv.tv_sec;
  this->nsec *= tv.tv_usec*1000;
  this->Correct();
  return *this;
}

Time Time::operator *(const Time &time) const
{
  Time t(this->sec * time.sec, this->nsec * time.nsec);
  t.Correct();
  return t;
}

const Time &Time::operator *=(const Time &time)
{
  this->sec *= time.sec;
  this->nsec *= time.nsec;
  this->Correct();
  return *this;
}

// Division operators
Time Time::operator /(const struct timeval &_tv) const
{
  return (*this) / Time(_tv);
}

const Time &Time::operator /=(const struct timeval &_tv)
{
  *this = *this / Time(_tv);
  return *this;
}

Time Time::operator /(const Time &_time) const
{
  Time result(*this);
  double remainder = 0.0;

  if (_time.sec != 0)
  {
    result.sec = this->sec / _time.sec;
    remainder = (this->sec / static_cast<double>(_time.sec)) - result.sec;
  }

  if (_time.nsec != 0)
    result.nsec = this->nsec / _time.nsec;

  result.nsec += remainder * 1e9;

  return result;
}

const Time &Time::operator /=(const Time &_time)
{
  *this = *this / _time;
  return *this;
}

// Equality operators
bool Time::operator ==(const struct timeval &tv) const
{
  return *this == Time(tv);
}

bool Time::operator ==(const Time &time) const
{
  return this->sec == time.sec && this->nsec == time.nsec;
}

bool Time::operator ==(double time) const
{
  return *this == Time(time);
}

bool Time::operator!=(const struct timeval &tv) const
{
  return !(*this == tv);
}

bool Time::operator!=(const Time &time) const
{
  return !(*this == time);
}

bool Time::operator!=(double time) const
{
  return !(*this == time);
}

bool Time::operator<(const struct timeval &tv) const
{
  return *this < Time(tv);
}

bool Time::operator<(const Time &time) const
{
  return this->sec < time.sec ||
    (this->sec == time.sec && this->nsec < time.nsec);
}

bool Time::operator<(double time) const
{
  return *this < Time(time);
}

bool Time::operator<=(const struct timeval &tv) const
{
  return *this <= Time(tv);
}

bool Time::operator<=(const Time &time) const
{
  return !(time < *this);
}

bool Time::operator<=(double time) const
{
  return *this <= Time(time);
}

bool Time::operator>(const struct timeval &tv) const
{
  return *this > Time(tv);
}

bool Time::operator>(const Time &time) const
{
  return time < *this;
}

bool Time::operator>(double time) const
{
  return *this > Time(time);
}

bool Time::operator>=(const struct timeval &tv) const
{
  return *this >= Time(tv);
}

bool Time::operator>=(const Time &time) const
{
  return !(*this < time);
}

bool Time::operator>=(double time) const
{
  return *this >= Time(time);
}


