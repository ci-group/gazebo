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
#include "common/Color.hh"
#include "math/Pose.hh"
#include "math/Vector3.hh"
#include "math/Vector2d.hh"

#include "sdf/interface/parser.hh"
#include "sdf/interface/SDF.hh"

using namespace sdf;

/////////////////////////////////////////////////
Element::Element()
{
  this->copyChildren = false;
}

/////////////////////////////////////////////////
Element::~Element()
{
  this->parent.reset();
  for (Param_V::iterator iter = this->attributes.begin();
      iter != this->attributes.end(); ++iter)
  {
    (*iter).reset();
  }
  this->attributes.clear();

  for (ElementPtr_V::iterator iter = this->elements.begin();
      iter != this->elements.end(); ++iter)
  {
    (*iter).reset();
  }

  for (ElementPtr_V::iterator iter = this->elementDescriptions.begin();
      iter != this->elementDescriptions.end(); ++iter)
  {
    (*iter).reset();
  }
  this->elements.clear();
  this->elementDescriptions.clear();

  this->value.reset();

  // this->Reset();
}

/////////////////////////////////////////////////
ElementPtr Element::GetParent() const
{
  return this->parent;
}

/////////////////////////////////////////////////
void Element::SetParent(const ElementPtr _parent)
{
  this->parent = _parent;
}

/////////////////////////////////////////////////
void Element::SetName(const std::string &_name)
{
  this->name = _name;
}

/////////////////////////////////////////////////
const std::string &Element::GetName() const
{
  return this->name;
}

/////////////////////////////////////////////////
void Element::SetRequired(const std::string &_req)
{
  this->required = _req;
}

/////////////////////////////////////////////////
const std::string &Element::GetRequired() const
{
  return this->required;
}

/////////////////////////////////////////////////
void Element::SetCopyChildren(bool _value)
{
  this->copyChildren = _value;
}

/////////////////////////////////////////////////
bool Element::GetCopyChildren() const
{
  return this->copyChildren;
}

/////////////////////////////////////////////////
void Element::AddValue(const std::string &_type,
    const std::string &_defaultValue, bool _required)
{
  this->value = this->CreateParam(this->name, _type, _defaultValue, _required);
}

/////////////////////////////////////////////////
boost::shared_ptr<Param> Element::CreateParam(const std::string &_key,
    const std::string &_type, const std::string &_defaultValue, bool _required)
{
  if (_type == "double")
  {
    boost::shared_ptr<ParamT<double> > param(
        new ParamT<double>(_key, _defaultValue, _required));
    return param;
  }
  else if (_type == "int")
  {
    boost::shared_ptr<ParamT<int> > param(
        new ParamT<int>(_key, _defaultValue, _required));
    return param;
  }
  else if (_type == "unsigned int")
  {
    boost::shared_ptr<ParamT<unsigned int> > param(
        new ParamT<unsigned int>(_key, _defaultValue, _required));
    return param;
  }
  else if (_type == "float")
  {
    boost::shared_ptr<ParamT<float> > param(
        new ParamT<float>(_key, _defaultValue, _required));
    return param;
  }
  else if (_type == "bool")
  {
    boost::shared_ptr<ParamT<bool> > param(
        new ParamT<bool>(_key, _defaultValue, _required));
    return param;
  }
  else if (_type == "string")
  {
    boost::shared_ptr<ParamT<std::string> > param(
        new ParamT<std::string>(_key, _defaultValue, _required));
    return param;
  }
  else if (_type == "color")
  {
    boost::shared_ptr<ParamT<gazebo::common::Color> > param(
        new ParamT<gazebo::common::Color>(_key, _defaultValue, _required));
    return param;
  }
  else if (_type == "vector3")
  {
    boost::shared_ptr<ParamT<gazebo::math::Vector3> > param(
        new ParamT<gazebo::math::Vector3>(_key, _defaultValue, _required));
    return param;
  }
  else if (_type == "vector2i")
  {
    boost::shared_ptr<ParamT<gazebo::math::Vector2i> > param(
        new ParamT<gazebo::math::Vector2i>(_key, _defaultValue, _required));
    return param;
  }
  else if (_type == "vector2d")
  {
    boost::shared_ptr<ParamT<gazebo::math::Vector2d> > param(
        new ParamT<gazebo::math::Vector2d>(_key, _defaultValue, _required));
    return param;
  }
  else if (_type == "pose")
  {
    boost::shared_ptr<ParamT<gazebo::math::Pose> > param(
        new ParamT<gazebo::math::Pose>(_key, _defaultValue, _required));
    return param;
  }
  else if (_type == "time")
  {
    boost::shared_ptr<ParamT<gazebo::common::Time> > param(
        new ParamT<gazebo::common::Time>(_key, _defaultValue, _required));
    return param;
  }
  else
  {
    gzerr << "Unknown attribute _type[" << _type << "]\n";
    return boost::shared_ptr<Param>();
  }
}

