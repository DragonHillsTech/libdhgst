/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * The GStreamer "Hello world!" (basic tutorial 1) with libdhgst: play a video with playbin
 * until it ends or an error occurs.
 * https://gstreamer.freedesktop.org/documentation/tutorials/basic/hello-world.html
 */

// libdhgst
#include "bus.hpp"
#include "pipeline.hpp"
#include "sharedptrs.hpp"

// gstreamer
#include <gst/gst.h>

// std
#include <iostream>

int main(int argc, char** argv)
{
  gst_init(&argc, &argv);

  // Build the pipeline
  auto pipeline{dh::gst::Pipeline::fromDescription(
    "playbin uri=https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm"
  )};

  // Start playing
  pipeline->setState(GST_STATE_PLAYING);

  // Wait until error or EOS
  const dh::gst::GstMessageSPtr message{pipeline->getBus()->timedPop(
    GST_CLOCK_TIME_NONE,
    static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS)
  )};

  if (GST_MESSAGE_TYPE(message.get()) == GST_MESSAGE_ERROR)
  {
    std::cerr << "An error occurred! Re-run with the GST_DEBUG=*:WARN environment variable set "
                 "for more details.\n";
  }

  // Free resources: the smart pointers release the message and the pipeline when main returns
  pipeline->setState(GST_STATE_NULL);
  return 0;
}
