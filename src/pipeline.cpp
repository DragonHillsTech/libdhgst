/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file pipeline.cpp
 * @author Sandro Stiller
 * @date 2024-09-15
 */

// local includes
#include "pipeline.hpp"
#include "spdlog/spdlog.h"

// std
#include <stdexcept>

namespace dh::gst
{

Pipeline::Pipeline(GstPipelineSPtr gstPipeline)
: Bin(GST_BIN_CAST(gstPipeline.get()), TransferType::None) // no pointer_cast because C inheritance
{
}

Pipeline::Pipeline(GstPipeline* gstPipeline, TransferType transferType)
: Bin(GST_BIN_CAST(gstPipeline), transferType)
{
}

Pipeline::Pipeline(const std::string& name)
: Bin(GST_BIN_CAST(gst_pipeline_new(name.empty() ? nullptr : name.c_str())), TransferType::Floating)
{
}

Pipeline Pipeline::fromDescription(const std::string& description)
{
  GError* error = nullptr;
  GstElement* pipeline = gst_parse_launch(description.c_str(), &error);

  if(error)
  {
    spdlog::warn("Failed to parse pipeline description: {}", error->message);
    g_error_free(error);
  }

  if(!pipeline)
  {
    throw std::runtime_error("Failed to create pipeline from description");
  }

  if(!GST_IS_PIPELINE(pipeline))
  {
    gst_object_unref(pipeline);
    throw std::runtime_error("Pipeline description does not create a GstPipeline");
  }

  return Pipeline(GST_PIPELINE(pipeline), TransferType::Floating);
}

Pipeline Pipeline::ref()
{
  return Pipeline(getGstPipeline());
}

GstClockSPtr Pipeline::getPipelineClock() const
{
  return makeGstSharedPtr(
    gst_pipeline_get_pipeline_clock(const_cast<GstPipeline*>(getRawGstPipeline())),
    TransferType::Full
  );
}

void Pipeline::setPipelineClock(GstClockSPtr clock)
{
  // transfer: none :-)
  gst_pipeline_set_clock(getRawGstPipeline(), clock.get());
}

// Does not exist for older gstreamer. Enable when needed
// bool Pipeline::isLive() const
// {
//   return gst_pipeline_is_live(const_cast<GstPipeline*>(getRawGstPipeline()));
// }

GstPipelineSPtr Pipeline::getGstPipeline()
{
  return makeGstSharedPtr(GST_PIPELINE_CAST(getGstObject().get()), TransferType::None);
}

const GstPipelineSPtr Pipeline::getGstPipeline() const
{
  return makeGstSharedPtr(GST_PIPELINE_CAST(getGstObject().get()), TransferType::None);
}

GstPipeline* Pipeline::getRawGstPipeline()
{
  return GST_PIPELINE_CAST(getRawGstObject());
}

const GstPipeline* Pipeline::getRawGstPipeline() const
{
  return GST_PIPELINE_CAST(getRawGstObject());
}

} // namespace dh::gst