/////////////////////////////////////////////////
void Element::AddAttribute(const std::string &_key, const std::string &_type,
    const std::string &_defaultValue, bool _required)
{
  this->attributes.push_back(
      this->CreateParam(_key, _type, _defaultValue, _required));
}

/////////////////////////////////////////////////
ElementPtr Element::Clone() const
{
  ElementPtr clone(new Element);
  clone->name = this->name;
  clone->required = this->required;
  // clone->parent = this->parent;
  clone->copyChildren = this->copyChildren;
  clone->includeFilename = this->includeFilename;

  Param_V::const_iterator aiter;
  for (aiter = this->attributes.begin();
      aiter != this->attributes.end(); ++aiter)
  {
    clone->attributes.push_back((*aiter)->Clone());
  }

  ElementPtr_V::const_iterator eiter;
  for (eiter = this->elementDescriptions.begin();
      eiter != this->elementDescriptions.end(); ++eiter)
  {
    clone->elementDescriptions.push_back((*eiter)->Clone());
  }

  for (eiter = this->elements.begin(); eiter != this->elements.end(); ++eiter)
  {
    clone->elements.push_back((*eiter)->Clone());
    clone->elements.back()->parent = clone;
  }

  if (this->value)
    clone->value = this->value->Clone();

  return clone;
}

/////////////////////////////////////////////////
void Element::Copy(const ElementPtr _elem)
{
  this->name = _elem->GetName();
  this->required = _elem->GetRequired();
  this->copyChildren = _elem->GetCopyChildren();
  this->includeFilename = _elem->includeFilename;

  for (Param_V::iterator iter = _elem->attributes.begin();
       iter != _elem->attributes.end(); ++iter)
  {
    if (!this->HasAttribute((*iter)->GetKey()))
      this->attributes.push_back((*iter)->Clone());
    ParamPtr param = this->GetAttribute((*iter)->GetKey());
    param->SetFromString((*iter)->GetAsString());
  }

  if (_elem->GetValue())
  {
    if (!this->value)
      this->value = _elem->GetValue()->Clone();
    else
      this->value->SetFromString(_elem->GetValue()->GetAsString());
  }

  this->elementDescriptions.clear();
  for (ElementPtr_V::const_iterator iter = _elem->elementDescriptions.begin();
       iter != _elem->elementDescriptions.end(); ++iter)
  {
    this->elementDescriptions.push_back((*iter)->Clone());
  }

  this->elements.clear();
  for (ElementPtr_V::iterator iter = _elem->elements.begin();
       iter != _elem->elements.end(); ++iter)
  {
    ElementPtr elem = (*iter)->Clone();
    elem->Copy(*iter);
    elem->parent = shared_from_this();
    this->elements.push_back(elem);
  }
}

/////////////////////////////////////////////////
void Element::PrintDescription(std::string _prefix)
{
  std::cout << _prefix << "<element name ='" << this->name
            << "' required ='" << this->required << "'>\n";

  Param_V::iterator aiter;
  for (aiter = this->attributes.begin();
      aiter != this->attributes.end(); ++aiter)
  {
    std::cout << _prefix << "  <attribute name ='"
              << (*aiter)->GetKey() << "' type ='" << (*aiter)->GetTypeName()
              << "' default ='" << (*aiter)->GetDefaultAsString()
              << "' required ='" << (*aiter)->GetRequired() << "'/>\n";
  }

  if (this->GetCopyChildren())
    std::cout << _prefix << "  <element copy_data ='true' required ='*'/>\n";

  ElementPtr_V::iterator eiter;
  for (eiter = this->elementDescriptions.begin();
      eiter != this->elementDescriptions.end(); ++eiter)
  {
    (*eiter)->PrintDescription(_prefix + "  ");
  }

  std::cout << _prefix << "</element>\n";
}

