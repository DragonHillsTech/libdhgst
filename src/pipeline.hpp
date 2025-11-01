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

#ifndef DH_GST_PIPELINE_HPP
#define DH_GST_PIPELINE_HPP

// local includes
#include "bin.hpp"
#include "bus.hpp"
#include "sharedptrs.hpp"

// gstreamer
#include <gst/gst.h>

namespace dh::gst
{

class Pipeline final : public Bin
{
protected:
  /**
   * @brief Create a new Pipeline object that wraps a GstPipelineSPtr.
   * @param gstPipeline
   */
  explicit Pipeline(GstPipelineSPtr gstPipeline);

  /**
   * @brief Create a new Pipeline object that wraps a GstPipeline*.
   * @ref makeGstSharedPtr is used to wrap in a internal shared_ptr
   * @param gstPipeline
   * @param transferType see if None, then increase use count
   */
  Pipeline(GstPipeline* gstPipeline, TransferType transferType);

  /**
  * @brief create an empty Pipeline with the given name
  * @param name the name of the Pipeline
  */
  explicit Pipeline(const std::string& name);
public:
  [[nodiscard]] static std::shared_ptr<Pipeline> create(GstPipelineSPtr gstPipeline);
  [[nodiscard]] static std::shared_ptr<Pipeline> create(const std::string& name);
  [[nodiscard]] static std::shared_ptr<Pipeline> create(GstPipeline* gstPipeline, TransferType transferType);

  /**
   * @brief Create a new Pipeline object from a pipeline description string.
   * If there was a recoverable error, a warning is printed
   * If parsing does not create a GstPipeline (could be a GstElement if only one entry), std::runtime_error is thrown.
   * @param description The GStreamer pipeline description.
   * @return A Pipeline object created from the description.
   * @throws std::runtime_error if the description is invalid or parsing fails.
   */
  [[nodiscard]] static Pipeline fromDescription(const std::string& description);

 /**
   * @brief Gets the current clock used by the pipeline.
   * Unlike @ref getClock, this function will always return a clock, even if the pipeline is not in the PLAYING state.
   * @return The current clock.
   */
  [[nodiscard]] GstClockSPtr getPipelineClock() const;

  /**
   * @brief Sets the clock to be used by the pipeline.
   * The clock will be distributed to all the elements managed by the pipeline.
   * @param clock The clock to set.
   */
  void setPipelineClock(GstClockSPtr clock);

  // Does not exist for older gstreamer. Enable when needed
  // /**
  //  * @brief Check if pipeline is live.
  //  * @return true if pipeline is live, false if not or if it did not reach the PAUSED state yet
  //  */
  // [[nodiscard]] bool isLive() const;

  /**
   * @brief Get the GstPipelineSPtr of the Pipeline.
   * @return The GstPipeline shared pointer.
   */
  [[nodiscard]] GstPipelineSPtr getGstPipeline();
  [[nodiscard]] const GstPipelineSPtr getGstPipeline() const;

  [[nodiscard]] std::shared_ptr<Bus> getBus() const;

private:
  [[nodiscard]] GstPipeline* getRawGstPipeline();
  [[nodiscard]] const GstPipeline* getRawGstPipeline() const;
};

} // namespace dh::gst

#endif // DH_GST_PIPELINE_HPP
