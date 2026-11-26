/* -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file linkPipelineElements.cpp
 * @brief This program starts a test video, rotates it 45° and displays it.
 * quit with [ctl]+[C] or by closing the window
 * @author Sandro Stiller
 * @date 2024-09-15
 */

#include "elementfactory.hpp"
#include "pipeline.hpp"

// C
#include <glib-unix.h>
#include <gst/gst.h>

// std
#include <csignal>
#include <iostream>

int main(int argc, char** argv)
{
  gst_init(&argc, &argv);

  using namespace dh::gst;

  // create an empty Pipeline
  auto pipeline = Pipeline::create("TestPipeline");

  // create elements and add them to the pipeline
  auto srcElement = ElementFactory::makeElement("videotestsrc", "srcElement");
  pipeline->addElement(*srcElement);

  auto rotateElement = ElementFactory::makeElement("rotate", "rotateElement");
  rotateElement->setProperty("angle", .7854); // 45 degrees
  pipeline->addElement(*rotateElement);

  auto convertElement = ElementFactory::makeElement("videoconvert", "convertElement");
  pipeline->addElement(*convertElement);

  auto dstElement = ElementFactory::makeElement("fpsdisplaysink", "dstElement");
  pipeline->addElement(*dstElement);

  // link all the elements
  srcElement->link(*rotateElement).link(*convertElement).link(*dstElement);

  GMainLoop* mainLoop{g_main_loop_new(nullptr, FALSE)};

  // quit on Ctrl+C
  g_unix_signal_add(
    SIGINT,
    [](gpointer loop) -> gboolean
    {
      g_main_loop_quit(static_cast<GMainLoop*>(loop));
      return G_SOURCE_REMOVE;
    },
    mainLoop
  );

  // quit on error (also posted when the window is closed) or end of stream
  gst_bus_add_watch(
    pipeline->getBus()->getGstBus().get(),
    [](GstBus*, GstMessage* message, gpointer loop) -> gboolean
    {
      if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR)
      {
        GError* error{nullptr};
        gst_message_parse_error(message, &error, nullptr);
        std::cerr << "Error: " << error->message << std::endl;
        g_error_free(error);
      }
      if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR
          || GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS)
      {
        g_main_loop_quit(static_cast<GMainLoop*>(loop));
      }
      return G_SOURCE_CONTINUE;
    },
    mainLoop
  );

  pipeline->setState(GST_STATE_PLAYING);
  g_main_loop_run(mainLoop);

  pipeline->setState(GST_STATE_NULL);
  g_main_loop_unref(mainLoop);
  return 0;
}
