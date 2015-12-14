/*
 * Copyright (C) 2012-2015 Open Source Robotics Foundation
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

#include "gazebo/common/Exception.hh"

#include "gazebo/rendering/Visual.hh"

#include "gazebo/gui/building/BuildingEditorEvents.hh"
#include "gazebo/gui/building/BuildingMaker.hh"
#include "gazebo/gui/building/BuildingModelManip.hh"
#include "gazebo/gui/building/BuildingModelManipPrivate.hh"

using namespace gazebo;
using namespace gui;

/////////////////////////////////////////////////
BuildingModelManip::BuildingModelManip()
  : dataPtr(new BuildingModelManipPrivate)
{
  this->dataPtr->parent = NULL;
  this->dataPtr->level = 0;

  this->dataPtr->connections.push_back(
      gui::editor::Events::ConnectChangeBuildingLevel(
      std::bind(&BuildingModelManip::OnChangeLevel, this,
      std::placeholders::_1)));
}

/////////////////////////////////////////////////
BuildingModelManip::~BuildingModelManip()
{
  this->DetachFromParent();
}

/////////////////////////////////////////////////
void BuildingModelManip::SetName(const std::string &_name)
{
  this->dataPtr->name = _name;
}

/////////////////////////////////////////////////
void BuildingModelManip::SetVisual(const rendering::VisualPtr &_visual)
{
  this->dataPtr->visual = _visual;
}

/////////////////////////////////////////////////
std::string BuildingModelManip::GetName() const
{
  return this->dataPtr->name;
}

/////////////////////////////////////////////////
rendering::VisualPtr BuildingModelManip::GetVisual() const
{
  return this->dataPtr->visual;
}

/////////////////////////////////////////////////
double BuildingModelManip::GetTransparency() const
{
  return this->dataPtr->transparency;
}

/////////////////////////////////////////////////
common::Color BuildingModelManip::GetColor() const
{
  return this->dataPtr->color;
}

/////////////////////////////////////////////////
std::string BuildingModelManip::GetTexture() const
{
  return this->dataPtr->texture;
}

/////////////////////////////////////////////////
void BuildingModelManip::SetMaker(BuildingMaker *_maker)
{
  this->dataPtr->maker = _maker;
}

/////////////////////////////////////////////////
BuildingModelManip *BuildingModelManip::GetParent() const
{
  return this->dataPtr->parent;
}

/////////////////////////////////////////////////
void BuildingModelManip::OnSizeChanged(double _width, double _depth,
    double _height)
{
  this->dataPtr->size =
      BuildingMaker::ConvertSize(_width, _depth, _height).Ign();
  double dScaleZ = this->dataPtr->visual->GetScale().Ign().Z() -
                   this->dataPtr->size.Z();
  this->dataPtr->visual->SetScale(this->dataPtr->size);
  auto originalPos = this->dataPtr->visual->GetPosition().Ign();
  auto newPos = originalPos - ignition::math::Vector3d(0, 0, dScaleZ/2.0);
  this->dataPtr->visual->SetPosition(newPos);
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::AttachManip(BuildingModelManip *_manip)
{
  if (!_manip->IsAttached())
  {
    _manip->SetAttachedTo(this);
    this->dataPtr->attachedManips.push_back(_manip);
  }
}

/////////////////////////////////////////////////
void BuildingModelManip::DetachManip(BuildingModelManip *_manip)
{
  if (_manip)
  {
    auto it = std::remove(this->dataPtr->attachedManips.begin(),
                          this->dataPtr->attachedManips.end(), _manip);
    if (it != this->dataPtr->attachedManips.end())
    {
      _manip->DetachFromParent();
      this->dataPtr->attachedManips.erase(it,
          this->dataPtr->attachedManips.end());
    }
  }
}

/////////////////////////////////////////////////
void BuildingModelManip::DetachFromParent()
{
  if (this->dataPtr->parent)
  {
    BuildingModelManip *tmp = this->dataPtr->parent;
    this->dataPtr->parent = NULL;
    tmp->DetachManip(this);
  }
}

/////////////////////////////////////////////////
void BuildingModelManip::SetAttachedTo(BuildingModelManip *_parent)
{
  if (this->IsAttached())
  {
    gzerr << this->dataPtr->name << " is already attached to a parent \n";
    return;
  }
  this->dataPtr->parent = _parent;
}

/////////////////////////////////////////////////
BuildingModelManip *BuildingModelManip::GetAttachedManip(
    unsigned int _index) const
{
  if (_index >= this->dataPtr->attachedManips.size())
    gzthrow("Index too large");

  return this->dataPtr->attachedManips[_index];
}

/////////////////////////////////////////////////
unsigned int BuildingModelManip::GetAttachedManipCount() const
{
  return this->dataPtr->attachedManips.size();
}

/////////////////////////////////////////////////
bool BuildingModelManip::IsAttached() const
{
  return (this->dataPtr->parent != NULL);
}

/////////////////////////////////////////////////
void BuildingModelManip::OnPoseChanged(double _x, double _y, double _z,
    double _roll, double _pitch, double _yaw)
{
  this->SetPose(_x, _y, _z, _roll, _pitch, _yaw);
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnPoseOriginTransformed(double _x, double _y,
    double _z, double _roll, double _pitch, double _yaw)
{
  // Handle translations, currently used by polylines
  auto trans = BuildingMaker::ConvertPose(_x, -_y, _z, _roll, _pitch,
      _yaw).Ign();

  auto oldPose = this->dataPtr->visual->GetParent()->GetWorldPose().Ign();

  this->dataPtr->visual->GetParent()->SetWorldPose(oldPose + trans);
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnPositionChanged(double _x, double _y, double _z)
{
  double scaledX = BuildingMaker::Convert(_x);
  double scaledY = BuildingMaker::Convert(-_y);
  double scaledZ = BuildingMaker::Convert(_z);

  this->dataPtr->visual->GetParent()->SetWorldPosition(ignition::math::Vector3d(
      scaledX, scaledY, scaledZ));
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnWidthChanged(double _width)
{
  double scaledWidth = BuildingMaker::Convert(_width);
  this->dataPtr->size = this->dataPtr->visual->GetScale().Ign();
  this->dataPtr->size.X(scaledWidth);
  this->dataPtr->visual->SetScale(this->dataPtr->size);
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnDepthChanged(double _depth)
{
  double scaledDepth = BuildingMaker::Convert(_depth);
  this->dataPtr->size = this->dataPtr->visual->GetScale().Ign();
  this->dataPtr->size.Y(scaledDepth);
  this->dataPtr->visual->SetScale(this->dataPtr->size);
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnHeightChanged(double _height)
{
  double scaledHeight = BuildingMaker::Convert(_height);
  this->dataPtr->size = this->dataPtr->visual->GetScale().Ign();
  this->dataPtr->size.Z(scaledHeight);
  auto dScale = this->dataPtr->visual->GetScale().Ign() - this->dataPtr->size;
  auto originalPos = this->dataPtr->visual->GetPosition().Ign();
  this->dataPtr->visual->SetScale(this->dataPtr->size);

  auto newPos = originalPos - ignition::math::Vector3d(0, 0, dScale.Z()/2.0);

  this->dataPtr->visual->SetPosition(newPos);
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnPosXChanged(double _posX)
{
  auto visualPose = this->dataPtr->visual->GetParent()->GetWorldPose().Ign();
  double scaledX = BuildingMaker::Convert(_posX);
  visualPose.Pos().X(scaledX);
  this->dataPtr->visual->GetParent()->SetWorldPosition(visualPose.Pos());
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnPosYChanged(double _posY)
{
  auto visualPose = this->dataPtr->visual->GetParent()->GetWorldPose().Ign();
  double scaledY = BuildingMaker::Convert(_posY);
  visualPose.Pos().Y(-scaledY);
  this->dataPtr->visual->GetParent()->SetWorldPosition(visualPose.Pos());
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnPosZChanged(double _posZ)
{
  auto visualPose = this->dataPtr->visual->GetParent()->GetWorldPose().Ign();
  double scaledZ = BuildingMaker::Convert(_posZ);
  visualPose.Pos().Z(scaledZ);
  this->dataPtr->visual->GetParent()->SetWorldPosition(visualPose.Pos());
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnYawChanged(double _yaw)
{
  double newYaw = BuildingMaker::ConvertAngle(_yaw);
  auto angles = this->dataPtr->visual->GetRotation().GetAsEuler().Ign();
  angles.Z(-newYaw);
  this->dataPtr->visual->GetParent()->SetRotation(
      ignition::math::Quaterniond(angles));
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnRotationChanged(double _roll, double _pitch,
    double _yaw)
{
  this->SetRotation(_roll, _pitch, _yaw);
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnLevelChanged(int _level)
{
  this->SetLevel(_level);
}

/////////////////////////////////////////////////
void BuildingModelManip::OnColorChanged(QColor _color)
{
  this->SetColor(_color);
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnTextureChanged(QString _texture)
{
  this->SetTexture(_texture);
  this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnTransparencyChanged(float _transparency)
{
  this->SetTransparency(_transparency);
  // For now transparency is used only to aid in the preview and doesn't affect
  // the saved building
  // this->dataPtr->maker->BuildingChanged();
}

/////////////////////////////////////////////////
void BuildingModelManip::OnDeleted()
{
  this->dataPtr->maker->RemovePart(this->dataPtr->name);
}

/////////////////////////////////////////////////
void BuildingModelManip::SetPose(double _x, double _y, double _z,
    double _roll, double _pitch, double _yaw)
{
  this->SetPosition(_x, _y, _z);
  this->SetRotation(_roll, _pitch, _yaw);
}

/////////////////////////////////////////////////
void BuildingModelManip::SetPosition(double _x, double _y, double _z)
{
  double scaledX = BuildingMaker::Convert(_x);
  double scaledY = BuildingMaker::Convert(-_y);
  double scaledZ = BuildingMaker::Convert(_z);
  this->dataPtr->visual->GetParent()->SetWorldPosition(
      ignition::math::Vector3d(scaledX, scaledY, scaledZ));
}

/////////////////////////////////////////////////
void BuildingModelManip::SetRotation(double _roll, double _pitch, double _yaw)
{
  double rollRad = BuildingMaker::ConvertAngle(_roll);
  double pitchRad = BuildingMaker::ConvertAngle(_pitch);
  double yawRad = BuildingMaker::ConvertAngle(_yaw);

  this->dataPtr->visual->GetParent()->SetRotation(
      math::Quaternion(rollRad, pitchRad, -yawRad));
}

/////////////////////////////////////////////////
void BuildingModelManip::SetSize(double _width, double _depth, double _height)
{
  this->dataPtr->size =
      BuildingMaker::ConvertSize(_width, _depth, _height).Ign();

  auto dScale = this->dataPtr->visual->GetScale() - this->dataPtr->size;

  auto originalPos = this->dataPtr->visual->GetPosition();
  this->dataPtr->visual->SetPosition(ignition::math::Vector3d(0, 0, 0));
  this->dataPtr->visual->SetScale(this->dataPtr->size);

  // adjust position due to difference in pivot points
  auto newPos = originalPos - dScale/2.0;

  this->dataPtr->visual->SetPosition(newPos);
}

/////////////////////////////////////////////////
void BuildingModelManip::SetColor(QColor _color)
{
  common::Color newColor(_color.red(), _color.green(), _color.blue());
  this->dataPtr->color = newColor;
  this->dataPtr->visual->SetAmbient(this->dataPtr->color);
  this->dataPtr->maker->BuildingChanged();
  emit ColorChanged(_color);
}

/////////////////////////////////////////////////
void BuildingModelManip::SetTexture(QString _texture)
{
  // TODO For now setting existing material scripts.
  // Add support for custom textures.
  this->dataPtr->texture = "Gazebo/Grey";
  if (_texture == ":wood.jpg")
    this->dataPtr->texture = "Gazebo/Wood";
  else if (_texture == ":tiles.jpg")
    this->dataPtr->texture = "Gazebo/CeilingTiled";
  else if (_texture == ":bricks.png")
    this->dataPtr->texture = "Gazebo/Bricks";

  // BuildingModelManip and BuildingMaker handle material names,
  // Inspectors and palette handle thumbnail uri
  this->dataPtr->visual->SetMaterial(this->dataPtr->texture);
  // Must set color after texture otherwise it gets overwritten
  this->dataPtr->visual->SetAmbient(this->dataPtr->color);
  this->dataPtr->maker->BuildingChanged();
  emit TextureChanged(_texture);
}

/////////////////////////////////////////////////
void BuildingModelManip::SetTransparency(float _transparency)
{
  this->dataPtr->transparency = _transparency;
  this->dataPtr->visual->SetTransparency(this->dataPtr->transparency);
}

/////////////////////////////////////////////////
void BuildingModelManip::SetVisible(bool _visible)
{
  this->dataPtr->visual->SetVisible(_visible);
}

/////////////////////////////////////////////////
void BuildingModelManip::SetLevel(const int _level)
{
  this->dataPtr->level = _level;
}

/////////////////////////////////////////////////
int BuildingModelManip::GetLevel() const
{
  return this->dataPtr->level;
}

/////////////////////////////////////////////////
void BuildingModelManip::OnChangeLevel(int _level)
{
  if (this->dataPtr->level > _level)
    this->SetVisible(false);
  else if (this->dataPtr->level < _level)
  {
    this->SetVisible(true);
    this->SetTransparency(0.0);
  }
  else
  {
    this->SetVisible(true);
    this->SetTransparency(0.4);
  }
}