/////////////////////////////////////////////////
void Element::PrintValues(std::string _prefix)
{
  std::cout << _prefix << "<" << this->name;

  Param_V::iterator aiter;
  for (aiter = this->attributes.begin();
      aiter != this->attributes.end(); ++aiter)
  {
    std::cout << " " << (*aiter)->GetKey() << "='"
      << (*aiter)->GetAsString() << "'";
  }

  if (this->elements.size() > 0)
  {
    std::cout << ">\n";
    ElementPtr_V::iterator eiter;
    for (eiter = this->elements.begin();
        eiter != this->elements.end(); ++eiter)
    {
      (*eiter)->PrintValues(_prefix + "  ");
    }
    std::cout << _prefix << "</" << this->name << ">\n";
  }
  else
  {
    if (this->value)
    {
      std::cout << ">" << this->value->GetAsString()
        << "</" << this->name << ">\n";
    }
    else
    {
      std::cout << "/>\n";
    }
  }
}

/////////////////////////////////////////////////
std::string Element::ToString(const std::string &_prefix) const
{
  std::ostringstream out;
  this->ToString(_prefix, out);
  return out.str();
}

/////////////////////////////////////////////////
void Element::ToString(const std::string &_prefix,
                       std::ostringstream &_out) const
{
  if (this->includeFilename.empty())
  {
    _out << _prefix << "<" << this->name;

    Param_V::const_iterator aiter;
    for (aiter = this->attributes.begin();
        aiter != this->attributes.end(); ++aiter)
    {
      _out << " " << (*aiter)->GetKey() << "='"
           << (*aiter)->GetAsString() << "'";
    }

    if (this->elements.size() > 0)
    {
      _out << ">\n";
      ElementPtr_V::const_iterator eiter;
      for (eiter = this->elements.begin();
          eiter != this->elements.end(); ++eiter)
      {
        (*eiter)->ToString(_prefix + "  ", _out);
      }
      _out << _prefix << "</" << this->name << ">\n";
    }
    else
    {
      if (this->value)
      {
        _out << ">" << this->value->GetAsString()
             << "</" << this->name << ">\n";
      }
      else
      {
        _out << "/>\n";
      }
    }
  }
  else
  {
    _out << _prefix << "<include filename='"
         << this->includeFilename << "'/>\n";
  }
}

/////////////////////////////////////////////////
bool Element::HasAttribute(const std::string &_key)
{
  return this->GetAttribute(_key) != NULL;
}

/////////////////////////////////////////////////
bool Element::GetAttributeSet(const std::string &_key)
{
  bool result = false;
  ParamPtr p = this->GetAttribute(_key);
  if (p)
    result = p->GetSet();

  return result;
}

/////////////////////////////////////////////////
ParamPtr Element::GetAttribute(const std::string &_key)
{
  Param_V::const_iterator iter;
  for (iter = this->attributes.begin();
      iter != this->attributes.end(); ++iter)
  {
    if ((*iter)->GetKey() == _key)
      return (*iter);
  }
  return ParamPtr();
}

/////////////////////////////////////////////////
unsigned int Element::GetAttributeCount() const
{
  return this->attributes.size();
}

/////////////////////////////////////////////////
ParamPtr Element::GetAttribute(unsigned int _index) const
{
  ParamPtr result;
  if (_index < this->attributes.size())
    result = this->attributes[_index];

  return result;
}

/////////////////////////////////////////////////
unsigned int Element::GetElementDescriptionCount() const
{
  return this->elementDescriptions.size();
}

/////////////////////////////////////////////////
ElementPtr Element::GetElementDescription(unsigned int _index) const
{
  ElementPtr result;
  if (_index < this->elementDescriptions.size())
    result = this->elementDescriptions[_index];
  return result;
}

/////////////////////////////////////////////////
ElementPtr Element::GetElementDescription(const std::string &_key) const
{
  ElementPtr_V::const_iterator iter;
  for (iter = this->elementDescriptions.begin();
       iter != this->elementDescriptions.end(); ++iter)
  {
    if ((*iter)->GetName() == _key)
      return (*iter);
  }

  return ElementPtr();
}

/////////////////////////////////////////////////
ParamPtr Element::GetValue()
{
  return this->value;
}

/////////////////////////////////////////////////
bool Element::HasElement(const std::string &_name) const
{
  ElementPtr_V::const_iterator iter;
  for (iter = this->elements.begin(); iter != this->elements.end(); ++iter)
  {
    if ((*iter)->GetName() == _name)
      return true;
  }

  return false;
}

