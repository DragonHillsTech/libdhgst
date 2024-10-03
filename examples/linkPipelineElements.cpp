/* -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file pipelineFromDesc.cpp
 * @author Sandro Stiller
 * @date 2024-09-15
 */

#include "pipeline.hpp"
#include "elementfactory.hpp"

#include <spdlog/spdlog.h>

int main(const int argc, const char **argv)
{
  gst_init(const_cast<int*>(&argc), const_cast<char***>(&argv));

  using namespace dh::gst;

  auto pipeline = Pipeline::create("TestPipeline");
  auto srcElement = ElementFactory::makeElement("videotestsrc", "srcElement");
  pipeline->addElement(srcElement);

  auto rotateElement = ElementFactory::makeElement("rotate", "rotateElement");
  rotateElement->setProperty("angle", .7854); // 45 degrees
  pipeline->addElement(rotateElement);

  auto convertElement = ElementFactory::makeElement("videoconvert", "convertElement");
  pipeline->addElement(convertElement);

  auto dstElement = ElementFactory::makeElement("fpsdisplaysink", "dstElement");
  pipeline->addElement(dstElement);

  srcElement->link(rotateElement)->link(convertElement)->link(dstElement);

  pipeline->setState(GST_STATE_PLAYING);

  gst_deinit(); // hangs :-) That's good because we currently have no main loop
  return 0;
}
