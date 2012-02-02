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
#include <string.h>
#include "math/gzmath.h"
#include "msgs/msgs.h"
#include "ServerFixture.hh"

using namespace gazebo;
class MathTest : public ServerFixture
{
};

TEST_F(MathTest, Box)
{
  {
    math::Box box;
    EXPECT_TRUE(box.min == math::Vector3(0, 0, 0));
    EXPECT_TRUE(box.max == math::Vector3(0, 0, 0));
  }

  {
    math::Box box(math::Vector3(0, 1, 2), math::Vector3(1, 2, 3));
    EXPECT_TRUE(box.min == math::Vector3(0, 1, 2));
    EXPECT_TRUE(box.max == math::Vector3(1, 2, 3));
  }

  {
    math::Box box(math::Vector3(0, 1, 2), math::Vector3(1, 2, 3));
    math::Box box1(box);
    EXPECT_TRUE(box1.min == box.min);
    EXPECT_TRUE(box1.max == box.max);
  }

  math::Box box(math::Vector3(0, 1, 2), math::Vector3(1, 2, 3));
  EXPECT_EQ(box.GetXLength(), 1);
  EXPECT_EQ(box.GetYLength(), 1);
  EXPECT_EQ(box.GetZLength(), 1);
  EXPECT_TRUE(box.GetSize() == math::Vector3(1, 1, 1));
  EXPECT_TRUE(box.GetCenter() == math::Vector3(0.5, 1.5, 2.5));

  box.Merge(math::Box(math::Vector3(-1, -1, -1), math::Vector3(2, 2, 2)));
  EXPECT_TRUE(box == math::Box(math::Vector3(-1, -1, -1),
                               math::Vector3(2, 2, 3)));

  box = math::Box(math::Vector3(1, 1, 1), math::Vector3(3, 3, 3));
  EXPECT_TRUE(box == math::Box(math::Vector3(1, 1, 1), math::Vector3(3, 3, 3)));

  box += math::Box(math::Vector3(2, 2, 2), math::Vector3(4, 4, 4));
  EXPECT_TRUE(box == math::Box(math::Vector3(1, 1, 1), math::Vector3(4, 4, 4)));

  box = box + math::Box(math::Vector3(-2, -2, -2), math::Vector3(4, 4, 4));
  EXPECT_TRUE(box == math::Box(math::Vector3(-2, -2, -2),
                               math::Vector3(4, 4, 4)));
}

TEST_F(MathTest, Vector4)
{
  {
    math::Vector4 v;
    EXPECT_TRUE(math::equal(v.x, 0));
    EXPECT_TRUE(math::equal(v.y, 0));
    EXPECT_TRUE(math::equal(v.z, 0));
    EXPECT_TRUE(math::equal(v.w, 0));
  }

  math::Vector4 v1(1, 2, 3, 4);
  EXPECT_TRUE(math::equal(v1.x, 1));
  EXPECT_TRUE(math::equal(v1.y, 2));
  EXPECT_TRUE(math::equal(v1.z, 3));
  EXPECT_TRUE(math::equal(v1.w, 4));

  math::Vector4 v(v1);
  EXPECT_TRUE(v == v1);

  EXPECT_TRUE(math::equal(v.Distance(math::Vector4(0, 0, 0, 0)), 5.4772, 1e-3));
}

