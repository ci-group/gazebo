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
/* Desc: Singleton base class
 * Author: Nate Koenig
 * Date: 2 Sept 2007
 */

#ifndef SINGLETONT_HH
#define SINGLETONT_HH

/// \addtogroup gazebo_common Common
/// \{
/// \brief Singleton class
template <class T>
class SingletonT
{
  /// \brief Get an instance of the singleton
  public: static T *Instance()
          {
            return &GetInstance();
          }

  /// \brief Constructor
  protected: SingletonT() {}
  /// \brief Destructor
  protected: virtual ~SingletonT() {}
  private: static T &GetInstance()
           {
             static T t;
             return static_cast<T &>(t);
           }

  private: static T &myself;
};

template <class T>
T &SingletonT<T>::myself = SingletonT<T>::GetInstance();
/// \}

#endif


