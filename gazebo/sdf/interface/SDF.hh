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
#ifndef SDF_HH
#define SDF_HH

#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "sdf/interface/Param.hh"

#define SDF_VERSION "1.0"

namespace sdf
{
  class SDF;
  class Element;
  typedef boost::shared_ptr<SDF> SDFPtr;
  typedef boost::shared_ptr<Element> ElementPtr;
  typedef std::vector< ElementPtr > ElementPtr_V;

  /// \brief SDF Element class
  class Element : public boost::enable_shared_from_this<Element>
  {
    public: Element();
    public: virtual ~Element();

    public: boost::shared_ptr<Element> Clone() const;

    /// \brief Copy values from an Element
    public: void Copy(const ElementPtr _elem);

    public: ElementPtr GetParent() const;
    public: void SetParent(const ElementPtr _parent);

    public: void SetName(const std::string &_name);
    public: const std::string &GetName() const;

    public: void SetRequired(const std::string &_req);
    public: const std::string &GetRequired() const;

    public: void SetCopyChildren(bool _value);
    public: bool GetCopyChildren() const;

    public: void PrintDescription(std::string _prefix);
    public: void PrintValues(std::string _prefix);

    private: void ToString(const std::string &_prefix,
                           std::ostringstream &_out) const;
    public: std::string ToString(const std::string &_prefix) const;

    public: void AddAttribute(const std::string &_key,
                              const std::string &_type,
                              const std::string &_defaultvalue,
                              bool _required);

    public: void AddValue(const std::string &_type,
                          const std::string &_defaultValue, bool _required);

    /// \brief Get the param of an attribute.
    /// \param _key the name of the attribute
    public: ParamPtr GetAttribute(const std::string &_key);

    /// \brief Get the number of attributes
    public: unsigned int GetAttributeCount() const;

    /// \brief Get an attribute using an index
    public: ParamPtr GetAttribute(unsigned int _index) const;

    /// \brief Get the number of element descriptions
    public: unsigned int GetElementDescriptionCount() const;

    /// \brief Get an element description using an index
    public: ElementPtr GetElementDescription(unsigned int _index) const;

    /// \brief Get an element descriptio using a key
    public: ElementPtr GetElementDescription(const std::string &_key) const;

    /// \brief Return true if an element description exists
    public: bool HasElementDescription(const std::string &_name);

    public: bool HasAttribute(const std::string &_key);

    /// \brief Return true if the attribute was set (i.e. not default value)
    public: bool GetAttributeSet(const std::string &_key);

    /// \brief Get the param of the elements value
    public: ParamPtr GetValue();

    public: bool GetValueBool(const std::string &_key = "");
    public: int GetValueInt(const std::string &_key = "");
    public: float GetValueFloat(const std::string &_key = "");
    public: double GetValueDouble(const std::string &_key = "");
    public: unsigned int GetValueUInt(const std::string &_key = "");
    public: char GetValueChar(const std::string &_key = "");
    public: std::string GetValueString(const std::string &_key = "");
    public: gazebo::math::Vector3 GetValueVector3(const std::string &_key = "");
    public: gazebo::math::Vector2d GetValueVector2d(
                const std::string &_key = "");
    public: gazebo::math::Quaternion GetValueQuaternion(
                const std::string &_key = "");
    public: gazebo::math::Pose GetValuePose(const std::string &_key = "");
    public: gazebo::common::Color GetValueColor(const std::string &_key = "");
    public: gazebo::common::Time GetValueTime(const std::string &_key = "");

    public: bool Set(const bool &_value);
    public: bool Set(const int &_value);
    public: bool Set(const unsigned int &_value);
    public: bool Set(const float &_value);
    public: bool Set(const double &_value);
    public: bool Set(const char &_value);
    public: bool Set(const std::string &_value);
    public: bool Set(const char *_value);
    public: bool Set(const gazebo::math::Vector3 &_value);
    public: bool Set(const gazebo::math::Vector2i &_value);
    public: bool Set(const gazebo::math::Vector2d &_value);
    public: bool Set(const gazebo::math::Quaternion &_value);
    public: bool Set(const gazebo::math::Pose &_value);
    public: bool Set(const gazebo::common::Color &_value);
    public: bool Set(const gazebo::common::Time &_value);

    public: bool HasElement(const std::string &_name) const;

    public: ElementPtr GetElement(const std::string &_name) const;
    public: ElementPtr GetFirstElement() const;

    public: ElementPtr GetNextElement(const std::string &_name = "") const;

    public: ElementPtr GetOrCreateElement(const std::string &_name);
    public: ElementPtr AddElement(const std::string &_name);
    public: void InsertElement(ElementPtr _elem);
    public: void ClearElements();

    public: void Update();
    public: void Reset();

    public: void SetInclude(const std::string &_filename);
    public: std::string GetInclude() const;

    /// \brief Add a new element description
    public: void AddElementDescription(ElementPtr _elem);

    private: boost::shared_ptr<Param> CreateParam(const std::string &_key,
                 const std::string &_type, const std::string &_defaultValue,
                 bool _required);

    private: std::string name;
    private: std::string required;
    private: bool copyChildren;

    private: ElementPtr parent;

    // Attributes of this element
    private: Param_V attributes;

    // Value of this element
    private: ParamPtr value;

    // The existing child elements
    private: ElementPtr_V elements;

    // The possible child elements
    private: ElementPtr_V elementDescriptions;

    /// name of the include file that was used to create this element
    private: std::string includeFilename;
  };


  /// \brief Base SDF class
  class SDF
  {
    public: SDF();
    public: void PrintDescription();
    public: void PrintValues();
    public: void Write(const std::string &_filename);
    public: std::string ToString() const;

    /// \brief Set SDF values from a string
    public: void SetFromString(const std::string &_sdfData);

    public: ElementPtr root;
  };
}
#endif
