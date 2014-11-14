/*
 * Copyright 2014 Open Source Robotics Foundation
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

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include "gazebo/common/Console.hh"
#include "gazebo/gui/ConfigWidget.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
ConfigWidget::ConfigWidget()
{
  this->configMsg = NULL;
}

/////////////////////////////////////////////////
ConfigWidget::~ConfigWidget()
{
  delete this->configMsg;
}

/////////////////////////////////////////////////
void ConfigWidget::Load(const google::protobuf::Message *_msg)
{
  this->configMsg = _msg->New();
  this->configMsg->CopyFrom(*_msg);

  QWidget *widget = this->Parse(this->configMsg);
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->setAlignment(Qt::AlignTop);
  mainLayout->addWidget(widget);
  this->setLayout(mainLayout);
}

/////////////////////////////////////////////////
void ConfigWidget::UpdateFromMsg(const google::protobuf::Message *_msg)
{
  this->configMsg->CopyFrom(*_msg);
  this->Parse(this->configMsg);
}

/////////////////////////////////////////////////
google::protobuf::Message *ConfigWidget::GetMsg()
{
  this->UpdateMsg(this->configMsg);
  return this->configMsg;
}

/////////////////////////////////////////////////
void ConfigWidget::SetIntWidgetProperty(const std::string &_name, int _value)
{
  if (configWidgets.find(_name) == configWidgets.end())
    return;

  ConfigChildWidget *childWidget = configWidgets[_name];
  this->UpdateIntWidget(childWidget, _value);
}

/////////////////////////////////////////////////
void ConfigWidget::SetUIntWidgetProperty(const std::string &_name,
    unsigned int _value)
{
  if (configWidgets.find(_name) == configWidgets.end())
    return;

  ConfigChildWidget *childWidget = configWidgets[_name];
  this->UpdateUIntWidget(childWidget, _value);
}

/////////////////////////////////////////////////
void ConfigWidget::SetDoubleWidgetProperty(const std::string &_name,
    double _value)
{
  if (configWidgets.find(_name) == configWidgets.end())
    return;

  ConfigChildWidget *childWidget = configWidgets[_name];
  this->UpdateDoubleWidget(childWidget, _value);
}

/////////////////////////////////////////////////
void ConfigWidget::SetBoolWidgetProperty(const std::string &_name,
    bool _value)
{
  if (configWidgets.find(_name) == configWidgets.end())
    return;

  ConfigChildWidget *childWidget = configWidgets[_name];
  this->UpdateBoolWidget(childWidget, _value);
}

/////////////////////////////////////////////////
void ConfigWidget::SetStringWidgetProperty(const std::string &_name,
    const std::string &_value)
{
  if (configWidgets.find(_name) == configWidgets.end())
    return;

  ConfigChildWidget *childWidget = configWidgets[_name];
  this->UpdateStringWidget(childWidget, _value);
}

/////////////////////////////////////////////////
void ConfigWidget::SetVector3WidgetProperty(const std::string &_name,
    const math::Vector3 &_value)
{
  if (configWidgets.find(_name) == configWidgets.end())
    return;

  ConfigChildWidget *childWidget = configWidgets[_name];
  this->UpdateVector3Widget(childWidget, _value);
}

/////////////////////////////////////////////////
void ConfigWidget::SetColorWidgetProperty(const std::string &_name,
    const common::Color &_value)
{
  if (configWidgets.find(_name) == configWidgets.end())
    return;

  ConfigChildWidget *childWidget = configWidgets[_name];
  this->UpdateColorWidget(childWidget, _value);
}

/////////////////////////////////////////////////
void ConfigWidget::SetPoseWidgetProperty(const std::string &_name,
    const math::Pose &_value)
{
  if (configWidgets.find(_name) == configWidgets.end())
    return;

  ConfigChildWidget *childWidget = configWidgets[_name];
  this->UpdatePoseWidget(childWidget, _value);
}

/////////////////////////////////////////////////
void ConfigWidget::SetGeometryWidgetProperty(const std::string &_name,
    const std::string &_value, const math::Vector3 &_dimensions)
{
  if (configWidgets.find(_name) == configWidgets.end())
    return;

  ConfigChildWidget *childWidget = configWidgets[_name];
  this->UpdateGeometryWidget(childWidget, _value, _dimensions);
}


/////////////////////////////////////////////////
QWidget *ConfigWidget::Parse(google::protobuf::Message *_msg,
    const std::string &_name)
{
  std::vector<QWidget *> newWidgets;

  const google::protobuf::Descriptor *d = _msg->GetDescriptor();
  if (!d)
    return NULL;
  unsigned int count = d->field_count();

  for (unsigned int i = 0; i < count ; ++i)
  {
    const google::protobuf::FieldDescriptor *field = d->field(i);

    if (!field)
      return NULL;

    const google::protobuf::Reflection *ref = _msg->GetReflection();

    if (!ref)
      return NULL;

    std::string name = field->name();

    // Parse each field in the message
    // TODO parse repeated fields and enum fields.
    if (!field->is_repeated())
    {
      QWidget *newFieldWidget = NULL;
      ConfigChildWidget *configChildWidget = NULL;

      bool newWidget = true;
      std::string scopedName = _name.empty() ? name : _name + "::" + name;
      if (configWidgets.find(scopedName) != configWidgets.end())
        newWidget = false;
      else
        configChildWidget = configWidgets[scopedName];

      switch (field->cpp_type())
      {
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        {
          double value = ref->GetDouble(*_msg, field);
          if (!math::equal(value, value))
            value = 0;
          if (newWidget)
          {
            configChildWidget = CreateDoubleWidget(name);
            newFieldWidget = configChildWidget;
          }
          this->UpdateDoubleWidget(configChildWidget, value);
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        {
          float value = ref->GetFloat(*_msg, field);
          if (!math::equal(value, value))
            value = 0;
          if (newWidget)
          {
            configChildWidget = CreateDoubleWidget(name);
            newFieldWidget = configChildWidget;
          }
          this->UpdateDoubleWidget(configChildWidget, value);
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        {
          int64_t value = ref->GetInt64(*_msg, field);
          if (newWidget)
          {
            configChildWidget = CreateIntWidget(name);
            newFieldWidget = configChildWidget;
          }
          this->UpdateIntWidget(configChildWidget, value);
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        {
          uint64_t value = ref->GetUInt64(*_msg, field);
          if (newWidget)
          {
            configChildWidget = CreateUIntWidget(name);
            newFieldWidget = configChildWidget;
          }
          this->UpdateUIntWidget(configChildWidget, value);
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        {
          int32_t value = ref->GetInt32(*_msg, field);
          if (newWidget)
          {
            configChildWidget = CreateIntWidget(name);
            newFieldWidget = configChildWidget;
          }
          this->UpdateIntWidget(configChildWidget, value);
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        {
          uint32_t value = ref->GetUInt32(*_msg, field);
          if (newWidget)
          {
            configChildWidget = CreateUIntWidget(name);
            newFieldWidget = configChildWidget;
          }
          this->UpdateUIntWidget(configChildWidget, value);
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        {
          bool value = ref->GetBool(*_msg, field);
          if (newWidget)
          {
            configChildWidget = CreateBoolWidget(name);
            newFieldWidget = configChildWidget;
          }
          this->UpdateBoolWidget(configChildWidget, value);
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
          std::string value = ref->GetString(*_msg, field);
          if (newWidget)
          {
            configChildWidget = CreateStringWidget(name);
            newFieldWidget = configChildWidget;
          }
          this->UpdateStringWidget(configChildWidget, value);
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
          google::protobuf::Message *valueMsg =
              ref->MutableMessage(_msg, field);

          // parse and create custom geometry widgets
          if (field->message_type()->name() == "Geometry")
          {
            if (newWidget)
            {
              configChildWidget = this->CreateGeometryWidget(name);
              newFieldWidget = configChildWidget;
            }

            // type
            const google::protobuf::Descriptor *valueDescriptor =
                valueMsg->GetDescriptor();
            const google::protobuf::FieldDescriptor *typeField =
                valueDescriptor->FindFieldByName("type");
            const google::protobuf::EnumValueDescriptor *typeValueDescriptor =
                valueMsg->GetReflection()->GetEnum(*valueMsg, typeField);

            std::string geometryTypeStr;
            if (typeValueDescriptor)
            {
              geometryTypeStr =
                  QString(typeValueDescriptor->name().c_str()).toLower().
                  toStdString();
            }

            math::Vector3 dimensions;
            // dimensions
            for (int k = 0; k < valueDescriptor->field_count() ; ++k)
            {
              const google::protobuf::FieldDescriptor *geomField =
                  valueDescriptor->field(k);

              if (geomField->is_repeated())
                  continue;

              if (geomField->cpp_type() !=
                  google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE ||
                  !valueMsg->GetReflection()->HasField(*valueMsg, geomField))
                continue;

              google::protobuf::Message *geomValueMsg =
                valueMsg->GetReflection()->MutableMessage(valueMsg, geomField);
              const google::protobuf::Descriptor *geomValueDescriptor =
                  geomValueMsg->GetDescriptor();

              std::string geomMsgName = geomField->message_type()->name();
              if (geomMsgName == "BoxGeom")
              {
                google::protobuf::Message *geomDimMsg =
                    geomValueMsg->GetReflection()->MutableMessage(geomValueMsg,
                    geomValueDescriptor->field(0));
                dimensions = this->ParseVector3(geomDimMsg);
                break;
              }
              else if (geomMsgName == "CylinderGeom")
              {
                const google::protobuf::FieldDescriptor *geomRadiusField =
                    geomValueDescriptor->FindFieldByName("radius");
                double radius = geomValueMsg->GetReflection()->GetDouble(
                    *geomValueMsg, geomRadiusField);
                const google::protobuf::FieldDescriptor *geomLengthField =
                    geomValueDescriptor->FindFieldByName("length");
                double length = geomValueMsg->GetReflection()->GetDouble(
                    *geomValueMsg, geomLengthField);
                dimensions.x = radius * 2.0;
                dimensions.y = radius * 2.0;
                dimensions.z = length;
                break;
              }
              else if (geomMsgName == "SphereGeom")
              {
                const google::protobuf::FieldDescriptor *geomRadiusField =
                    geomValueDescriptor->FindFieldByName("radius");
                double radius = geomValueMsg->GetReflection()->GetDouble(
                    *geomValueMsg, geomRadiusField);
                dimensions.x = radius;
                dimensions.y = radius;
                dimensions.z = radius;
                break;
              }
            }
            this->UpdateGeometryWidget(configChildWidget,
                geometryTypeStr, dimensions);
          }
          // parse and create custom pose widgets
          else if (field->message_type()->name() == "Pose")
          {
            if (newWidget)
            {
              configChildWidget = this->CreatePoseWidget(name);
              newFieldWidget = configChildWidget;
            }

            math::Pose value;
            const google::protobuf::Descriptor *valueDescriptor =
                valueMsg->GetDescriptor();
            int valueMsgFieldCount = valueDescriptor->field_count();
            for (int j = 0; j < valueMsgFieldCount ; ++j)
            {
              const google::protobuf::FieldDescriptor *valueField =
                  valueDescriptor->field(j);

              if (valueField->cpp_type() !=
                  google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                continue;

              if (valueField->message_type()->name() == "Vector3d")
              {
                // pos
                google::protobuf::Message *posValueMsg =
                    valueMsg->GetReflection()->MutableMessage(
                    valueMsg, valueField);
                math::Vector3 vec3 = this->ParseVector3(posValueMsg);
                value.pos = vec3;
              }
              else if (valueField->message_type()->name() == "Quaternion")
              {
                // rot
                google::protobuf::Message *quatValueMsg =
                    valueMsg->GetReflection()->MutableMessage(
                    valueMsg, valueField);
                const google::protobuf::Descriptor *quatValueDescriptor =
                    quatValueMsg->GetDescriptor();
                std::vector<double> quatValues;
                for (unsigned int k = 0; k < 4; ++k)
                {
                  const google::protobuf::FieldDescriptor *quatValueField =
                      quatValueDescriptor->field(k);
                  quatValues.push_back(quatValueMsg->GetReflection()->GetDouble(
                      *quatValueMsg, quatValueField));
                }
                math::Quaternion quat(quatValues[3], quatValues[0],
                    quatValues[1], quatValues[2]);
                value.rot = quat;
              }
            }
            this->UpdatePoseWidget(configChildWidget, value);
          }
          // parse and create custom vector3 widgets
          else if (field->message_type()->name() == "Vector3d")
          {
            if (newWidget)
            {
              configChildWidget = this->CreateVector3dWidget(name);
              newFieldWidget = configChildWidget;
            }

            math::Vector3 vec3 = this->ParseVector3(valueMsg);
            this->UpdateVector3Widget(configChildWidget, vec3);

          }
          // parse and create custom color widgets
          else if (field->message_type()->name() == "Color")
          {
            if (newWidget)
            {
              configChildWidget = this->CreateColorWidget(name);
              newFieldWidget = configChildWidget;
            }

            common::Color color;
            const google::protobuf::Descriptor *valueDescriptor =
                valueMsg->GetDescriptor();
            std::vector<double> values;
            for (unsigned int j = 0; j < configChildWidget->widgets.size(); ++j)
            {
              const google::protobuf::FieldDescriptor *valueField =
                  valueDescriptor->field(j);
              values.push_back(
                  valueMsg->GetReflection()->GetFloat(*valueMsg, valueField));
            }
            color.r = values[0];
            color.g = values[1];
            color.b = values[2];
            color.a = values[3];
            this->UpdateColorWidget(configChildWidget, color);
          }
          else
          {
            // parse the message fields recursively
            newFieldWidget = Parse(valueMsg, scopedName);
          }

          if (newWidget)
          {
            // create a group widget to collapse or expand child widgets
            // (contained in a group box).
            GroupWidget *groupWidget = new GroupWidget;
            newFieldWidget->setStyleSheet(
                "QGroupBox {border : 0px; padding-left : 10px}");

            QVBoxLayout *configGroupLayout = new QVBoxLayout;
            configGroupLayout->setContentsMargins(0, 0, 0, 0);
            QPushButton *groupButton = new QPushButton(tr(name.c_str()));
            // set the child widget so it can be toggled
            groupWidget->childWidget = newFieldWidget;
            configGroupLayout->addWidget(groupButton);
            configGroupLayout->addWidget(newFieldWidget);
            groupWidget->setLayout(configGroupLayout);
            connect(groupButton, SIGNAL(clicked()), groupWidget,SLOT(Toggle()));
            // reset new field widget pointer in order for it to be added
            // to the parent widget
            newFieldWidget = groupWidget;
          }

          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
          // const google::protobuf::EnumValueDescriptor *value =
          //    ref->GetEnum(*_msg, field);
          // TODO Implement enum parsing
          break;
        default:
          break;
      }
      if (newWidget && newFieldWidget)
      {
        newFieldWidget->setSizePolicy(QSizePolicy::Preferred,
            QSizePolicy::Fixed);

        newWidgets.push_back(newFieldWidget);

        // store the newly created widget in a map with a unique scoped name.
        if (qobject_cast<GroupWidget *>(newFieldWidget))
        {
          GroupWidget *groupWidget=
              qobject_cast<GroupWidget *>(newFieldWidget);
          this->configWidgets[scopedName] =
              qobject_cast<ConfigChildWidget *>(groupWidget->childWidget);
        }
        else if (qobject_cast<ConfigChildWidget *>(newFieldWidget))
        {
          this->configWidgets[scopedName] =
              qobject_cast<ConfigChildWidget *>(newFieldWidget);
        }
      }
    }
  }

  if (!newWidgets.empty())
  {
    // create a group box to hold child widgets.
    QGroupBox *widget = new QGroupBox();
    QVBoxLayout *widgetLayout = new QVBoxLayout;

    for (unsigned int i = 0; i < newWidgets.size(); ++i)
      widgetLayout->addWidget(newWidgets[i]);

    widgetLayout->setContentsMargins(0, 0, 0, 0);
    widgetLayout->setAlignment(Qt::AlignTop);
    widget->setLayout(widgetLayout);
    widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    return widget;
  }
  return NULL;
}

/////////////////////////////////////////////////
math::Vector3 ConfigWidget::ParseVector3(const google::protobuf::Message *_msg)
{
  math::Vector3 vec3;
  const google::protobuf::Descriptor *valueDescriptor =
      _msg->GetDescriptor();
  std::vector<double> values;
  for (unsigned int i = 0; i < 3; ++i)
  {
    const google::protobuf::FieldDescriptor *valueField =
        valueDescriptor->field(i);
    values.push_back(_msg->GetReflection()->GetDouble(*_msg, valueField));
  }
  vec3.x = values[0];
  vec3.y = values[1];
  vec3.z = values[2];
  return vec3;
}

/////////////////////////////////////////////////
ConfigChildWidget *ConfigWidget::CreateUIntWidget(const std::string &_key)
{
  QHBoxLayout *widgetLayout = new QHBoxLayout;
  QLabel *keyLabel = new QLabel(tr(_key.c_str()));
  widgetLayout->addWidget(keyLabel);
  QSpinBox *valueSpinBox = new QSpinBox;
  valueSpinBox->setRange(0, 1e6);
  valueSpinBox->setAlignment(Qt::AlignRight);
  widgetLayout->addWidget(valueSpinBox);
  ConfigChildWidget *widget = new ConfigChildWidget;
  widgetLayout->setContentsMargins(0, 0, 0, 0);
  widget->setLayout(widgetLayout);
  widget->widgets.push_back(valueSpinBox);
  return widget;
}

/////////////////////////////////////////////////
ConfigChildWidget *ConfigWidget::CreateIntWidget(const std::string &_key)
{
  QHBoxLayout *widgetLayout = new QHBoxLayout;
  QLabel *keyLabel = new QLabel(tr(_key.c_str()));
  widgetLayout->addWidget(keyLabel);
  QSpinBox *valueSpinBox = new QSpinBox;
  valueSpinBox->setRange(-1e6, 1e6);
  valueSpinBox->setAlignment(Qt::AlignRight);
  widgetLayout->addWidget(valueSpinBox);
  ConfigChildWidget *widget = new ConfigChildWidget;
  widgetLayout->setContentsMargins(0, 0, 0, 0);
  widget->setLayout(widgetLayout);
  widget->widgets.push_back(valueSpinBox);
  return widget;
}

/////////////////////////////////////////////////
ConfigChildWidget *ConfigWidget::CreateDoubleWidget(const std::string &_key)
{
  QHBoxLayout *widgetLayout = new QHBoxLayout;
  QLabel *keyLabel = new QLabel(tr(_key.c_str()));
  widgetLayout->addWidget(keyLabel);
  QDoubleSpinBox *valueSpinBox = new QDoubleSpinBox;
  valueSpinBox->setRange(-1e6, 1e6);
  valueSpinBox->setAlignment(Qt::AlignRight);
  widgetLayout->addWidget(valueSpinBox);
  ConfigChildWidget *widget = new ConfigChildWidget;
  widgetLayout->setContentsMargins(0, 0, 0, 0);
  widget->setLayout(widgetLayout);
  widget->widgets.push_back(valueSpinBox);
  return widget;
}

/////////////////////////////////////////////////
ConfigChildWidget *ConfigWidget::CreateStringWidget(const std::string &_key)
{
  QHBoxLayout *widgetLayout = new QHBoxLayout;
  QLabel *keyLabel = new QLabel(tr(_key.c_str()));
  widgetLayout->addWidget(keyLabel);
  QLineEdit *valueLineEdit = new QLineEdit;
  valueLineEdit->setSizePolicy(QSizePolicy::Minimum,
      QSizePolicy::Fixed);
  widgetLayout->addWidget(valueLineEdit);
  ConfigChildWidget *widget = new ConfigChildWidget;
  widgetLayout->setContentsMargins(0, 0, 0, 0);
  widget->setLayout(widgetLayout);
  widget->widgets.push_back(valueLineEdit);
  return widget;
}

/////////////////////////////////////////////////
ConfigChildWidget *ConfigWidget::CreateBoolWidget(const std::string &_key)
{
  QHBoxLayout *widgetLayout = new QHBoxLayout;
  QLabel *keyLabel = new QLabel(tr(_key.c_str()));
  widgetLayout->addWidget(keyLabel);

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  QRadioButton *valueTrueRadioButton = new QRadioButton;
  valueTrueRadioButton->setText(tr("True"));
  QRadioButton *valueFalseRadioButton = new QRadioButton;
  valueFalseRadioButton->setText(tr("False"));
  QButtonGroup *boolButtonGroup = new QButtonGroup;
  boolButtonGroup->addButton(valueTrueRadioButton);
  boolButtonGroup->addButton(valueFalseRadioButton);
  boolButtonGroup->setExclusive(true);
  buttonLayout->addWidget(valueTrueRadioButton);
  buttonLayout->addWidget(valueFalseRadioButton);
  widgetLayout->addLayout(buttonLayout);
  ConfigChildWidget *widget = new ConfigChildWidget;
  widgetLayout->setContentsMargins(0, 0, 0, 0);
  widget->setLayout(widgetLayout);
  widget->widgets.push_back(valueTrueRadioButton);
  widget->widgets.push_back(valueFalseRadioButton);
  return widget;
}

/////////////////////////////////////////////////
ConfigChildWidget *ConfigWidget::CreateVector3dWidget(
    const std::string &/*_key*/)
{
  QLabel *vecXLabel = new QLabel(tr("x: "));
  QLabel *vecYLabel = new QLabel(tr("y: "));
  QLabel *vecZLabel = new QLabel(tr("z: "));

  QDoubleSpinBox *vecXSpinBox = new QDoubleSpinBox;
  vecXSpinBox->setRange(-1e6, 1e6);
  vecXSpinBox->setSingleStep(0.01);
  vecXSpinBox->setDecimals(6);
  vecXSpinBox->setAlignment(Qt::AlignRight);

  QDoubleSpinBox *vecYSpinBox = new QDoubleSpinBox;
  vecYSpinBox->setRange(-1e6, 1e6);
  vecYSpinBox->setSingleStep(0.01);
  vecYSpinBox->setDecimals(6);
  vecYSpinBox->setAlignment(Qt::AlignRight);

  QDoubleSpinBox *vecZSpinBox = new QDoubleSpinBox;
  vecZSpinBox->setRange(-1e6, 1e6);
  vecZSpinBox->setSingleStep(0.01);
  vecZSpinBox->setDecimals(6);
  vecZSpinBox->setAlignment(Qt::AlignRight);

  QHBoxLayout *widgetLayout = new QHBoxLayout;
  widgetLayout->addWidget(vecXLabel);
  widgetLayout->addWidget(vecXSpinBox);
  widgetLayout->addWidget(vecYLabel);
  widgetLayout->addWidget(vecYSpinBox);
  widgetLayout->addWidget(vecZLabel);
  widgetLayout->addWidget(vecZSpinBox);

  ConfigChildWidget *widget = new ConfigChildWidget;
  widget->setLayout(widgetLayout);
  widgetLayout->setContentsMargins(0, 0, 0, 0);

  widget->widgets.push_back(vecXSpinBox);
  widget->widgets.push_back(vecYSpinBox);
  widget->widgets.push_back(vecZSpinBox);
  return widget;
}

