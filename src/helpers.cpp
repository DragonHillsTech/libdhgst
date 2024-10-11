/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file helpers.cpp
 * @author Sandro Stiller
 * @date 2024-10-11
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


} // dh::gst::helpers
