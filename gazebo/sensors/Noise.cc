/*
 * Copyright 2013 Open Source Robotics Foundation
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


#include <boost/math/special_functions/round.hpp>

#include "gazebo/common/Assert.hh"
#include "gazebo/common/Console.hh"
#include "gazebo/math/Helpers.hh"
#include "gazebo/math/Rand.hh"

#include "gazebo/sensors/Noise.hh"

using namespace gazebo;
using namespace sensors;

//////////////////////////////////////////////////
Noise::Noise()
 : noiseType(NONE),
   mean(0.0),
   stdDev(0.0),
   bias(0.0),
   precision(0.0)
{
}

//////////////////////////////////////////////////
Noise::~Noise()
{
}

//////////////////////////////////////////////////
void Noise::Load(sdf::ElementPtr _sdf)
{
  this->sdf = _sdf;
  GZ_ASSERT(this->sdf != NULL, "this->sdf is NULL");
  std::string type = this->sdf->Get<std::string>("type");
  if (type == "none")
    this->noiseType = NONE;
  else if (type == "gaussian")
    this->noiseType = GAUSSIAN;
  else if (type == "gaussian_quantized")
    this->noiseType = GAUSSIAN_QUANTIZED;
  else
  {
    gzerr << "Unrecognized noise type: [" << type << "]"
          << ", using default [none]" << std::endl;
    this->noiseType = NONE;
  }

  if (this->noiseType == GAUSSIAN ||
      this->noiseType == GAUSSIAN_QUANTIZED)
  {
    this->mean = this->sdf->Get<double>("mean");
    this->stdDev = this->sdf->Get<double>("stddev");
    // Sample the bias
    double biasMean = this->sdf->Get<double>("bias_mean");
    double biasStdDev = this->sdf->Get<double>("bias_stddev");
    this->bias = math::Rand::GetDblNormal(biasMean, biasStdDev);
    // With equal probability, we pick a negative bias (by convention,
    // rateBiasMean should be positive, though it would work fine if
    // negative).
    if (math::Rand::GetDblUniform() < 0.5)
      this->bias = -this->bias;
    gzlog << "applying Gaussian noise model with mean " << this->mean
      << ", stddev " << this->stdDev
      << ", bias " << this->bias << std::endl;
  }

  if (this->noiseType == GAUSSIAN_QUANTIZED)
    this->precision = this->sdf->Get<double>("precision");
}

//////////////////////////////////////////////////
double Noise::Apply(double _in) const
{
  double output = 0.0;
  if (this->noiseType == NONE)
    output = _in;
  else if (this->noiseType == GAUSSIAN ||
           this->noiseType == GAUSSIAN_QUANTIZED)
  {
    double whiteNoise = math::Rand::GetDblNormal(this->mean, this->stdDev);
    output = _in + this->bias + whiteNoise;
    if (this->noiseType == GAUSSIAN_QUANTIZED)
    {
      // Apply this->precision
      if (!math::equal(this->precision, 0.0, 1e-6))
      {
        output = boost::math::round(output / this->precision) * this->precision;
      }
    }
  }
  return output;
}