/////////////////////////////////////////////////
ConfigChildWidget *ConfigWidget::CreateColorWidget(const std::string &/*_key*/)
{
  QLabel *colorRLabel = new QLabel(tr("r: "));
  QLabel *colorGLabel = new QLabel(tr("g: "));
  QLabel *colorBLabel = new QLabel(tr("b: "));
  QLabel *colorALabel = new QLabel(tr("a: "));

  QDoubleSpinBox *colorRSpinBox = new QDoubleSpinBox;
  colorRSpinBox->setRange(0, 1.0);
  colorRSpinBox->setSingleStep(0.01);
  colorRSpinBox->setDecimals(3);
  colorRSpinBox->setAlignment(Qt::AlignRight);

  QDoubleSpinBox *colorGSpinBox = new QDoubleSpinBox;
  colorGSpinBox->setRange(0, 1.0);
  colorGSpinBox->setSingleStep(0.01);
  colorGSpinBox->setDecimals(3);
  colorGSpinBox->setAlignment(Qt::AlignRight);

  QDoubleSpinBox *colorBSpinBox = new QDoubleSpinBox;
  colorBSpinBox->setRange(0, 1.0);
  colorBSpinBox->setSingleStep(0.01);
  colorBSpinBox->setDecimals(3);
  colorBSpinBox->setAlignment(Qt::AlignRight);

  QDoubleSpinBox *colorASpinBox = new QDoubleSpinBox;
  colorASpinBox->setRange(0, 1.0);
  colorASpinBox->setSingleStep(0.01);
  colorASpinBox->setDecimals(3);
  colorASpinBox->setAlignment(Qt::AlignRight);

  QHBoxLayout *widgetLayout = new QHBoxLayout;
  widgetLayout->addWidget(colorRLabel);
  widgetLayout->addWidget(colorRSpinBox);
  widgetLayout->addWidget(colorGLabel);
  widgetLayout->addWidget(colorGSpinBox);
  widgetLayout->addWidget(colorBLabel);
  widgetLayout->addWidget(colorBSpinBox);
  widgetLayout->addWidget(colorALabel);
  widgetLayout->addWidget(colorASpinBox);

  ConfigChildWidget *widget = new ConfigChildWidget;
  widget->setLayout(widgetLayout);
  widgetLayout->setContentsMargins(0, 0, 0, 0);

  widget->widgets.push_back(colorRSpinBox);
  widget->widgets.push_back(colorGSpinBox);
  widget->widgets.push_back(colorBSpinBox);
  widget->widgets.push_back(colorASpinBox);

  return widget;
}

