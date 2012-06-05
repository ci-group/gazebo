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
/* Desc: Map shape
 * Author: Nate Koenig
*/

#include <string.h>
#include <math.h>

#include "common/Image.hh"
#include "common/Exception.hh"

#include "physics/World.hh"
#include "physics/PhysicsEngine.hh"
#include "physics/BoxShape.hh"
#include "physics/Collision.hh"
#include "physics/MapShape.hh"

using namespace gazebo;
using namespace physics;


unsigned int MapShape::collisionCounter = 0;

//////////////////////////////////////////////////
MapShape::MapShape(CollisionPtr _parent)
    : Shape(_parent)
{
  this->AddType(Base::MAP_SHAPE);

  this->root = new QuadNode(NULL);
}

//////////////////////////////////////////////////
MapShape::~MapShape()
{
  delete this->root;
  delete this->mapImage;
  this->mapImage = NULL;
}

//////////////////////////////////////////////////
void MapShape::Update()
{
}

//////////////////////////////////////////////////
void MapShape::Load(sdf::ElementPtr _sdf)
{
  Base::Load(_sdf);

  std::string imageFilename = _sdf->GetValueString("filename");

  // Make sure they are ok
  if (_sdf->GetValueDouble("scale") <= 0)
    _sdf->GetAttribute("scale")->Set(0.1);
  if (this->sdf->GetValueInt("threshold") <= 0)
    _sdf->GetAttribute("threshold")->Set(200);
  if (this->sdf->GetValueDouble("height") <= 0)
    _sdf->GetAttribute("height")->Set(1.0);

  // Load the image
  this->mapImage = new common::Image();
  this->mapImage->Load(imageFilename);

  if (!this->mapImage->Valid())
    gzthrow(std::string("Unable to open image file[") + imageFilename + "]");
}

//////////////////////////////////////////////////
void MapShape::Init()
{
  this->root->x = 0;
  this->root->y = 0;

  this->root->width = this->mapImage->GetWidth();
  this->root->height = this->mapImage->GetHeight();

  this->BuildTree(this->root);

  this->merged = true;
  while (this->merged)
  {
    this->merged = false;
    this->ReduceTree(this->root);
  }

  this->CreateBoxes(this->root);
}

//////////////////////////////////////////////////
void MapShape::FillShapeMsg(msgs::Geometry &_msg)
{
  _msg.set_type(msgs::Geometry::IMAGE);
  _msg.mutable_image()->set_filename(this->GetFilename());
  _msg.mutable_image()->set_scale(this->GetScale());
  _msg.mutable_image()->set_threshold(this->GetThreshold());
  _msg.mutable_image()->set_height(this->GetHeight());
  _msg.mutable_image()->set_granularity(this->GetGranularity());
}


//////////////////////////////////////////////////
std::string MapShape::GetFilename() const
{
  return this->sdf->GetValueString("filename");
}

//////////////////////////////////////////////////
double MapShape::GetScale() const
{
  return this->sdf->GetValueDouble("scale");
}

//////////////////////////////////////////////////
int MapShape::GetThreshold() const
{
  return this->sdf->GetValueInt("threshold");
}

//////////////////////////////////////////////////
double MapShape::GetHeight() const
{
  return this->sdf->GetValueDouble("height");
}

//////////////////////////////////////////////////
int MapShape::GetGranularity() const
{
  return this->sdf->GetValueInt("granularity");
}

//////////////////////////////////////////////////
void MapShape::CreateBoxes(QuadNode * /*_node*/)
{
  /*TODO: fix this to use SDF
  if (node->leaf)
  {
    if (!node->valid || !node->occupied)
      return;

    std::ostringstream stream;

    // Create the box geometry
    CollisionPtr collision = this->GetWorld()->GetPhysicsEngine()->CreateCollision("box", this->collisionParent->GetLink());
    collision->SetSaveable(false);

    stream << "<gazebo:world xmlns:gazebo =\"http://playerstage.sourceforge.net/gazebo/xmlschema/#gz\" xmlns:collision =\"http://playerstage.sourceforge.net/gazebo/xmlschema/#collision\">";

    float x = (node->x + node->width / 2.0) * this->sdf->GetValueDouble("scale");
    float y = (node->y + node->height / 2.0) * this->sdf->GetValueDouble("scale");
    float z = this->sdf->GetValueDouble("height") / 2.0;
    float xSize = (node->width) * this->sdf->GetValueDouble("scale");
    float ySize = (node->height) * this->sdf->GetValueDouble("scale");
    float zSize = this->sdf->GetValueDouble("height");

    char collisionName[256];
    sprintf(collisionName, "map_collision_%d", collisionCounter++);

    stream << "<collision:box name ='" << collisionName << "'>";
    stream << "  <xyz>" << x << " " << y << " " << z << "</xyz>";
    stream << "  <rpy>0 0 0</rpy>";
    stream << "  <size>" << xSize << " " << ySize << " " << zSize << "</size>";
    stream << "  <static>true</static>";
    stream << "  <visual>";
    stream << "    <mesh>unit_box</mesh>";
    stream << "    <material>" << this->materialP->GetValue() << "</material>";
    stream << "    <size>" << xSize << " "<< ySize << " " << zSize << "</size>";
    stream << "  </visual>";
    stream << "</collision:box>";
    stream << "</gazebo:world>";

    boxConfig->LoadString(stream.str());

    collision->SetStatic(true);
    collision->Load(boxConfig->GetRootNode()->GetChild());

    delete boxConfig;
  }
  else
  {
    std::deque<QuadNode*>::iterator iter;
    for (iter = node->children.begin(); iter != node->children.end(); iter++)
    {
      this->CreateBoxes(*iter);
    }
  }
  */
}

