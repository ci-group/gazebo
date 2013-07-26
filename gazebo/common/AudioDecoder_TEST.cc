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

#include <gazebo/gazebo_config.h>

#include <boost/filesystem.hpp>
#include <gtest/gtest.h>

#include "test_config.h"
#include "gazebo/common/Common.hh"
#include "gazebo/common/AudioDecoder.hh"

#ifdef HAVE_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
//#include <libavcodec/avcodec.h>
}
#endif
using namespace gazebo;

/////////////////////////////////////////////////
TEST(AudioDecoder, FileNotSet)
{
  common::AudioDecoder audio;
  unsigned int dataBufferSize;
  uint8_t *dataBuffer = NULL;
  EXPECT_FALSE(audio.Decode(&dataBuffer, &dataBufferSize));
}

/////////////////////////////////////////////////
TEST(AudioDecoder, MissingFile)
{
  common::AudioDecoder audio;
  unsigned int dataBufferSize;
  uint8_t *dataBuffer = NULL;
  EXPECT_FALSE(audio.Decode(&dataBuffer, &dataBufferSize));
}

/////////////////////////////////////////////////
TEST(AudioDecoder, BufferSizeInvalid)
{
  common::AudioDecoder audio;
  boost::filesystem::path path;

  common::load();

  path = PROJECT_SOURCE_PATH;
  path /= "media/audio/cheer.wav";
  EXPECT_TRUE(audio.SetFile(path.string()));

  unsigned int *dataBufferSize = NULL;
  uint8_t *dataBuffer = NULL;
  EXPECT_FALSE(audio.Decode(&dataBuffer, dataBufferSize));
}

/////////////////////////////////////////////////
TEST(AudioDecoder, DataBuffer)
{
  boost::filesystem::path path;
  common::AudioDecoder audio;

  common::load();

  path = PROJECT_SOURCE_PATH;
  path /= "media/audio/cheer.wav";
  EXPECT_TRUE(audio.SetFile(path.string()));

  unsigned int dataBufferSize;
  uint8_t *dataBuffer = NULL;
  EXPECT_TRUE(audio.Decode(&dataBuffer, &dataBufferSize));

  unsigned int dataBufferSize2;
  uint8_t *dataBuffer2 = new uint8_t[5];
  EXPECT_TRUE(audio.Decode(&dataBuffer2, &dataBufferSize2));

  EXPECT_EQ(dataBufferSize2, dataBufferSize);
  EXPECT_EQ(sizeof(dataBuffer), sizeof(dataBuffer2));
}

/////////////////////////////////////////////////
TEST(AudioDecoder, NoCodec)
{
  common::load();
  common::AudioDecoder audio;
  boost::filesystem::path path;

  path = TEST_PATH;
  path /= "/data/audio_bad_codec.mp4";
  EXPECT_FALSE(audio.SetFile(path.string()));
}

/////////////////////////////////////////////////
TEST(AudioDecoder, CheerFile)
{
  common::load();
  common::AudioDecoder audio;
  boost::filesystem::path path;

  // Test a bad filename
  EXPECT_FALSE(audio.SetFile("_bad_audio_filename_.wav"));

  // Test no stream info
  path = TEST_PATH;
  path /= "data/audio_bad_codec.grf";
  EXPECT_FALSE(audio.SetFile(path.string()));

  // Test a valid file without an audio stream
  path = TEST_PATH;
  path /= "data/empty_audio.mp4";
  EXPECT_FALSE(audio.SetFile(path.string()));

  path = PROJECT_SOURCE_PATH;
  path /= "media/audio/cheer.wav";
  EXPECT_TRUE(audio.SetFile(path.string()));
  EXPECT_EQ(audio.GetFile(), path.string());
  EXPECT_EQ(audio.GetSampleRate(), 48000);

  unsigned int dataBufferSize;
  uint8_t *dataBuffer = NULL;
  audio.Decode(&dataBuffer, &dataBufferSize);
  EXPECT_EQ(dataBufferSize, 5428736u);
}

/////////////////////////////////////////////////
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