/////////////////////////////////////////////////
TEST_F(MathTest, Quaternion)
{
  {
    math::Quaternion q;
    EXPECT_TRUE(math::equal(q.w, 1));
    EXPECT_TRUE(math::equal(q.x, 0));
    EXPECT_TRUE(math::equal(q.y, 0));
    EXPECT_TRUE(math::equal(q.z, 0));
  }

  {
    math::Quaternion q(1, 2, 3, 4);
    EXPECT_TRUE(math::equal(q.w, 1));
    EXPECT_TRUE(math::equal(q.x, 2));
    EXPECT_TRUE(math::equal(q.y, 3));
    EXPECT_TRUE(math::equal(q.z, 4));
  }

  {
    math::Quaternion q(0, 1, 2);
    EXPECT_TRUE(q == math::Quaternion(math::Vector3(0, 1, 2)));
  }

  math::Quaternion q1(math::Vector3(0, 0, 1), M_PI);
  EXPECT_TRUE(math::equal(q1.x, 0));
  EXPECT_TRUE(math::equal(q1.y, 0));
  EXPECT_TRUE(math::equal(q1.z, 1));
  EXPECT_TRUE(math::equal(q1.w, 0));

  math::Quaternion q(q1);
  EXPECT_TRUE(q == q1);

  q.SetToIdentity();
  EXPECT_TRUE(math::equal(q.w, 1));
  EXPECT_TRUE(math::equal(q.x, 0));
  EXPECT_TRUE(math::equal(q.y, 0));
  EXPECT_TRUE(math::equal(q.z, 0));

  q = math::Quaternion(M_PI*0.1, M_PI*0.5, M_PI);
  EXPECT_TRUE(q == math::Quaternion(0.110616, -0.698401, 0.110616, 0.698401));

  EXPECT_TRUE(q.GetLog() ==
      math::Quaternion(0, -1.02593, 0.162491, 1.02593));

  EXPECT_TRUE(q.GetExp() ==
      math::Quaternion(0.545456, -0.588972, 0.093284, 0.588972));

  q1 = q;
  q1.w = 2.0;
  EXPECT_TRUE(q1.GetLog() ==
      math::Quaternion(0, -0.698401, 0.110616, 0.698401));

  q1.x = 0.000000001;
  q1.y = 0.0;
  q1.z = 0.0;
  q1.w = 0.0;
  EXPECT_TRUE(q1.GetExp() == math::Quaternion(1, 0, 0, 0));

  q.Invert();
  EXPECT_TRUE(q == math::Quaternion(0.110616, 0.698401, -0.110616, -0.698401));

  q.SetFromAxis(0, 1, 0, M_PI);
  EXPECT_TRUE(q == math::Quaternion(6.12303e-17, 0, 1, 0));

  q.SetFromAxis(math::Vector3(1, 0, 0), M_PI);
  EXPECT_TRUE(q == math::Quaternion(0, 1, 0, 0));

  q.Set(1, 2, 3, 4);
  EXPECT_TRUE(math::equal(q.w, 1));
  EXPECT_TRUE(math::equal(q.x, 2));
  EXPECT_TRUE(math::equal(q.y, 3));
  EXPECT_TRUE(math::equal(q.z, 4));

  q.Normalize();
  EXPECT_TRUE(q == math::Quaternion(0.182574, 0.365148, 0.547723, 0.730297));


  EXPECT_TRUE(math::equal(q.GetRoll(), 1.4289, 1e-3));
  EXPECT_TRUE(math::equal(q.GetPitch(), -0.339837, 1e-3));
  EXPECT_TRUE(math::equal(q.GetYaw(), 2.35619, 1e-3));

  math::Vector3 axis;
  double angle;
  q.GetAsAxis(axis, angle);
  EXPECT_TRUE(axis == math::Vector3(0.371391, 0.557086, 0.742781));
  EXPECT_TRUE(math::equal(angle, 2.77438, 1e-3));

  q.Scale(0.1);
  EXPECT_TRUE(q == math::Quaternion(0.990394, 0.051354, 0.0770309, 0.102708));

  q = q + math::Quaternion(0, 1, 2);
  EXPECT_TRUE(q == math::Quaternion(1.46455, -0.352069, 0.336066, 0.841168));

  q += q;
  EXPECT_TRUE(q == math::Quaternion(2.92911, -0.704137, 0.672131, 1.68234));

  q -= math::Quaternion(.4, .2, .1);
  EXPECT_TRUE(q == math::Quaternion(1.95416, -0.896677, 0.56453, 1.65341));

  q = q - math::Quaternion(0, 1, 2);
  EXPECT_TRUE(q == math::Quaternion(1.48, -0.493254, 0.305496, 0.914947));

  q *= math::Quaternion(.4, .1, .01);
  EXPECT_TRUE(q == math::Quaternion(1.53584, -0.236801, 0.551841, 0.802979));

  q = q * 5.0;
  EXPECT_TRUE(q == math::Quaternion(7.67918, -1.184, 2.7592, 4.0149));

  EXPECT_TRUE(q.RotateVectorReverse(math::Vector3(1, 2, 3)) ==
      math::Vector3(-0.104115, 0.4975, 3.70697));

  EXPECT_TRUE(q.GetXAxis() == math::Vector3(-46.4652, 55.1284, -51.8841));
  EXPECT_TRUE(q.GetYAxis() == math::Vector3(-68.196, -34.0425, 3.97149));
  EXPECT_TRUE(q.GetZAxis() == math::Vector3(32.8695, 40.3402, -17.0301));

  EXPECT_TRUE(-q == math::Quaternion(-7.67918, 1.184, -2.7592, -4.0149));
  EXPECT_TRUE(q.GetAsMatrix3() == math::Matrix3(
       0.435639, -0.810851, 0.390819,
       0.655477, 0.583344, 0.479645,
       -0.616903, 0.047221, 0.785622));
  EXPECT_TRUE(q.GetAsMatrix4() == math::Matrix4(
       0.435639, -0.810851, 0.390819, 0,
       0.655477, 0.583344, 0.479645, 0,
       -0.616903, 0.047221, 0.785622, 0,
       0, 0, 0, 1));

  EXPECT_TRUE(math::equal(q.Dot(math::Quaternion(.4, .2, .1)), 7.67183, 1e-3));

  EXPECT_TRUE(math::Quaternion::Squad(1.1, math::Quaternion(.1, 0, .2),
        math::Quaternion(0, .3, .4), math::Quaternion(.5, .2, 1),
        math::Quaternion(0, 0, 2), true) ==
      math::Quaternion(0.346807, -0.0511734, -0.0494723, 0.935232));

  EXPECT_TRUE(math::Quaternion::EulerToQuaternion(math::Vector3(.1, .2, .3)) ==
      math::Quaternion(0.983347, 0.0342708, 0.106021, 0.143572));

  q.Round(2);
  EXPECT_TRUE(math::equal(-1.18, q.x));
  EXPECT_TRUE(math::equal(2.76, q.y));
  EXPECT_TRUE(math::equal(4.01, q.z));
  EXPECT_TRUE(math::equal(7.68, q.w));

  q.x = q.y = q.z = q.w = 0.0;
  q.Normalize();
  EXPECT_TRUE(q == math::Quaternion());

  q.SetFromAxis(0, 0, 0, 0);
  EXPECT_TRUE(q == math::Quaternion());
 
  EXPECT_TRUE(math::Quaternion::EulerToQuaternion(0.1,0.2,0.3) ==
      math::Quaternion(0.983347, 0.0342708, 0.106021, 0.143572));

  q.x = q.y = q.z = q.w = 0.0;
  q.GetAsAxis(axis, angle);
  EXPECT_TRUE(axis == math::Vector3(1, 0, 0));
  EXPECT_TRUE(math::equal(angle, 0, 1e-3));


  //std::cout << "Exp[" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << "]\n";
}