//////////////////////////////////////////////////
void MapShape::ReduceTree(QuadNode *_node)
{
  std::deque<QuadNode*>::iterator iter;

  if (!_node->valid)
    return;

  if (!_node->leaf)
  {
    unsigned int count = 0;
    int size = _node->children.size();

    for (int i = 0; i < size; i++)
    {
      if (_node->children[i]->valid)
      {
        this->ReduceTree(_node->children[i]);
      }
      if (_node->children[i]->leaf)
        count++;
    }

    if (_node->parent && count == _node->children.size())
    {
      for (iter = _node->children.begin();
           iter != _node->children.end(); ++iter)
      {
        _node->parent->children.push_back(*iter);
        (*iter)->parent = _node->parent;
      }
      _node->valid = false;
    }
    else
    {
      bool done = false;
      while (!done)
      {
        done = true;
        for (iter = _node->children.begin();
             iter != _node->children.end(); ++iter)
        {
          if (!(*iter)->valid)
          {
            _node->children.erase(iter, iter+1);
            done = false;
            break;
          }
        }
      }
    }
  }
  else
  {
    this->Merge(_node, _node->parent);
  }
}

//////////////////////////////////////////////////
void MapShape::Merge(QuadNode *_nodeA, QuadNode *_nodeB)
{
  std::deque<QuadNode*>::iterator iter;

  if (!_nodeB)
    return;

  if (_nodeB->leaf)
  {
    if (_nodeB->occupied != _nodeA->occupied)
      return;

    if (_nodeB->x == _nodeA->x + _nodeA->width &&
         _nodeB->y == _nodeA->y &&
         _nodeB->height == _nodeA->height)
    {
      _nodeA->width += _nodeB->width;
      _nodeB->valid = false;
      _nodeA->valid = true;

      this->merged = true;
    }

    if (_nodeB->x == _nodeA->x &&
        _nodeB->width == _nodeA->width &&
        _nodeB->y == _nodeA->y + _nodeA->height)
    {
      _nodeA->height += _nodeB->height;
      _nodeB->valid = false;
      _nodeA->valid = true;

      this->merged = true;
    }
  }
  else
  {
    for (iter = _nodeB->children.begin();
         iter != _nodeB->children.end(); ++iter)
    {
      if ((*iter)->valid)
      {
        this->Merge(_nodeA, (*iter));
      }
    }
  }
}


//////////////////////////////////////////////////
void MapShape::BuildTree(QuadNode *_node)
{
  QuadNode *newNode = NULL;
  unsigned int freePixels, occPixels;

  this->GetPixelCount(_node->x, _node->y, _node->width, _node->height,
                      freePixels, occPixels);

  // int diff = labs(freePixels - occPixels);

  if (static_cast<int>(_node->width*_node->height) >
      this->sdf->GetValueInt("granularity"))
  {
    float newX, newY;
    float newW, newH;

    newY = _node->y;
    newW = _node->width / 2.0;
    newH = _node->height / 2.0;

    // Create the children for the node
    for (int i = 0; i < 2; i++)
    {
      newX = _node->x;

      for (int j = 0; j < 2; j++)
      {
        newNode = new QuadNode(_node);
        newNode->x = (unsigned int)newX;
        newNode->y = (unsigned int)newY;

        if (j == 0)
          newNode->width = (unsigned int)floor(newW);
        else
          newNode->width = (unsigned int)ceil(newW);

        if (i == 0)
          newNode->height = (unsigned int)floor(newH);
        else
          newNode->height = (unsigned int)ceil(newH);

        _node->children.push_back(newNode);

        this->BuildTree(newNode);

        newX += newNode->width;

        if (newNode->width == 0 || newNode->height == 0)
          newNode->valid = false;
      }

      if (i == 0)
        newY += floor(newH);
      else
        newY += ceil(newH);
    }

    // _node->occupied = true;
    _node->occupied = false;
    _node->leaf = false;
  }
  else if (occPixels == 0)
  {
    _node->occupied = false;
    _node->leaf = true;
  }
  else
  {
    _node->occupied = true;
    _node->leaf = true;
  }
}

//////////////////////////////////////////////////
void MapShape::GetPixelCount(unsigned int xStart, unsigned int yStart,
                                 unsigned int width, unsigned int height,
                                 unsigned int &freePixels,
                                 unsigned int &occPixels)
{
  common::Color pixColor;
  unsigned char v;
  unsigned int x, y;

  freePixels = occPixels = 0;

  for (y = yStart; y < yStart + height; y++)
  {
    for (x = xStart; x < xStart + width; x++)
    {
      pixColor = this->mapImage->GetPixel(x, y);

      v = (unsigned char)(255 *
          ((pixColor.R() + pixColor.G() + pixColor.B()) / 3.0));
      // if (this->sdf->GetValueBool("negative"))
        // v = 255 - v;

      if (v > this->sdf->GetValueInt("threshold"))
        freePixels++;
      else
        occPixels++;
    }
  }
}

//////////////////////////////////////////////////
void MapShape::ProcessMsg(const msgs::Geometry & /*_msg*/)
{
  gzerr << "TODO: not implement yet.";
}

