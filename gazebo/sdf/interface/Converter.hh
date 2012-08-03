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
#ifndef _SDF_CONVERTER_HH_
#define _SDF_CONVERTER_HH_

#include <tinyxml.h>
#include <string>

namespace sdf
{
  /// \brief Convert from one version of SDF to another
  class Converter
  {
    public: static bool Convert(TiXmlElement *_elem,
                                const std::string &_toVersion);

    private: static void ConvertImpl(TiXmlElement *_elem,
                                     TiXmlElement *_convert);

    private: static const char *GetValue(const char *_valueElem,
                                         const char *_valueAttr,
                                         TiXmlElement *_elem);

    private: static void CheckDeprecation(TiXmlElement *_elem,
                                          TiXmlElement *_convert);
  };
}
#endif