/////////////////////////////////////////////////
ElementPtr Element::GetElement(const std::string &_name) const
{
  ElementPtr_V::const_iterator iter;
  for (iter = this->elements.begin(); iter != this->elements.end(); ++iter)
  {
    if ((*iter)->GetName() == _name)
      return (*iter);
  }

  gzdbg << "Unable to find element with name[" << _name << "] return empty\n";
  return ElementPtr();
}

/////////////////////////////////////////////////
ElementPtr Element::GetFirstElement() const
{
  if (this->elements.empty())
    return ElementPtr();
  else
    return this->elements.front();
}

/////////////////////////////////////////////////
ElementPtr Element::GetNextElement(const std::string &_name) const
{
  if (this->parent)
  {
    ElementPtr_V::const_iterator iter;
    iter = std::find(this->parent->elements.begin(),
        this->parent->elements.end(), shared_from_this());

    if (iter == this->parent->elements.end())
    {
      return ElementPtr();
    }

    ++iter;
    if (iter == this->parent->elements.end())
      return ElementPtr();
    else if (_name.empty())
      return *(iter);
    else
    {
      for (; iter != this->parent->elements.end(); ++iter)
      {
        if ((*iter)->GetName() == _name)
          return (*iter);
      }
    }
  }

  return ElementPtr();
}

/////////////////////////////////////////////////
boost::shared_ptr<Element> Element::GetOrCreateElement(const std::string &_name)
{
  if (this->HasElement(_name))
    return this->GetElement(_name);
  else
    return this->AddElement(_name);
}

/////////////////////////////////////////////////
void Element::InsertElement(ElementPtr _elem)
{
  this->elements.push_back(_elem);
}

/////////////////////////////////////////////////
bool Element::HasElementDescription(const std::string &_name)
{
  bool result = false;
  ElementPtr_V::const_iterator iter;
  for (iter = this->elementDescriptions.begin();
       iter != this->elementDescriptions.end(); ++iter)
  {
    if ((*iter)->name == _name)
    {
      result = true;
      break;
    }
  }

  return result;
}

/////////////////////////////////////////////////
ElementPtr Element::AddElement(const std::string &_name)
{
  ElementPtr_V::const_iterator iter;
  for (iter = this->elementDescriptions.begin();
      iter != this->elementDescriptions.end(); ++iter)
  {
    if ((*iter)->name == _name)
    {
      ElementPtr elem = (*iter)->Clone();
      elem->SetParent(shared_from_this());
      this->elements.push_back(elem);
      return this->elements.back();
    }
  }
  gzerr << "Missing element description for [" << _name << "]\n";
  return ElementPtr();
}

