/*
 * Copyright (C) 2012-2016 Open Source Robotics Foundation
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

#ifdef _WIN32
  // Ensure that Winsock2.h is included before Windows.h, which can get
  // pulled in by anybody (e.g., Boost).
  #include <Winsock2.h>
#endif

#include <map>

#include <ignition/math/Helpers.hh>

#include "gazebo/common/Assert.hh"
#include "gazebo/common/Console.hh"

#include "gazebo/math/Helpers.hh"
#include "gazebo/gui/plot/PlotCurve.hh"
#include "gazebo/gui/plot/IncrementalPlot.hh"

using namespace gazebo;
using namespace gui;


namespace gazebo
{
  namespace gui
  {
    /// \internal
    /// \brief IncrementalPlot private data
    class IncrementalPlotPrivate
    {
      /// \brief A map of unique ids to plot curves.
      public: typedef std::map<unsigned int, PlotCurvePtr > CurveMap;

      /// \brief The curve to draw.
      public: CurveMap curves;

      /// \brief Drawing utility
      public: QwtPlotDirectPainter *directPainter;

      /// \brief Pointer to the plot maginfier
      public: QwtPlotMagnifier *magnifier;

      /// \brief Period duration in seconds.
      public: unsigned int period;
    };
  }
}

/////////////////////////////////////////////////
IncrementalPlot::IncrementalPlot(QWidget *_parent)
  : QwtPlot(_parent),
    dataPtr(new IncrementalPlotPrivate)
{
  this->setObjectName("incrementalPlot");

  this->dataPtr->period = 10;
  this->dataPtr->directPainter = new QwtPlotDirectPainter(this);

  // panning with the left mouse button
  (void) new QwtPlotPanner(this->canvas());

  // zoom in/out with the wheel
  this->dataPtr->magnifier = new QwtPlotMagnifier(this->canvas());

#if defined(Q_WS_X11)
  this->canvas()->setAttribute(Qt::WA_PaintOutsidePaintEvent, true);
  this->canvas()->setAttribute(Qt::WA_PaintOnScreen, true);
#endif

  this->setAutoReplot(false);

  this->setFrameStyle(QFrame::NoFrame);
  this->setLineWidth(0);

  this->plotLayout()->setAlignCanvasToScales(true);

  QwtLegend *qLegend = new QwtLegend;
  this->insertLegend(qLegend, QwtPlot::RightLegend);

  QwtPlotGrid *grid = new QwtPlotGrid;

#if (QWT_VERSION < ((6 << 16) | (1 << 8) | 0))
  grid->setMajPen(QPen(Qt::gray, 0, Qt::DotLine));
#else
  grid->setMajorPen(QPen(Qt::gray, 0, Qt::DotLine));
#endif
  grid->attach(this);

  /// TODO Figure out a way to properly label the x and y axis
  QwtText xtitle("Sim Time");
  xtitle.setFont(QFont(fontInfo().family(), 10, QFont::Bold));
  this->setAxisTitle(QwtPlot::xBottom, xtitle);
  QwtText ytitle("Variable values");
  ytitle.setFont(QFont(fontInfo().family(), 10, QFont::Bold));
  this->setAxisTitle(QwtPlot::yLeft, ytitle);

  this->enableAxis(QwtPlot::yLeft);
  this->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
  this->setAxisAutoScale(QwtPlot::yLeft, true);

  this->replot();
}

/////////////////////////////////////////////////
IncrementalPlot::~IncrementalPlot()
{
  this->dataPtr->curves.clear();
}

/////////////////////////////////////////////////
PlotCurveWeakPtr IncrementalPlot::Curve(const std::string &_label) const
{
  for (const auto &it : this->dataPtr->curves)
  {
    if (it.second->Label() == _label)
      return it.second;
  }

  return PlotCurveWeakPtr();
}

/////////////////////////////////////////////////
PlotCurveWeakPtr IncrementalPlot::Curve(const unsigned int _id) const
{
  auto it = this->dataPtr->curves.find(_id);
  if (it != this->dataPtr->curves.end())
    return it->second;
  else
  {
    return PlotCurveWeakPtr();
  }
}

/////////////////////////////////////////////////
void IncrementalPlot::AddPoints(const unsigned int _id,
    const std::vector<ignition::math::Vector2d> &_pts)
{
  PlotCurveWeakPtr plotCurve = this->Curve(_id);

  auto c = plotCurve.lock();
  if (!c)
  {
    gzerr << "Unable to add points. "
        << "Curve with id' " << _id << "' is not found" << std::endl;
    return;
  }

  c->AddPoints(_pts);
}

/////////////////////////////////////////////////
void IncrementalPlot::AddPoint(const unsigned int _id,
    const ignition::math::Vector2d &_pt)
{
  PlotCurveWeakPtr plotCurve = this->Curve(_id);

  auto c = plotCurve.lock();
  if (!c)
  {
    gzerr << "Unable to add point. "
        << "Curve with id' " << _id << "' is not found" << std::endl;
    return;
  }

  c->AddPoint(_pt);
}

/////////////////////////////////////////////////
PlotCurveWeakPtr IncrementalPlot::AddCurve(const std::string &_label)
{
  PlotCurveWeakPtr plotCurve = this->Curve(_label);
  if (!plotCurve.expired())
  {
    gzerr << "Curve '" << _label << "' already exists" << std::endl;
    return plotCurve;
  }

  PlotCurvePtr newPlotCurve(new PlotCurve(_label));
  newPlotCurve->Attach(this);
  this->dataPtr->curves[newPlotCurve->Id()] = newPlotCurve;

  return newPlotCurve;
}

/////////////////////////////////////////////////
void IncrementalPlot::Clear()
{
  for (auto &c : this->dataPtr->curves)
    c.second->Clear();

  this->dataPtr->curves.clear();

  this->replot();
}

/////////////////////////////////////////////////
void IncrementalPlot::Update()
{
  if (this->dataPtr->curves.empty())
    return;

  double yMin = IGN_DBL_MAX;
  double yMax = 0;

  ignition::math::Vector2d lastPoint;
  for (auto &curve : this->dataPtr->curves)
  {
    if (!curve.second->Active())
      continue;

    unsigned int pointCount = curve.second->Size();
    if (pointCount == 0u)
      continue;

    lastPoint = curve.second->Point(pointCount-1);

    ignition::math::Vector2d minPt = curve.second->Min();
    ignition::math::Vector2d maxPt = curve.second->Max();
    if (maxPt.Y() > yMax)
      yMax = maxPt.Y();
    if (minPt.Y() < yMin)
      yMin = minPt.Y();

    this->dataPtr->directPainter->drawSeries(curve.second->Curve(),
      pointCount - 1, pointCount - 1);
  }

  this->setAxisScale(QwtPlot::xBottom,
      std::max(0.0, static_cast<double>(lastPoint.X() - this->dataPtr->period)),
      std::max(1.0, static_cast<double>(lastPoint.X())));

  this->replot();
}

/////////////////////////////////////////////////
void IncrementalPlot::SetPeriod(const unsigned int _seconds)
{
  this->dataPtr->period = _seconds;
}

/////////////////////////////////////////////////
void IncrementalPlot::AttachCurve(PlotCurveWeakPtr _plotCurve)
{
  auto c = _plotCurve.lock();
  if (!c)
    return;

  c->Attach(this);
  this->dataPtr->curves[c->Id()] = c;
}

/////////////////////////////////////////////////
PlotCurvePtr IncrementalPlot::DetachCurve(const unsigned int _id)
{
  PlotCurveWeakPtr plotCurve =  this->Curve(_id);

  auto c = plotCurve.lock();
  if (!c)
    return c;

  c->Detach();
  this->dataPtr->curves.erase(_id);
  return c;
}

/////////////////////////////////////////////////
void IncrementalPlot::RemoveCurve(const unsigned int _id)
{
  auto it = this->dataPtr->curves.find(_id);

  if (it == this->dataPtr->curves.end())
    return;

  this->dataPtr->curves.erase(it);
}

/////////////////////////////////////////////////
void IncrementalPlot::SetCurveLabel(const unsigned int _id,
    const std::string &_label)
{
  if (_label.empty())
    return;

  PlotCurveWeakPtr plotCurve = this->Curve(_id);

  auto c = plotCurve.lock();
  if (!c)
    return;

  c->SetLabel(_label);
}

/////////////////////////////////////////////////
std::vector<PlotCurveWeakPtr> IncrementalPlot::Curves() const
{
  std::vector<PlotCurveWeakPtr> curves;
  for (const auto &it : this->dataPtr->curves)
    curves.push_back(it.second);

  return curves;
}

/////////////////////////////////////////////////
QSize IncrementalPlot::sizeHint() const
{
  // TODO find better way to specify plot size
  return QSize(500, 380);
}