TEST_F(MathTest, Pose)
{
  {
    math::Pose pose;
    EXPECT_TRUE(pose.pos == math::Vector3(0, 0, 0));
    EXPECT_TRUE(pose.rot == math::Quaternion(0, 0, 0));
  }

  math::Pose pose(math::Vector3(1, 2, 3), math::Quaternion(.1, .2, .3));
  EXPECT_TRUE(pose.pos == math::Vector3(1, 2, 3));
  EXPECT_TRUE(pose.rot == math::Quaternion(.1, .2, .3));

  math::Pose pose1(pose);
  EXPECT_TRUE(pose1 == pose);

  pose.Set(math::Vector3(2, 3, 4), math::Quaternion(.3, .4, .5));
  EXPECT_TRUE(pose.pos == math::Vector3(2, 3, 4));
  EXPECT_TRUE(pose.rot == math::Quaternion(.3, .4, .5));
  EXPECT_TRUE(pose.IsFinite());

  pose1 = pose.GetInverse();
  EXPECT_TRUE(pose1.pos == math::Vector3(-1.38368, -3.05541, -4.21306));
  EXPECT_TRUE(pose1.rot ==
      math::Quaternion(0.946281, -0.0933066, -0.226566, -0.210984));

  pose = math::Pose(1, 2, 3, .1, .2, .3) + math::Pose(4, 5, 6, .4, .5, .6);
  EXPECT_TRUE(pose ==
      math::Pose(5.74534, 7.01053, 8.62899, 0.675732, 0.535753, 1.01174));

  pose += pose;
  EXPECT_TRUE(pose ==
      math::Pose(11.314, 16.0487, 15.2559, 1.49463, 0.184295, 2.13932));

  pose -= pose;
  EXPECT_TRUE(pose ==
      math::Pose(0, 0, 0, 0, 0, 0));

  pose.pos.Set(5, 6, 7);
  pose.rot.SetFromEuler(math::Vector3(.4, .6, 0));

  EXPECT_TRUE(pose.CoordPositionAdd(math::Vector3(1, 2, 3)) ==
      math::Vector3(7.82531, 6.67387, 9.35871));

  EXPECT_TRUE(pose.CoordPositionAdd(pose1) ==
      math::Vector3(2.58141, 2.4262, 3.8013));
  EXPECT_TRUE(pose.CoordRotationAdd(math::Quaternion(0.1, 0, 0.2)) ==
      math::Quaternion(0.520975, 0.596586, 0.268194));
  EXPECT_TRUE(pose.CoordPoseSolve(pose1) ==
      math::Pose(-0.130957, -11.552, -10.2329,
                 -0.462955, -1.15624, -0.00158047));

  EXPECT_TRUE(pose.RotatePositionAboutOrigin(math::Quaternion(0.1, 0, 0.2)) ==
      math::Pose(6.09235, 5.56147, 6.47714, 0.4, 0.6, 0));

  pose.Reset();
  EXPECT_TRUE(pose.pos == math::Vector3(0, 0, 0));
  EXPECT_TRUE(pose.rot == math::Quaternion(0, 0, 0));
}

