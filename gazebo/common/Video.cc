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

#include "gazebo/gazebo_config.h"
#include "gazebo/common/Console.hh"
#include "gazebo/common/Video.hh"
#include "gazebo/common/ffmpeg_inc.h"

using namespace gazebo;
using namespace common;

// suppress deprecation warnings for ffmpeg3
// av_free_packet has been replaced by av_packet_unref
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

/// \brief Destination audio video frame
/// TODO Do not merge forward. Declared here for gazebo7 ABI compatibility
AVFrame *avFrameDst;

/////////////////////////////////////////////////
// #ifdef HAVE_FFMPEG
// static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
//                      char *filename)
// {
//   FILE *f;
//   int i;
//
//   f = fopen(filename, "w");
//   fprintf(f, "P6\n%d %d\n%d\n", xsize, ysize, 255);
//   for(i = 0; i < ysize; ++i)
//     fwrite(buf + i * wrap, 1, xsize * 3, f);
//   fclose(f);
// }
// #endif

/////////////////////////////////////////////////
Video::Video()
{
  this->formatCtx = NULL;
  this->codecCtx = NULL;
  this->swsCtx = NULL;
  this->avFrame = NULL;
  this->videoStream = -1;

  this->pic = NULL;
  avFrameDst = NULL;
}

/////////////////////////////////////////////////
Video::~Video()
{
  this->Cleanup();
}

/////////////////////////////////////////////////
void Video::Cleanup()
{
#ifdef HAVE_FFMPEG
  // Free the YUV frame
  av_free(this->avFrame);

  // Close the video file
  avformat_close_input(&this->formatCtx);

  // Close the codec
  avcodec_close(this->codecCtx);

  av_free(avFrameDst);
#endif
}

/////////////////////////////////////////////////
#ifdef HAVE_FFMPEG
bool Video::Load(const std::string &_filename)
{
  AVCodec *codec = NULL;
  this->videoStream = -1;

  if (this->formatCtx || this->avFrame || this->codecCtx)
    this->Cleanup();

  this->avFrame = common::AVFrameAlloc();

  // Open video file
  if (avformat_open_input(&this->formatCtx, _filename.c_str(), NULL, NULL) < 0)
  {
    gzerr << "Unable to read video file[" << _filename << "]\n";
    return false;
  }

  // Retrieve stream information
  if (avformat_find_stream_info(this->formatCtx, NULL) < 0)
  {
    gzerr << "Couldn't find stream information\n";
    return false;
  }

  // Find the first video stream
  for (unsigned int i = 0; i < this->formatCtx->nb_streams; ++i)
  {
    if (this->formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      this->videoStream = static_cast<int>(i);
      break;
    }
  }

  if (this->videoStream == -1)
  {
    gzerr << "Unable to find a video stream\n";
    return false;
  }

  // Get a pointer to the codec context for the video stream
  this->codecCtx = this->formatCtx->streams[this->videoStream]->codec;

  // Find the decoder for the video stream
  codec = avcodec_find_decoder(this->codecCtx->codec_id);
  if (codec == NULL)
  {
    gzerr << "Codec not found\n";
    return false;
  }

  // Inform the codec that we can handle truncated bitstreams -- i.e.,
  // bitstreams where frame boundaries can fall in the middle of packets
  if (codec->capabilities & CODEC_CAP_TRUNCATED)
    this->codecCtx->flags |= CODEC_FLAG_TRUNCATED;

  // Open codec
  if (avcodec_open2(this->codecCtx, codec, NULL) < 0)
  {
    gzerr << "Could not open codec\n";
    return false;
  }

  this->swsCtx = sws_getContext(
      this->codecCtx->width,
      this->codecCtx->height,
      this->codecCtx->pix_fmt,
      this->codecCtx->width,
      this->codecCtx->height,
      AV_PIX_FMT_RGB24,
      SWS_BICUBIC, NULL, NULL, NULL);

  if (this->swsCtx == NULL)
  {
    gzerr << "Error while calling sws_getContext\n";
    return false;
  }

  avFrameDst = common::AVFrameAlloc();
  avFrameDst->format = this->codecCtx->pix_fmt;
  avFrameDst->width = this->codecCtx->width;
  avFrameDst->height = this->codecCtx->height;
  av_image_alloc(avFrameDst->data, avFrameDst->linesize,
      this->codecCtx->width, this->codecCtx->height, this->codecCtx->pix_fmt,
      1);

  return true;
}
#else
bool Video::Load(const std::string &/*_filename*/)
{
  return false;
}
#endif

/////////////////////////////////////////////////
#ifdef HAVE_FFMPEG
bool Video::GetNextFrame(unsigned char **_buffer)
{
  AVPacket packet, tmpPacket;
  int frameAvailable = 0;

  av_init_packet(&packet);

  // Read a frame.
  if (av_read_frame(this->formatCtx, &packet) < 0)
    return false;

  if (packet.stream_index == this->videoStream)
  {
    tmpPacket.data = packet.data;
    tmpPacket.size = packet.size;

    // Process all the data in the frame
    while (tmpPacket.size > 0)
    {
      // sending data to libavcodec
      int processedLength = avcodec_decode_video2(this->codecCtx, this->avFrame,
          &frameAvailable, &tmpPacket);
      if (processedLength < 0)
      {
        gzerr << "Error while processing the data\n";
        break;
      }

      tmpPacket.data = tmpPacket.data + processedLength;
      tmpPacket.size = tmpPacket.size - processedLength;

      // processing the image if available
      if (frameAvailable)
      {
        sws_scale(swsCtx, this->avFrame->data, this->avFrame->linesize, 0,
            this->codecCtx->height, avFrameDst->data,
            avFrameDst->linesize);

        memcpy(*_buffer, avFrameDst->data[0],
            this->codecCtx->height * (this->codecCtx->width*3));

        // Debug:
        // pgm_save(this->pic.data[0], this->pic.linesize[0],
        //          this->codecCtx->width, this->codecCtx->height, buf);
      }
    }
  }
  av_free_packet(&packet);

  return true;
}
#else
bool Video::GetNextFrame(unsigned char ** /*_buffer*/)
{
  return false;
}
#endif

/////////////////////////////////////////////////
int Video::GetWidth() const
{
#ifdef HAVE_FFMPEG
  return this->codecCtx->width;
#else
  return 0;
#endif
}

/////////////////////////////////////////////////
int Video::GetHeight() const
{
#ifdef HAVE_FFMPEG
  return this->codecCtx->height;
#else
  return 0;
#endif
}

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
