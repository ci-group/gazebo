/*
 * Copyright 2012 Open Source Robotics Foundation
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

#include "gazebo/transport/Transport.hh"
#include "gazebo/transport/Node.hh"
#include "gazebo/transport/Publisher.hh"

#include "gazebo/math/Vector2d.hh"

#include "gazebo/gui/viewers/ViewFactory.hh"
#include "gazebo/gui/viewers/LaserView.hh"

using namespace gazebo;
using namespace gui;

GZ_REGISTER_STATIC_VIEWER("gazebo.msgs.LaserScanStamped", LaserView)

/////////////////////////////////////////////////
LaserView::LaserView(QWidget *_parent)
: TopicView(_parent, "gazebo.msgs.LaserScanStamped", "laser")
{
  this->setWindowTitle(tr("Gazebo: Laser View"));

  this->firstMsg = true;

  // Create the laser display
  // {
  QVBoxLayout *frameLayout = new QVBoxLayout;

  QGraphicsScene *scene = new QGraphicsScene();

  QBrush brush(QColor(250, 250, 250), Qt::SolidPattern);
  scene->setBackgroundBrush(brush);

  this->view = new CustomView(this);
  this->view->setScene(scene);
  this->view->setDragMode(QGraphicsView::ScrollHandDrag);
  this->view->centerOn(QPointF(0, 0));
  this->view->setMinimumHeight(240);

  this->laserItem = new LaserView::LaserItem();
  scene->addItem(this->laserItem);

  QHBoxLayout *controlLayout = new QHBoxLayout;

  QPushButton *fitButton = new QPushButton("Fit in View");
  connect(fitButton, SIGNAL(clicked()), this, SLOT(OnFitInView()));

  QRadioButton *degreeToggle = new QRadioButton();
  degreeToggle->setText("Degrees");
  connect(degreeToggle, SIGNAL(toggled(bool)), this, SLOT(OnDegree(bool)));

  controlLayout->addWidget(degreeToggle);
  controlLayout->addWidget(fitButton);
  controlLayout->addStretch(1);

  frameLayout->addWidget(this->view);
  frameLayout->addLayout(controlLayout);

  this->frame->setObjectName("blackBorderFrame");
  this->frame->setLayout(frameLayout);
  // }
}

/////////////////////////////////////////////////
LaserView::~LaserView()
{
}

/////////////////////////////////////////////////
void LaserView::UpdateImpl()
{
}

/////////////////////////////////////////////////
void LaserView::SetTopic(const std::string &_topicName)
{
  this->firstMsg = true;

  TopicView::SetTopic(_topicName);

  // Subscribe to the new topic.
  this->sub.reset();
  this->sub = this->node->Subscribe(_topicName, &LaserView::OnScan, this);
}

/////////////////////////////////////////////////
void LaserView::resizeEvent(QResizeEvent *_event)
{
  // Automatically fit the laser item to the view if the user has not zoomed
  // in/out
  if (!this->view->viewZoomed && this->laserItem)
  {
    QRectF bound = this->laserItem->GetBoundingRect();
    this->view->fitInView(bound, Qt::KeepAspectRatio);
  }

  QDialog::resizeEvent(_event);
}

/////////////////////////////////////////////////
void LaserView::OnScan(ConstLaserScanStampedPtr &_msg)
{
  // Update the Hz and Bandwidth info
  this->OnMsg(msgs::Convert(_msg->time()), _msg->ByteSize());

  this->laserItem->Clear();

  double angle = _msg->scan().angle_min();

  double r;
  for (unsigned int i = 0;
       i < static_cast<unsigned int>(_msg->scan().ranges_size()); i++)
  {
    r = _msg->scan().ranges(i) + _msg->scan().range_min();

    if (i+1 >= this->laserItem->GetRangeCount())
      this->laserItem->AddRange(r);
    else
      this->laserItem->SetRange(i+1, r);

    angle += _msg->scan().angle_step();
  }

  // Recalculate the points to draw.
  this->laserItem->Update(
      _msg->scan().angle_min(),
      _msg->scan().angle_max(),
      _msg->scan().angle_step(),
      _msg->scan().range_max(),
      _msg->scan().range_min());

  if (this->firstMsg)
  {
    QRectF bound = this->laserItem->GetBoundingRect();
    this->view->fitInView(bound, Qt::KeepAspectRatio);
    this->firstMsg = false;
  }
}

/////////////////////////////////////////////////
void LaserView::OnFitInView()
{
  QRectF bound = this->laserItem->GetBoundingRect();
  this->view->fitInView(bound, Qt::KeepAspectRatio);
  this->view->viewZoomed = false;
}

/////////////////////////////////////////////////
void LaserView::OnDegree(bool _toggled)
{
  this->laserItem->radians = !_toggled;
}

/////////////////////////////////////////////////
LaserView::LaserItem::LaserItem()
{
  this->setFlag(QGraphicsItem::ItemIsSelectable, false);
  this->setAcceptHoverEvents(true);
  this->indexAngle = -999;
  this->scale = 100.0;
  this->rangeMax = 8.0;
  this->rangeMin = 0.0;
  this->radians = true;
}

/////////////////////////////////////////////////
void LaserView::LaserItem::paint(QPainter *_painter,
    const QStyleOptionGraphicsItem * /*_option*/, QWidget * /*_widget*/)
{
  boost::mutex::scoped_lock lock(this->mutex);

  QColor orange(245, 129, 19, 255);
  QColor darkGrey(100, 100, 100, 255);
  QColor lightGrey(100, 100, 100, 50);

  _painter->setPen(QPen(darkGrey));
  _painter->setBrush(lightGrey);

  // Draw the filled polygon that represents the current laser data.
  _painter->drawPolygon(&this->points[0], this->points.size());

  // Draw a box with an arrow around the (0, 0) location
  _painter->setPen(QPen(orange));
  _painter->setBrush(QColor(0, 0, 0, 0));
  _painter->drawRect(-10, -10, 20, 20);
  _painter->drawLine(0, 0, 20, 0);
  _painter->drawLine(25, 0, 20, -5);
  _painter->drawLine(20, -5, 20, 5);
  _painter->drawLine(20, 5, 25, 0);


  // Compute the index of the ray that the mouse is hovering over.
  int index = static_cast<int>(
      rint((this->indexAngle - this->angleMin) / this->angleStep));

  // Draw the ray and associated data if the index is valid.
  if (index >= 0 && index < static_cast<int>(this->ranges.size()))
  {
    double x1, y1;
    double x2, y2;

    double rangeScaled = this->ranges[index] * this->scale;
    double rangeMaxScaled = this->rangeMax * this->scale;

    // Draw the ray
    x1 = rangeScaled * cos(this->indexAngle);
    y1 = -rangeScaled * sin(this->indexAngle);
    _painter->setPen(QPen(orange));
    _painter->drawLine(0, 0, x1, y1);

    // Set the text for the range measurement
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(4)
           << this->ranges[index] << " m";

    // Compute the size of the box we want to use for the range measurement.
    // This will help us scale the text properly.
    float textBoxWidth = (rangeMaxScaled * 1.2) -rangeMaxScaled;

    // Compute the text scaling factor.
    float textFactor = textBoxWidth /
      _painter->fontMetrics().width(stream.str().c_str());

    // Set the font according to the scaling factor.
    QFont f = _painter->font();
    f.setPointSizeF(f.pointSizeF() * textFactor);
    _painter->setFont(f);

    // The final text width and height
    float textWidth = _painter->fontMetrics().width(stream.str().c_str());
    float textHeight = _painter->fontMetrics().height();

    // Compute the additional offset to apply to the range text. This will
    // prevent the text from overlapping the range polygon.
    double rAngle = M_PI - this->indexAngle;
    double xOff = cos(rAngle) < 0 ? 0 : cos(rAngle) * textWidth;
    double yOff = sin(rAngle) * textHeight;

    // The location to draw the range text
    x1 = (rangeScaled * 1.05) * cos(this->indexAngle);
    y1 = -(rangeScaled * 1.05) * sin(this->indexAngle);

    x1 -= xOff;
    y1 -= yOff;

    // Draw the range text
    _painter->setPen(QPen(orange));
    _painter->drawText(x1, y1, stream.str().c_str());

    // This section draws the arc and the angle of the ray
    {
      // Give the arc some padding.
      textWidth *= 1.4;

      // Compute the position for the angle text.
      x1 = (rangeMaxScaled * 1.15 + textWidth) * cos(this->indexAngle * 0.5);
      y1 = -(rangeMaxScaled * 1.15 + textWidth) * sin(this->indexAngle * 0.5);

      // Draw the text for the angle of the ray
      stream.str(std::string());
      if (this->radians)
        stream << std::fixed << std::setprecision(4)
          << this->indexAngle << " radians";
      else
        stream << std::fixed << std::setprecision(4)
          << GZ_RTOD(this->indexAngle) << " degrees";

      _painter->setPen(QPen(orange));
      _painter->drawText(x1, y1, stream.str().c_str());

      // Draw an arc that depicts the angle of the ray
      QRectF rect(-(rangeMaxScaled * 1.1 + textWidth),
                  -(rangeMaxScaled * 1.1 + textWidth),
                  rangeMaxScaled * 1.1 * 2.0 + textWidth * 2.0,
                  rangeMaxScaled * 1.1 * 2.0 + textWidth * 2.0);

      _painter->setPen(QPen(orange));
      _painter->drawArc(rect, 0, GZ_RTOD(this->indexAngle) * 16);


      // Draw the line that marks the start of the arc
      x1 = (rangeMaxScaled * 1.1 + textWidth);
      x2 = (rangeMaxScaled * 1.15 + textWidth);

      _painter->setPen(QPen(orange));
      _painter->drawLine(x1, 0, x2, 0);

      // Draw the line that marks the end of the arc
      x1 = (rangeMaxScaled * 1.1 + textWidth) * cos(this->indexAngle);
      y1 = -(rangeMaxScaled * 1.1 + textWidth) * sin(this->indexAngle);

      x2 = (rangeMaxScaled * 1.15 + textWidth) * cos(this->indexAngle);
      y2 = -(rangeMaxScaled * 1.15 + textWidth) * sin(this->indexAngle);

      _painter->setPen(QPen(orange));
      _painter->drawLine(x1, y1, x2, y2);
    }
  }
}

