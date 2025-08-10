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

std::shared_ptr<Pipeline> Pipeline::create(GstPipelineSPtr gstPipeline)
{
  return std::shared_ptr<Pipeline>(new Pipeline(gstPipeline));
}

std::shared_ptr<Pipeline> Pipeline::create(const std::string& name)
{
  return std::shared_ptr<Pipeline>(new Pipeline(name));
}

std::shared_ptr<Pipeline> Pipeline::create(GstPipeline* gstPipeline, TransferType transferType)
{
  return std::shared_ptr<Pipeline>(new Pipeline(gstPipeline, transferType));
}

Pipeline Pipeline::fromDescription(const std::string& description)
{
  // gst_parse_launch: transfer:floating
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

std::shared_ptr<Bus> Pipeline::getBus() const
{
  // gst_pipeline_get_bus returns  transfer::full
  return Bus::create(gst_pipeline_get_bus(const_cast<GstPipeline*>(getRawGstPipeline())), TransferType::Full);
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