/////////////////////////////////////////////////
ConfigChildWidget *ConfigWidget::CreatePoseWidget(const std::string &/*_key*/)
{
  QLabel *posXLabel = new QLabel(tr("x: "));
  QLabel *posYLabel = new QLabel(tr("y: "));
  QLabel *posZLabel = new QLabel(tr("z: "));
  QLabel *rotRLabel = new QLabel(tr("roll: "));
  QLabel *rotPLabel = new QLabel(tr("pitch: "));
  QLabel *rotYLabel = new QLabel(tr("yaw: "));

  QDoubleSpinBox *posXSpinBox = new QDoubleSpinBox;
  posXSpinBox->setRange(-1e6, 1e6);
  posXSpinBox->setSingleStep(0.01);
  posXSpinBox->setDecimals(6);
  posXSpinBox->setAlignment(Qt::AlignRight);

  QDoubleSpinBox *posYSpinBox = new QDoubleSpinBox;
  posYSpinBox->setRange(-1e6, 1e6);
  posYSpinBox->setSingleStep(0.01);
  posYSpinBox->setDecimals(6);
  posYSpinBox->setAlignment(Qt::AlignRight);

  QDoubleSpinBox *posZSpinBox = new QDoubleSpinBox;
  posZSpinBox->setRange(-1e6, 1e6);
  posZSpinBox->setSingleStep(0.01);
  posZSpinBox->setDecimals(6);
  posZSpinBox->setAlignment(Qt::AlignRight);

  QDoubleSpinBox *rotRSpinBox = new QDoubleSpinBox;
  rotRSpinBox->setRange(-1e6, 1e6);
  rotRSpinBox->setSingleStep(0.01);
  rotRSpinBox->setDecimals(6);
  rotRSpinBox->setAlignment(Qt::AlignRight);

  QDoubleSpinBox *rotPSpinBox = new QDoubleSpinBox;
  rotPSpinBox->setRange(-1e6, 1e6);
  rotPSpinBox->setSingleStep(0.01);
  rotPSpinBox->setDecimals(6);
  rotPSpinBox->setAlignment(Qt::AlignRight);

  QDoubleSpinBox *rotYSpinBox = new QDoubleSpinBox;
  rotYSpinBox->setRange(-1e6, 1e6);
  rotYSpinBox->setSingleStep(0.01);
  rotYSpinBox->setDecimals(6);
  rotYSpinBox->setAlignment(Qt::AlignRight);

  QGridLayout *poseGroupLayout = new QGridLayout;
  poseGroupLayout->addWidget(posXLabel, 0, 0);
  poseGroupLayout->addWidget(posXSpinBox, 0, 1);
  poseGroupLayout->addWidget(posYLabel, 0, 2);
  poseGroupLayout->addWidget(posYSpinBox, 0, 3);
  poseGroupLayout->addWidget(posZLabel, 0, 4);
  poseGroupLayout->addWidget(posZSpinBox, 0, 5);
  poseGroupLayout->addWidget(rotRLabel, 1, 0);
  poseGroupLayout->addWidget(rotRSpinBox, 1, 1);
  poseGroupLayout->addWidget(rotPLabel, 1, 2);
  poseGroupLayout->addWidget(rotPSpinBox, 1, 3);
  poseGroupLayout->addWidget(rotYLabel, 1, 4);
  poseGroupLayout->addWidget(rotYSpinBox, 1, 5);

  poseGroupLayout->setColumnStretch(1, 1);
  poseGroupLayout->setAlignment(posXSpinBox, Qt::AlignLeft);
  poseGroupLayout->setAlignment(posYSpinBox, Qt::AlignLeft);
  poseGroupLayout->setAlignment(posZSpinBox, Qt::AlignLeft);
  poseGroupLayout->setAlignment(rotRSpinBox, Qt::AlignLeft);
  poseGroupLayout->setAlignment(rotPSpinBox, Qt::AlignLeft);
  poseGroupLayout->setAlignment(rotYSpinBox, Qt::AlignLeft);

  ConfigChildWidget *widget = new ConfigChildWidget;
  poseGroupLayout->setContentsMargins(0, 0, 0, 0);
  widget->setLayout(poseGroupLayout);

  widget->widgets.push_back(posXSpinBox);
  widget->widgets.push_back(posYSpinBox);
  widget->widgets.push_back(posZSpinBox);
  widget->widgets.push_back(rotRSpinBox);
  widget->widgets.push_back(rotPSpinBox);
  widget->widgets.push_back(rotYSpinBox);

  return widget;
}