/////////////////////////////////////////////////
QRectF LaserView::LaserItem::GetBoundingRect() const
{
  if (this->ranges.size() == 0)
    return QRectF(0, 0, 0, 0);

  // Compute the maximum size of bound box by scaling up the maximum
  // possible range. The multiplication of 1.8 increases the bounding box
  // size to make the mouse hover behavior easier to use.
  double max = this->rangeMax * this->scale * 1.8;

  // Return the top-left position and the width and height.
  return QRectF(-max, -max, max * 2.0, max * 2.0);
}

/////////////////////////////////////////////////
QRectF LaserView::LaserItem::boundingRect() const
{
  return this->GetBoundingRect();
}

/////////////////////////////////////////////////
void LaserView::LaserItem::Clear()
{
  boost::mutex::scoped_lock lock(this->mutex);
  this->ranges.clear();
  this->points.clear();
}

/////////////////////////////////////////////////
void LaserView::LaserItem::AddRange(double _range)
{
  boost::mutex::scoped_lock lock(this->mutex);
  this->ranges.push_back(_range);
}

/////////////////////////////////////////////////
void LaserView::LaserItem::SetRange(unsigned int _index, double _range)
{
  boost::mutex::scoped_lock lock(this->mutex);
  if (_index < this->ranges.size())
    this->ranges[_index] = _range;
}

