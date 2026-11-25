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

// local includes
#include "pipeline.hpp"
#include "bin.hpp"
#include "bus.hpp"
#include "sharedptrs.hpp"
#include "transfertype.hpp"

// std
#include <memory>
#include <stdexcept>
#include <string>

namespace dh::gst
{

Pipeline::Pipeline(const GstPipelineSPtr& gstPipeline)
  : Bin(
      GST_BIN_CAST(gstPipeline.get()),
      TransferType::none
    ) // no pointer_cast because C inheritance
{
}

Pipeline::Pipeline(GstPipeline* gstPipeline, TransferType transferType)
  : Bin(GST_BIN_CAST(gstPipeline), transferType)
{
}

Pipeline::Pipeline(const std::string& name)
  : Bin(
      GST_BIN_CAST(gst_pipeline_new(name.empty() ? nullptr : name.c_str())),
      TransferType::floating
    )
{
}

std::shared_ptr<Pipeline> Pipeline::create(const GstPipelineSPtr& gstPipeline)
{
  std::shared_ptr<Pipeline> pipeline(new Pipeline(gstPipeline));
  pipeline->requireType<GstPipeline>();
  return pipeline;
}

std::shared_ptr<Pipeline> Pipeline::create(const std::string& name)
{
  return std::shared_ptr<Pipeline>(new Pipeline(name));
}

std::shared_ptr<Pipeline> Pipeline::create(GstPipeline* gstPipeline, TransferType transferType)
{
  std::shared_ptr<Pipeline> pipeline(new Pipeline(gstPipeline, transferType));
  pipeline->requireType<GstPipeline>();
  return pipeline;
}

std::shared_ptr<Pipeline> Pipeline::fromDescription(const std::string& description)
{
  // gst_parse_launch: transfer:floating
  // Owned (and sunk) right away, so every error path below releases it.
  GError* error = nullptr;
  const GstElementSPtr element =
    makeGstSharedPtr(gst_parse_launch(description.c_str(), &error), TransferType::floating);
  const GErrorSPtr errorSPtr = makeGlibSharedPtr(error, TransferType::full);

  // gst_parse_launch can return a partly built pipeline and still report an error
  // (e.g. an element that doesn't exist or a link that failed): don't hand out a broken pipeline
  if (errorSPtr || !element)
  {
    throw std::runtime_error(
      "Failed to create pipeline from description: "
      + std::string(errorSPtr && errorSPtr->message ? errorSPtr->message : "unknown error")
    );
  }

  if (!GST_IS_PIPELINE(element.get()))
  {
    throw std::runtime_error("Pipeline description does not create a GstPipeline");
  }

  return create(GST_PIPELINE_CAST(element.get()), TransferType::none);
}

GstClockSPtr Pipeline::getPipelineClock() const
{
  return makeGstSharedPtr(
    gst_pipeline_get_pipeline_clock(const_cast<GstPipeline*>(getRawGstPipeline())),
    TransferType::full
  );
}

void Pipeline::setPipelineClock(const GstClockSPtr& clock)
{
  // transfer: none :-)
  gst_pipeline_set_clock(getRawGstPipeline(), clock.get());
}

#if DH_GST_HAS_PIPELINE_IS_LIVE
bool Pipeline::isLive() const
{
  return gst_pipeline_is_live(const_cast<GstPipeline*>(getRawGstPipeline()));
}
#endif

GstPipelineSPtr Pipeline::getGstPipeline()
{
  return makeGstSharedPtr(GST_PIPELINE_CAST(getGstObject().get()), TransferType::none);
}

const GstPipelineSPtr Pipeline::getGstPipeline() const
{
  return makeGstSharedPtr(GST_PIPELINE_CAST(getGstObject().get()), TransferType::none);
}

std::shared_ptr<Bus> Pipeline::getBus() const
{
  // gst_pipeline_get_bus returns  transfer::full
  return Bus::create(
    gst_pipeline_get_bus(const_cast<GstPipeline*>(getRawGstPipeline())),
    TransferType::full
  );
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
