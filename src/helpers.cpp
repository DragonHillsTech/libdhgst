/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/* Copyright (C) 2024 Sandro Stiller <sandro.stiller@dragonhills.de>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of Libdhgst <https://dragonhills.de/>.
 *
 * Libdhgst is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Libdhgst is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Libdhgst.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "helpers.hpp"

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

  GstVideoInfo vinfo;
  gst_video_info_init(&vinfo);

  vinfo.width = videoMeta->width;
  vinfo.height = videoMeta->height;
  vinfo.finfo = gst_video_format_get_info(videoMeta->format);
  if (!vinfo.finfo)
  {
    throw std::runtime_error("Invalid video format in buffer metadata");
  }

  return vinfo;
}


} // dh::gst::helpers