/////////////////////////////////////////////////
void LaserView::LaserItem::Update(double _angleMin, double _angleMax,
    double _angleStep, double _rangeMax, double _rangeMin)
{
  boost::mutex::scoped_lock lock(this->mutex);

  this->angleMin = _angleMin;
  this->angleMax = _angleMax;
  this->angleStep = _angleStep;
  this->rangeMax = _rangeMax;
  this->rangeMin = _rangeMin;

  // Resize the point array if the number of ranges has changed.
  if (this->rangeMin > 0.0 &&
      this->ranges.size() * 2 != this->points.size())
  {
    // A min range > 0 means we have to draw an inner circle, so we twice as
    // many points
    this->points.resize(this->ranges.size() * 2);
  }
  else if (math::equal(this->rangeMin, 0.0) &&
      this->ranges.size() + 1 != this->points.size())
  {
    // A min range == 0 mean we just need a closing point at the (0, 0)
    // location
    this->points.resize(this->ranges.size() + 1);
  }

  double angle = this->angleMin;

  // Add a point for each laser reading
  for (unsigned int i = 0; i < this->ranges.size(); ++i)
  {
    QPointF pt(this->ranges[i] * this->scale * cos(angle),
        -this->ranges[i] * this->scale * sin(angle));
    this->points[i] = pt;
    angle += this->angleStep;
  }

  // If a min range is set, then draw an inner circle
  if (this->rangeMin > 0.0)
  {
    // Create the inner circle that denotes the min range
    for (int i = this->ranges.size()-1; i >=0 ; --i)
    {
      QPointF pt(this->rangeMin * this->scale * cos(angle),
          -this->rangeMin * this->scale * sin(angle));
      this->points[i + this->ranges.size()] = pt;
      angle -= this->angleStep;
    }
  }
  else
  {
    // Connect the last point to the (0,0)
    this->points[this->ranges.size()] = QPointF(0, 0);
  }

  // Tell QT we have changed.
  this->prepareGeometryChange();
}

/////////////////////////////////////////////////
unsigned int LaserView::LaserItem::GetRangeCount()
{
  boost::mutex::scoped_lock lock(this->mutex);
  return this->ranges.size();
}

/////////////////////////////////////////////////
void LaserView::LaserItem::hoverEnterEvent(QGraphicsSceneHoverEvent *_event)
{
  this->indexAngle = atan2(-_event->pos().y(), _event->pos().x());

  QApplication::setOverrideCursor(Qt::CrossCursor);
}

//////////////////////////////////////////////////
void LaserView::LaserItem::hoverLeaveEvent(
    QGraphicsSceneHoverEvent * /*_event*/)
{
  this->indexAngle = -999;
  QApplication::setOverrideCursor(Qt::OpenHandCursor);
}

/////////////////////////////////////////////////
void LaserView::LaserItem::hoverMoveEvent(QGraphicsSceneHoverEvent *_event)
{
  this->indexAngle = atan2(-_event->pos().y(), _event->pos().x());
}