/////////////////////////////////////////////////
ConfigChildWidget *ConfigWidget::CreateGeometryWidget(
    const std::string &/*_key*/)
{
  QLabel *geometryLabel = new QLabel(tr("geometry"));
  QComboBox *geometryComboBox = new QComboBox;
  geometryComboBox->addItem(tr("box"));
  geometryComboBox->addItem(tr("cylinder"));
  geometryComboBox->addItem(tr("sphere"));
  // geometryComboBox->addItem(tr("custom"));

  QDoubleSpinBox *geomSizeXSpinBox = new QDoubleSpinBox;
  geomSizeXSpinBox->setRange(-1000, 1000);
  geomSizeXSpinBox->setSingleStep(0.01);
  geomSizeXSpinBox->setDecimals(6);
  geomSizeXSpinBox->setValue(1.000);

  QDoubleSpinBox *geomSizeYSpinBox = new QDoubleSpinBox;
  geomSizeYSpinBox->setRange(-1000, 1000);
  geomSizeYSpinBox->setSingleStep(0.01);
  geomSizeYSpinBox->setDecimals(6);
  geomSizeYSpinBox->setValue(1.000);

  QDoubleSpinBox *geomSizeZSpinBox = new QDoubleSpinBox;
  geomSizeZSpinBox->setRange(-1000, 1000);
  geomSizeZSpinBox->setSingleStep(0.01);
  geomSizeZSpinBox->setDecimals(6);
  geomSizeZSpinBox->setValue(1.000);

  QLabel *geomSizeXLabel = new QLabel(tr("x: "));
  QLabel *geomSizeYLabel = new QLabel(tr("y: "));
  QLabel *geomSizeZLabel = new QLabel(tr("z: "));

  QHBoxLayout *geomSizeLayout = new QHBoxLayout;
  geomSizeLayout->addWidget(geomSizeXLabel);
  geomSizeLayout->addWidget(geomSizeXSpinBox);
  geomSizeLayout->addWidget(geomSizeYLabel);
  geomSizeLayout->addWidget(geomSizeYSpinBox);
  geomSizeLayout->addWidget(geomSizeZLabel);
  geomSizeLayout->addWidget(geomSizeZSpinBox);

  QLabel *geomRadiusLabel = new QLabel(tr("radius: "));
  QLabel *geomLengthLabel = new QLabel(tr("length: "));

  QDoubleSpinBox *geomRadiusSpinBox = new QDoubleSpinBox;
  geomRadiusSpinBox->setRange(-1000, 1000);
  geomRadiusSpinBox->setSingleStep(0.01);
  geomRadiusSpinBox->setDecimals(6);
  geomRadiusSpinBox->setValue(0.500);

  QDoubleSpinBox *geomLengthSpinBox = new QDoubleSpinBox;
  geomLengthSpinBox->setRange(-1000, 1000);
  geomLengthSpinBox->setSingleStep(0.01);
  geomLengthSpinBox->setDecimals(6);
  geomLengthSpinBox->setValue(1.000);

  QHBoxLayout *geomRLLayout = new QHBoxLayout;
  geomRLLayout->addWidget(geomRadiusLabel);
  geomRLLayout->addWidget(geomRadiusSpinBox);
  geomRLLayout->addWidget(geomLengthLabel);
  geomRLLayout->addWidget(geomLengthSpinBox);

  QStackedWidget *geomDimensionWidget = new QStackedWidget;

  QWidget *geomSizeWidget = new QWidget;
  geomSizeWidget->setLayout(geomSizeLayout);
  geomDimensionWidget->insertWidget(0, geomSizeWidget);

  QWidget *geomRLWidget = new QWidget;
  geomRLWidget->setLayout(geomRLLayout);
  geomDimensionWidget->insertWidget(1, geomRLWidget);
  geomDimensionWidget->setCurrentIndex(0);
  geomDimensionWidget->setSizePolicy(
      QSizePolicy::Minimum, QSizePolicy::Minimum);

  QGridLayout *widgetLayout = new QGridLayout;
  widgetLayout->addWidget(geometryLabel, 1, 0);
  widgetLayout->addWidget(geometryComboBox, 1, 1);
  widgetLayout->addWidget(geomDimensionWidget, 2, 1);

  GeometryConfigWidget *widget = new GeometryConfigWidget;
  widget->geomDimensionWidget = geomDimensionWidget;
  widget->geomLengthSpinBox = geomLengthSpinBox;
  widget->geomLengthLabel = geomLengthLabel;

  connect(geometryComboBox,
    SIGNAL(currentIndexChanged(const QString)),
    widget, SLOT(GeometryChanged(const QString)));

  widgetLayout->setContentsMargins(0, 0, 0, 0);
  widget->setLayout(widgetLayout);

  widget->widgets.push_back(geometryComboBox);
  widget->widgets.push_back(geomSizeXSpinBox);
  widget->widgets.push_back(geomSizeYSpinBox);
  widget->widgets.push_back(geomSizeZSpinBox);
  widget->widgets.push_back(geomRadiusSpinBox);
  widget->widgets.push_back(geomLengthSpinBox);

  return widget;
}

