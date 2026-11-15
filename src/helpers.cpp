/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/* Copyright (C) 2024 Sandro Stiller <sandro.stiller@dragonhills.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This file is part of Libdhgst <https://dragonhills.de/>.
 *
 * Libdhgst is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Libdhgst is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Libdhgst. If not, see <http://www.gnu.org/licenses/>.
 */

#include "helpers.hpp"

#include <algorithm>

namespace dh::gst::helpers
{

std::string gstStreamStatusTypeToString(GstStreamStatusType type)
{
 switch (type)
 {
  case GST_STREAM_STATUS_TYPE_CREATE: return "CREATE";
  case GST_STREAM_STATUS_TYPE_ENTER: return "ENTER";
  case GST_STREAM_STATUS_TYPE_LEAVE: return "LEAVE";
  case GST_STREAM_STATUS_TYPE_DESTROY: return "DESTROY";
  case GST_STREAM_STATUS_TYPE_START: return "START";
  case GST_STREAM_STATUS_TYPE_PAUSE: return "PAUSE";
  case GST_STREAM_STATUS_TYPE_STOP: return "STOP";
  default: return "UNKNOWN";
 }
}

GstVideoInfo createVideoInfo(const GstCaps& caps)
{
 GstVideoInfo vinfo;
 gst_video_info_init(&vinfo);

 if (!gst_video_info_from_caps(&vinfo, &caps))
 {
  throw std::runtime_error("Failed to initialize video info from caps");
 }

 return vinfo;
}

GstVideoInfo createVideoInfo(const GstBuffer& buffer)
{
  const GstVideoMeta* videoMeta = gst_buffer_get_video_meta(const_cast<GstBuffer*>(&buffer));
  if(!videoMeta)
  {
    throw std::runtime_error("Buffer does not contain video metadata");
  }

  // format, size and the default layout of the format
  GstVideoInfo vinfo;
  gst_video_info_init(&vinfo);
  if(!gst_video_info_set_format(&vinfo, videoMeta->format, videoMeta->width, videoMeta->height))
  {
    throw std::runtime_error("Invalid video format or size in buffer metadata");
  }
  if(videoMeta->n_planes != GST_VIDEO_INFO_N_PLANES(&vinfo))
  {
    throw std::runtime_error("Number of planes in buffer metadata does not match the video format");
  }

  const guint nPlanes = videoMeta->n_planes;
  gsize frameSize = 0;
  if(GST_VIDEO_FORMAT_INFO_IS_TILED(vinfo.finfo))
  {
    // the strides of tiled formats describe tiles, not rows: assume the frame fills the buffer
    frameSize = gst_buffer_get_size(const_cast<GstBuffer*>(&buffer));
  }
  else
  {
    // The default layout packs the planes without gaps, so the number of rows of a plane is the distance
    // to the next plane (in memory, not by index: e.g. YV12 stores V before U) divided by its stride.
    // With the strides and offsets of the meta, the frame ends where the last plane ends.
    for(guint plane = 0; plane < nPlanes; ++plane)
    {
      const gsize defaultOffset = GST_VIDEO_INFO_PLANE_OFFSET(&vinfo, plane);
      gsize defaultEnd = GST_VIDEO_INFO_SIZE(&vinfo);
      for(guint other = 0; other < nPlanes; ++other)
      {
        const gsize otherOffset = GST_VIDEO_INFO_PLANE_OFFSET(&vinfo, other);
        if(otherOffset > defaultOffset && otherOffset < defaultEnd)
        {
          defaultEnd = otherOffset;
        }
      }
      const gsize rows = (defaultEnd - defaultOffset) / GST_VIDEO_INFO_PLANE_STRIDE(&vinfo, plane);
      frameSize = std::max(frameSize, videoMeta->offset[plane] + rows * videoMeta->stride[plane]);
    }
  }

  // the actual layout of the buffer, which may differ from the default one (e.g. padded rows)
  for(guint plane = 0; plane < nPlanes; ++plane)
  {
    GST_VIDEO_INFO_PLANE_STRIDE(&vinfo, plane) = videoMeta->stride[plane];
    GST_VIDEO_INFO_PLANE_OFFSET(&vinfo, plane) = videoMeta->offset[plane];
  }
  GST_VIDEO_INFO_SIZE(&vinfo) = frameSize;

  return vinfo;
}


} // dh::gst::helpers