TEST_F(MathTest, Plane)
{
  {
    math::Plane plane;
    EXPECT_TRUE(math::equal(plane.d, 0.0));
    EXPECT_TRUE(plane.normal == math::Vector3());
    EXPECT_TRUE(plane.size == math::Vector2d(0, 0));
  }

  {
    math::Plane plane(math::Vector3(0, 0, 1), math::Vector2d(2, 3), 2.0);
    EXPECT_TRUE(math::equal(plane.d, 2.0));
    EXPECT_TRUE(plane.normal == math::Vector3(0, 0, 1));
    EXPECT_TRUE(plane.size == math::Vector2d(2, 3));

    plane.Set(math::Vector3(1, 0, 0), math::Vector2d(1, 1), 1.0);
    EXPECT_TRUE(math::equal(plane.d, 1.0));
    EXPECT_TRUE(plane.normal == math::Vector3(1, 0, 0));
    EXPECT_TRUE(plane.size == math::Vector2d(1, 1));

    plane = math::Plane(math::Vector3(0, 1, 0), math::Vector2d(4, 4), 5.0);
    EXPECT_TRUE(math::equal(plane.d, 5.0));
    EXPECT_TRUE(plane.normal == math::Vector3(0, 1, 0));
    EXPECT_TRUE(plane.size == math::Vector2d(4, 4));
  }
}