/////////////////////////////////////////////////
void ConfigWidget::UpdateMsg(google::protobuf::Message *_msg,
    const std::string &_name)
{
  const google::protobuf::Descriptor *d = _msg->GetDescriptor();
  if (!d)
    return;
  unsigned int count = d->field_count();

  for (unsigned int i = 0; i < count ; ++i)
  {
    const google::protobuf::FieldDescriptor *field = d->field(i);

    if (!field)
      return;

    const google::protobuf::Reflection *ref = _msg->GetReflection();

    if (!ref)
      return;

    std::string name = field->name();

    // Update each field in the message
    // TODO update repeated fields and enum fields
    if (!field->is_repeated() && ref->HasField(*_msg, field))
    {
      std::string scopedName = _name.empty() ? name : _name + "::" + name;
      if (configWidgets.find(scopedName) == configWidgets.end())
        continue;

      ConfigChildWidget *childWidget = configWidgets[scopedName];

      switch (field->cpp_type())
      {
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        {
          QDoubleSpinBox *valueSpinBox =
              qobject_cast<QDoubleSpinBox *>(childWidget->widgets[0]);
          ref->SetDouble(_msg, field, valueSpinBox->value());
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        {
          QDoubleSpinBox *valueSpinBox =
              qobject_cast<QDoubleSpinBox *>(childWidget->widgets[0]);
          ref->SetFloat(_msg, field, valueSpinBox->value());
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        {
          QSpinBox *valueSpinBox =
              qobject_cast<QSpinBox *>(childWidget->widgets[0]);
          ref->SetInt64(_msg, field, valueSpinBox->value());
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        {
          QSpinBox *valueSpinBox =
              qobject_cast<QSpinBox *>(childWidget->widgets[0]);
          ref->SetUInt64(_msg, field, valueSpinBox->value());
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        {
          QSpinBox *valueSpinBox =
              qobject_cast<QSpinBox *>(childWidget->widgets[0]);
          ref->SetInt32(_msg, field, valueSpinBox->value());
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        {
          QSpinBox *valueSpinBox =
              qobject_cast<QSpinBox *>(childWidget->widgets[0]);
          ref->SetUInt32(_msg, field, valueSpinBox->value());
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        {
          QRadioButton *valueRadioButton =
              qobject_cast<QRadioButton *>(childWidget->widgets[0]);
          ref->SetBool(_msg, field, valueRadioButton->isChecked());
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
          QLineEdit *valueLineEdit =
              qobject_cast<QLineEdit *>(childWidget->widgets[0]);
          ref->SetString(_msg, field, valueLineEdit->text().toStdString());
          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
          google::protobuf::Message *valueMsg =
              (ref->MutableMessage(_msg, field));

          // update geometry msg field
          if (field->message_type()->name() == "Geometry")
          {
            // manually retrieve values from widgets in order to update
            // the message fields.
            QComboBox *valueComboBox =
                qobject_cast<QComboBox *>(childWidget->widgets[0]);
            std::string geomType = valueComboBox->currentText().toStdString();

            const google::protobuf::Descriptor *valueDescriptor =
                valueMsg->GetDescriptor();
            const google::protobuf::Reflection *geomReflection =
                valueMsg->GetReflection();
            const google::protobuf::FieldDescriptor *typeField =
                valueDescriptor->FindFieldByName("type");
            const google::protobuf::EnumDescriptor *typeEnumDescriptor =
                typeField->enum_type();

            if (geomType == "box")
            {
              double sizeX = qobject_cast<QDoubleSpinBox *>(
                  childWidget->widgets[1])->value();
              double sizeY = qobject_cast<QDoubleSpinBox *>(
                  childWidget->widgets[2])->value();
              double sizeZ = qobject_cast<QDoubleSpinBox *>(
                  childWidget->widgets[3])->value();
              math::Vector3 boxSize(sizeX, sizeY, sizeZ);

              // set type
              const google::protobuf::EnumValueDescriptor *geometryType =
                  typeEnumDescriptor->FindValueByName("BOX");
              geomReflection->SetEnum(valueMsg, typeField, geometryType);

              // set dimensions
              const google::protobuf::FieldDescriptor *geomFieldDescriptor =
                valueDescriptor->FindFieldByName(geomType);
              google::protobuf::Message *geomValueMsg =
                  geomReflection->MutableMessage(valueMsg, geomFieldDescriptor);
              google::protobuf::Message *geomDimensionMsg =
                  geomValueMsg->GetReflection()->MutableMessage(geomValueMsg,
                  geomValueMsg->GetDescriptor()->field(0));
              this->UpdateVector3Msg(geomDimensionMsg, boxSize);
            }
            else if (geomType == "cylinder")
            {
              double radius = qobject_cast<QDoubleSpinBox *>(
                  childWidget->widgets[4])->value();
              double length = qobject_cast<QDoubleSpinBox *>(
                  childWidget->widgets[5])->value();

              // set type
              const google::protobuf::EnumValueDescriptor *geometryType =
                  typeEnumDescriptor->FindValueByName("CYLINDER");
              geomReflection->SetEnum(valueMsg, typeField, geometryType);

              // set radius and length
              const google::protobuf::FieldDescriptor *geomFieldDescriptor =
                valueDescriptor->FindFieldByName(geomType);
              google::protobuf::Message *geomValueMsg =
                  geomReflection->MutableMessage(valueMsg, geomFieldDescriptor);

              const google::protobuf::FieldDescriptor *geomRadiusField =
                  geomValueMsg->GetDescriptor()->field(0);
              geomValueMsg->GetReflection()->SetDouble(geomValueMsg,
                  geomRadiusField, radius);
              const google::protobuf::FieldDescriptor *geomLengthField =
                  geomValueMsg->GetDescriptor()->field(1);
              geomValueMsg->GetReflection()->SetDouble(geomValueMsg,
                  geomLengthField, length);
            }
            else if (geomType == "sphere")
            {
              double radius = qobject_cast<QDoubleSpinBox *>(
                  childWidget->widgets[4])->value();

              // set type
              const google::protobuf::EnumValueDescriptor *geometryType =
                  typeEnumDescriptor->FindValueByName("SPHERE");
              geomReflection->SetEnum(valueMsg, typeField, geometryType);

              // set radius and length
              const google::protobuf::FieldDescriptor *geomFieldDescriptor =
                valueDescriptor->FindFieldByName(geomType);
              google::protobuf::Message *geomValueMsg =
                  geomReflection->MutableMessage(valueMsg, geomFieldDescriptor);

              const google::protobuf::FieldDescriptor *geomRadiusField =
                  geomValueMsg->GetDescriptor()->field(0);
              geomValueMsg->GetReflection()->SetDouble(geomValueMsg,
                  geomRadiusField, radius);
            }
          }
          // update pose msg field
          else if (field->message_type()->name() == "Pose")
          {

            const google::protobuf::Descriptor *valueDescriptor =
                valueMsg->GetDescriptor();
            int valueMsgFieldCount = valueDescriptor->field_count();

            // loop through the message fields to update:
            // a vector3d field (position)
            // and quaternion field (orientation)
            for (int j = 0; j < valueMsgFieldCount ; ++j)
            {
              const google::protobuf::FieldDescriptor *valueField =
                  valueDescriptor->field(j);

              if (valueField->cpp_type() !=
                  google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                continue;

              if (valueField->message_type()->name() == "Vector3d")
              {
                // pos
                google::protobuf::Message *posValueMsg =
                    valueMsg->GetReflection()->MutableMessage(
                    valueMsg, valueField);
                std::vector<double> values;
                for (unsigned int k = 0; k < 3; ++k)
                {
                  QDoubleSpinBox *valueSpinBox =
                      qobject_cast<QDoubleSpinBox *>(childWidget->widgets[k]);
                  values.push_back(valueSpinBox->value());
                }
                math::Vector3 vec3(values[0], values[1], values[2]);
                this->UpdateVector3Msg(posValueMsg, vec3);
              }
              else if (valueField->message_type()->name() == "Quaternion")
              {
                // rot
                google::protobuf::Message *quatValueMsg =
                    valueMsg->GetReflection()->MutableMessage(
                    valueMsg, valueField);
                std::vector<double> rotValues;
                for (unsigned int k = 3; k < 6; ++k)
                {
                  QDoubleSpinBox *valueSpinBox =
                      qobject_cast<QDoubleSpinBox *>(childWidget->widgets[k]);
                  rotValues.push_back(valueSpinBox->value());
                }
                math::Quaternion quat(rotValues[0], rotValues[1], rotValues[2]);

                std::vector<double> quatValues;
                quatValues.push_back(quat.x);
                quatValues.push_back(quat.y);
                quatValues.push_back(quat.z);
                quatValues.push_back(quat.w);
                const google::protobuf::Descriptor *quatValueDescriptor =
                    quatValueMsg->GetDescriptor();
                for (unsigned int k = 0; k < quatValues.size(); ++k)
                {
                  const google::protobuf::FieldDescriptor *quatValueField =
                      quatValueDescriptor->field(k);
                  quatValueMsg->GetReflection()->SetDouble(quatValueMsg,
                      quatValueField, quatValues[k]);
                }
              }
            }
          }
          else if (field->message_type()->name() == "Vector3d")
          {
            std::vector<double> values;
            for (unsigned int j = 0; j < childWidget->widgets.size(); ++j)
            {
              QDoubleSpinBox *valueSpinBox =
                  qobject_cast<QDoubleSpinBox *>(childWidget->widgets[j]);
              values.push_back(valueSpinBox->value());
            }
            math::Vector3 vec3(values[0], values[1], values[2]);
            this->UpdateVector3Msg(valueMsg, vec3);
          }
          else if (field->message_type()->name() == "Color")
          {
            const google::protobuf::Descriptor *valueDescriptor =
                valueMsg->GetDescriptor();
            for (unsigned int j = 0; j < childWidget->widgets.size(); ++j)
            {
              QDoubleSpinBox *valueSpinBox =
                  qobject_cast<QDoubleSpinBox *>(childWidget->widgets[j]);
              const google::protobuf::FieldDescriptor *valueField =
                  valueDescriptor->field(j);
              valueMsg->GetReflection()->SetFloat(valueMsg, valueField,
                  valueSpinBox->value());
            }
          }
          else
          {
            // update the message fields recursively
            this->UpdateMsg(valueMsg, scopedName);
          }

          break;
        }
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
          // todo update enum fields.
          break;
        default:
          break;
      }
    }
  }
}

/////////////////////////////////////////////////
void ConfigWidget::UpdateVector3Msg(google::protobuf::Message *_msg,
    const math::Vector3 _value)
{
  const google::protobuf::Descriptor *valueDescriptor =
      _msg->GetDescriptor();

  std::vector<double> values;
  values.push_back(_value.x);
  values.push_back(_value.y);
  values.push_back(_value.z);

  for (unsigned int i = 0; i < 3; ++i)
  {
    const google::protobuf::FieldDescriptor *valueField =
        valueDescriptor->field(i);
    _msg->GetReflection()->SetDouble(_msg, valueField, values[i]);
  }
}


/////////////////////////////////////////////////
void ConfigWidget::UpdateIntWidget(ConfigChildWidget *_widget,  int _value)
{
  if (_widget->widgets.size() == 1u)
  {
    qobject_cast<QSpinBox *>(_widget->widgets[0])->setValue(_value);
  }
  else
  {
    gzerr << "Error updating Int Config widget" << std::endl;
  }
}

/////////////////////////////////////////////////
void ConfigWidget::UpdateUIntWidget(ConfigChildWidget *_widget,
    unsigned int _value)
{
  if (_widget->widgets.size() == 1u)
  {
    qobject_cast<QSpinBox *>(_widget->widgets[0])->setValue(_value);
  }
  else
  {
    gzerr << "Error updating UInt Config widget" << std::endl;
  }
}

/////////////////////////////////////////////////
void ConfigWidget::UpdateDoubleWidget(ConfigChildWidget *_widget, double _value)
{
  if (_widget->widgets.size() == 1u)
  {
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[0])->setValue(_value);
  }
  else
  {
    gzerr << "Error updating Double Config widget" << std::endl;
  }
}

/////////////////////////////////////////////////
void ConfigWidget::UpdateStringWidget(ConfigChildWidget *_widget,
    const std::string &_value)
{
  if (_widget->widgets.size() == 1u)
  {
    qobject_cast<QLineEdit *>(_widget->widgets[0])->setText(tr(_value.c_str()));
  }
  else
  {
    gzerr << "Error updating String Config Widget" << std::endl;
  }
}

/////////////////////////////////////////////////
void ConfigWidget::UpdateBoolWidget(ConfigChildWidget *_widget, bool _value)
{
  if (_widget->widgets.size() == 2u)
  {
    qobject_cast<QRadioButton *>(_widget->widgets[0])->setChecked(_value);
    qobject_cast<QRadioButton *>(_widget->widgets[1])->setChecked(!_value);
  }
  else
  {
    gzerr << "Error updating Bool Config widget" << std::endl;
  }
}

/////////////////////////////////////////////////
void ConfigWidget::UpdateVector3Widget(ConfigChildWidget *_widget,
    const math::Vector3 &_vec)
{
  if (_widget->widgets.size() == 3u)
  {
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[0])->setValue(_vec.x);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[1])->setValue(_vec.y);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[2])->setValue(_vec.z);
  }
  else
  {
    gzerr << "Error updating Vector3 Config widget" << std::endl;
  }
}

/////////////////////////////////////////////////
void ConfigWidget::UpdateColorWidget(ConfigChildWidget *_widget,
    const common::Color &_color)
{
  if (_widget->widgets.size() == 4u)
  {
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[0])->setValue(_color.r);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[1])->setValue(_color.g);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[2])->setValue(_color.b);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[3])->setValue(_color.a);
  }
  else
  {
    gzerr << "Error updating Color Config widget" << std::endl;
  }
}

