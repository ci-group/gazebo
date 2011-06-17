/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
* 
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
* 
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Wim Meeussen */

#ifndef URDF_PARSER_H
#define URDF_PARSER_H

#include <string>
#include <map>
#include <boost/shared_ptr.hpp>

#include "sensor.h"
#include "plugin.h"

namespace sdf
{
  bool initXml(TiXmlElement *_config, boost::shared_ptr<Sensor> &_sensor);

  bool initXml(TiXmlElement *_config, boost::shared_ptr<Contact> &_sensor) {return true;}

  bool initXml(TiXmlElement *_config, boost::shared_ptr<Camera> &_sensor);
  bool initXml(TiXmlElement *_config, boost::shared_ptr<Material> &_material);
  bool initXml(TiXmlElement *_config, boost::shared_ptr<Inertial> &_inertial);



  /* JOHN below here */


  bool getBoolFromStr(std::string _str, bool &_value);
 
  bool getDoubleFromStr(const std::string &_str, double &_value);
 
  bool getIntFromStr(const std::string &_str, int &_value);
    
  bool getUIntFromStr(const std::string &_str, unsigned int &_value);

  bool getPlugins(TiXmlElement *pluginXml, std::map<std::string, 
                  boost::shared_ptr<Plugin> > &plugins);
}

#endif