TEST_F(MathTest, Matrix3)
{
  {
    math::Matrix3 matrix;
    EXPECT_TRUE(matrix == math::Matrix3(0, 0, 0, 0, 0, 0, 0, 0, 0));
  }

  {
    math::Matrix3 matrix(1, 2, 3, 4, 5, 6, 7, 8, 9);
    EXPECT_TRUE(matrix == math::Matrix3(1, 2, 3, 4, 5, 6, 7, 8, 9));

    math::Matrix3 matrix1(matrix);
    EXPECT_TRUE(matrix1 == math::Matrix3(1, 2, 3, 4, 5, 6, 7, 8, 9));
  }

  math::Matrix3 matrix;
  matrix.SetFromAxes(math::Vector3(1, 1, 1), math::Vector3(2, 2, 2),
                     math::Vector3(3, 3, 3));
  EXPECT_TRUE(matrix == math::Matrix3(1, 2, 3, 1, 2, 3, 1, 2, 3));

  matrix.SetFromAxis(math::Vector3(1, 1, 1), M_PI);
  EXPECT_TRUE(matrix == math::Matrix3(1, 2, 2, 2, 1, 2, 2, 2, 1));

  matrix.SetCol(0, math::Vector3(3, 4, 5));
  EXPECT_TRUE(matrix == math::Matrix3(3, 2, 2, 4, 1, 2, 5, 2, 1));

  EXPECT_THROW(matrix.SetCol(3, math::Vector3(1, 1, 1)), common::Exception);
}

TEST_F(MathTest, Angle)
{
  math::Angle angle1;
  EXPECT_TRUE(math::equal(0.0, angle1.GetAsRadian()));

  angle1.SetFromDegree(180.0);
  EXPECT_TRUE(angle1 == M_PI);

  angle1 = math::Angle(0.1) - math::Angle(0.3);
  EXPECT_TRUE(angle1 == -0.2);

  math::Angle angle(0.5);
  EXPECT_TRUE(math::equal(0.5, angle.GetAsRadian()));

  angle.SetFromRadian(M_PI);
  EXPECT_TRUE(math::equal(GZ_RTOD(M_PI), angle.GetAsDegree()));

  angle.Normalize();
  EXPECT_TRUE(math::equal(GZ_RTOD(M_PI), angle.GetAsDegree()));

  angle = math::Angle(0.1) + math::Angle(0.2);
  EXPECT_TRUE(math::equal(0.3, angle.GetAsRadian()));

  angle = math::Angle(0.1) * math::Angle(0.2);
  EXPECT_TRUE(math::equal(0.02, angle.GetAsRadian()));

  angle = math::Angle(0.1) / math::Angle(0.2);
  EXPECT_TRUE(math::equal(0.5, angle.GetAsRadian()));

  angle -= math::Angle(0.1);
  EXPECT_TRUE(math::equal(0.4, angle.GetAsRadian()));

  angle += math::Angle(0.2);
  EXPECT_TRUE(math::equal(0.6, angle.GetAsRadian()));

  angle *= math::Angle(0.5);
  EXPECT_TRUE(math::equal(0.3, angle.GetAsRadian()));

  angle /= math::Angle(0.1);
  EXPECT_TRUE(math::equal(3, angle.GetAsRadian()));
  EXPECT_TRUE(angle == math::Angle(3));
  EXPECT_TRUE(angle != math::Angle(2));
  EXPECT_TRUE(angle < math::Angle(4));
  EXPECT_TRUE(angle <= math::Angle(3));
  EXPECT_TRUE(angle > math::Angle(2));
  EXPECT_TRUE(angle >= math::Angle(3));
}

TEST_F(MathTest, Random)
{
  double d;
  int i;
  // TODO: implement a proper random number generator test

  d = gazebo::math::Rand::GetDblUniform(1, 2);
  EXPECT_LE(d, 2);
  EXPECT_GE(d, 1);

  d = math::Rand::GetDblNormal(2, 3);

  i = math::Rand::GetIntUniform(1, 2);
  EXPECT_LE(i, 2);
  EXPECT_GE(i, 1);

  i = math::Rand::GetIntNormal(2, 3);
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