/////////////////////////////////////////////////
void ConfigWidget::UpdatePoseWidget(ConfigChildWidget *_widget,
    const math::Pose &_pose)
{
  if (_widget->widgets.size() == 6u)
  {
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[0])->setValue(_pose.pos.x);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[1])->setValue(_pose.pos.y);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[2])->setValue(_pose.pos.z);

    math::Vector3 rot = _pose.rot.GetAsEuler();
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[3])->setValue(rot.x);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[4])->setValue(rot.y);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[5])->setValue(rot.z);
  }
  else
  {
    gzerr << "Error updating Pose Config widget" << std::endl;
  }
}

/////////////////////////////////////////////////
void ConfigWidget::UpdateGeometryWidget(ConfigChildWidget *_widget,
    const std::string &_value, const math::Vector3 &_dimensions)
{
  if (_widget->widgets.size() != 6u)
  {
    gzerr << "Error updating Geometry Config widget " << std::endl;
    return;
  }

  QComboBox * valueComboBox = qobject_cast<QComboBox *>(_widget->widgets[0]);
  int index = valueComboBox->findText(tr(_value.c_str()));

  if (index < 0)
  {
    gzerr << "Error updating Geometry Config widget: '" << _value <<
      "' not found" << std::endl;
    return;
  }

  qobject_cast<QComboBox *>(_widget->widgets[0])->setCurrentIndex(index);

  if (_value == "box")
  {
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[1])->setValue(
        _dimensions.x);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[2])->setValue(
        _dimensions.y);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[3])->setValue(
        _dimensions.z);
  }
  else if (_value == "cylinder")
  {
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[4])->setValue(
        _dimensions.x/2.0);
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[5])->setValue(
        _dimensions.z);
  }
  else if (_value == "sphere")
  {
    qobject_cast<QDoubleSpinBox *>(_widget->widgets[4])->setValue(
        _dimensions.x/2.0);
  }
}


/////////////////////////////////////////////////
void ConfigWidget::OnItemSelection(QTreeWidgetItem *_item,
                                         int /*_column*/)
{
  if (_item && _item->childCount() > 0)
    _item->setExpanded(!_item->isExpanded());
}

/////////////////////////////////////////////////
void GroupWidget::Toggle()
{
  if (!this->childWidget)
    return;

  this->childWidget->setVisible(!this->childWidget->isVisible());
}

/////////////////////////////////////////////////
void GeometryConfigWidget::GeometryChanged(const QString _text)
{
  QWidget *widget= qobject_cast<QWidget *>(QObject::sender());

  if (widget)
  {
    std::string textStr = _text.toStdString();
    if (textStr == "box")
    {
      this->geomDimensionWidget->setCurrentIndex(0);
    }
    else if (textStr == "cylinder")
    {
      this->geomDimensionWidget->setCurrentIndex(1);
      this->geomLengthSpinBox->show();
      this->geomLengthLabel->show();
    }
    else if (textStr == "sphere")
    {
      this->geomDimensionWidget->setCurrentIndex(1);
      this->geomLengthSpinBox->hide();
      this->geomLengthLabel->hide();
    }
  }
}
