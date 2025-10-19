/* -*- mode: c++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file pipelineFromDesc.cpp
 * @brief This program starts a test video, rotates it 45° and displays it.
 * @author Sandro Stiller
 * @date 2024-09-15
 */

#include "pipeline.hpp"
#include "elementfactory.hpp"


int main(const int argc, const char **argv)
{
  gst_init(const_cast<int*>(&argc), const_cast<char***>(&argv));

  using namespace dh::gst;

  // create an empty Pipeline
  auto pipeline = Pipeline::create("TestPipeline");

  // create elements and add them to the pipeline
  auto srcElement = ElementFactory::makeElement("videotestsrc", "srcElement");
  pipeline->addElement(srcElement);

  auto rotateElement = ElementFactory::makeElement("rotate", "rotateElement");
  rotateElement->setProperty("angle", .7854); // 45 degrees
  pipeline->addElement(rotateElement);

  auto convertElement = ElementFactory::makeElement("videoconvert", "convertElement");
  pipeline->addElement(convertElement);

  auto dstElement = ElementFactory::makeElement("fpsdisplaysink", "dstElement");
  pipeline->addElement(dstElement);

  // link all the elements
  srcElement->link(rotateElement)->link(convertElement)->link(dstElement);

  // start the pipeline
  pipeline->setState(GST_STATE_PLAYING);

  gst_deinit(); // hangs :-) That's good because we currently have no main loop
  return 0;
}