/////////////////////////////////////////////////
bool Element::GetValueBool(const std::string &_key)
{
  bool result = false;

  if (_key.empty())
    this->value->Get(result);
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValueBool();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValueBool();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
int Element::GetValueInt(const std::string &_key)
{
  int result = 0;
  if (_key.empty())
    this->value->Get(result);
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValueInt();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValueInt();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
float Element::GetValueFloat(const std::string &_key)
{
  float result = 0.0;
  if (_key.empty())
    this->value->Get(result);
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValueFloat();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValueFloat();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
double Element::GetValueDouble(const std::string &_key)
{
  double result = 0.0;
  if (_key.empty())
  {
    if (this->value->IsStr())
      result = boost::lexical_cast<double>(this->value->GetAsString());
    else
      this->value->Get(result);
  }
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValueDouble();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValueDouble();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
unsigned int Element::GetValueUInt(const std::string &_key)
{
  unsigned int result = 0;
  if (_key.empty())
  {
    if (this->value->IsStr())
      result = boost::lexical_cast<unsigned int>(this->value->GetAsString());
    else
      this->value->Get(result);
  }
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValueUInt();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValueUInt();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
char Element::GetValueChar(const std::string &_key)
{
  char result = '\0';
  if (_key.empty())
  {
    if (this->value->IsStr())
      result = boost::lexical_cast<char>(this->value->GetAsString());
    else
      this->value->Get(result);
  }
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValueChar();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValueChar();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
std::string Element::GetValueString(const std::string &_key)
{
  std::string result = "";
  if (_key.empty())
    this->value->Get(result);
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValueString();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValueString();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
gazebo::math::Vector3 Element::GetValueVector3(const std::string &_key)
{
  gazebo::math::Vector3 result;
  if (_key.empty())
    this->value->Get(result);
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValueVector3();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValueVector3();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
gazebo::math::Vector2d Element::GetValueVector2d(const std::string &_key)
{
  gazebo::math::Vector2d result;
  if (_key.empty())
    this->value->Get(result);
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValueVector2d();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValueVector2d();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
gazebo::math::Quaternion Element::GetValueQuaternion(const std::string &_key)
{
  gazebo::math::Quaternion result;
  if (_key.empty())
    this->value->Get(result);
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValueQuaternion();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValueQuaternion();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
gazebo::math::Pose Element::GetValuePose(const std::string &_key)
{
  gazebo::math::Pose result;
  if (_key.empty())
    this->value->Get(result);
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValuePose();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValuePose();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
gazebo::common::Color Element::GetValueColor(const std::string &_key)
{
  gazebo::common::Color result;
  if (_key.empty())
    this->value->Get(result);
  else
  {
    ParamPtr param = this->GetAttribute(_key);
    if (param)
      param->Get(result);
    else if (this->HasElement(_key))
      result = this->GetElement(_key)->GetValueColor();
    else if (this->HasElementDescription(_key))
      result = this->GetElementDescription(_key)->GetValueColor();
    else
      gzerr << "Unable to find value for key[" << _key << "]\n";
  }
  return result;
}

/////////////////////////////////////////////////
void Element::ClearElements()
{
  this->elements.clear();
}

/////////////////////////////////////////////////
void Element::Update()
{
  for (sdf::Param_V::iterator iter = this->attributes.begin();
      iter != this->attributes.end(); ++iter)
  {
    (*iter)->Update();
  }

  for (sdf::ElementPtr_V::iterator iter = this->elements.begin();
      iter != this->elements.end(); ++iter)
  {
    (*iter)->Update();
  }
}

/////////////////////////////////////////////////
void Element::Reset()
{
  this->parent.reset();

  for (ElementPtr_V::iterator iter = this->elements.begin();
      iter != this->elements.end(); ++iter)
  {
    (*iter)->Reset();
    (*iter).reset();
  }

  for (ElementPtr_V::iterator iter = this->elementDescriptions.begin();
      iter != this->elementDescriptions.end(); ++iter)
  {
    (*iter)->Reset();
    (*iter).reset();
  }
  this->elements.clear();
  this->elementDescriptions.clear();

  this->value.reset();
}

/////////////////////////////////////////////////
void Element::AddElementDescription(ElementPtr _elem)
{
  this->elementDescriptions.push_back(_elem);
}

/////////////////////////////////////////////////
void Element::SetInclude(const std::string &_filename)
{
  this->includeFilename = _filename;
}

/////////////////////////////////////////////////
std::string Element::GetInclude() const
{
  return this->includeFilename;
}

/////////////////////////////////////////////////
bool Element::Set(const bool &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const int &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const unsigned int &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const float &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const double &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const char &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const std::string &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const char *_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const gazebo::math::Vector3 &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const gazebo::math::Vector2i &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const gazebo::math::Vector2d &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const gazebo::math::Quaternion &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const gazebo::math::Pose &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const gazebo::common::Color &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}

/////////////////////////////////////////////////
bool Element::Set(const gazebo::common::Time &_value)
{
  if (this->value)
  {
    this->value->Set(_value);
    return true;
  }
  return false;
}




/////////////////////////////////////////////////
SDF::SDF()
: root(new Element)
{
}

/////////////////////////////////////////////////
void SDF::PrintDescription()
{
  this->root->PrintDescription("");
}

/////////////////////////////////////////////////
void SDF::PrintValues()
{
  this->root->PrintValues("");
}

/////////////////////////////////////////////////
void SDF::Write(const std::string &_filename)
{
  std::string string = this->root->ToString("");

  std::ofstream out(_filename.c_str(), std::ios::out);

  if (!out)
  {
    gzerr << "Unable to open file[" << _filename << "] for writing\n";
    return;
  }
  out << string;
  out.close();
}

/////////////////////////////////////////////////
std::string SDF::ToString() const
{
  std::string result;

  if (this->root->GetName() != "gazebo")
    result = std::string("<gazebo version ='") + SDF_VERSION + "'>";

  result += this->root->ToString("");

  if (this->root->GetName() != "gazebo")
    result += "</gazebo>";

  return result;
}

/////////////////////////////////////////////////
void SDF::SetFromString(const std::string &_sdfData)
{
  sdf::initFile("sdf/gazebo.sdf", this->root);
  if (!sdf::readString(_sdfData, this->root))
  {
    gzerr << "Unable to parse sdf string[" << _sdfData << "]\n";
  }
}
