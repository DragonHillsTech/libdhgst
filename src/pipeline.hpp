/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file pipeline.hpp
 * @author Sandro Stiller
 * @date 2024-09-15
 */

#ifndef DH_GST_PIPELINE_HPP
#define DH_GST_PIPELINE_HPP

// local includes
#include "bin.hpp"
#include "sharedptrs.hpp"

// gstreamer
#include <gst/gst.h>

namespace dh::gst
{

class Pipeline final : public Bin
{
public:
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

  /**
   * @brief Create a new Pipeline object from a pipeline description string.
   * If there was a recoverable error, a warning is printed (spdlog::warn)
   * If parsing does not create a GstPipeline (could be a GstElement if only one entry), std::runtime_error is thrown.
   * @param description The GStreamer pipeline description.
   * @return A Pipeline object created from the description.
   * @throws std::runtime_error if the description is invalid or parsing fails.
   */
  [[nodiscard]] static Pipeline fromDescription(const std::string& description);

  /**
   * @brief Create a reference to the same Pipeline.
   * @return A new Pipeline object that shares the internal GstPipeline*.
   */
  [[nodiscard]] Pipeline ref();

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

  /**
   * @brief Check if pipeline is live.
   * @return true if pipeline is live, false if not or if it did not reach the PAUSED state yet
   */
  [[nodiscard]] bool isLive() const;

  /**
   * @brief Get the GstPipelineSPtr of the Pipeline.
   * @return The GstPipeline shared pointer.
   */
  [[nodiscard]] GstPipelineSPtr getGstPipeline();
  [[nodiscard]] const GstPipelineSPtr getGstPipeline() const;

private:
  [[nodiscard]] GstPipeline* getRawGstPipeline();
  [[nodiscard]] const GstPipeline* getRawGstPipeline() const;
};

} // namespace dh::gst

#endif // DH_GST_PIPELINE_HPP
