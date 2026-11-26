/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/* Copyright (C) 2026 Sandro Stiller <sandro.stiller@dragonhills.de>
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

#ifndef DH_GST_APPSINK_HPP
#define DH_GST_APPSINK_HPP

// local includes
#include "element.hpp"
#include "sharedptrs.hpp"

// std
#include <functional>
#include <memory>
#include <string>

// gstreamer
#include <gst/app/gstappsink.h>
#include <gst/gst.h>

namespace dh::gst
{

/**
 * @class AppSink
 * @brief A wrapper class for GstAppSink, the sink that hands the samples of a pipeline to the
 * application.
 */
class AppSink final : public Element
{
private:
  /**
   * @brief Create a new AppSink object that wraps a GstAppSinkSPtr.
   * @param gstAppSink
   */
  explicit AppSink(const GstAppSinkSPtr& gstAppSink);

  AppSink(GstAppSink* gstAppSink, TransferType transferType);

public:
  [[nodiscard]] static std::shared_ptr<AppSink> create(const GstAppSinkSPtr& gstAppSink);
  [[nodiscard]] static std::shared_ptr<AppSink>
  create(GstAppSink* gstAppSink, TransferType transferType);

  /**
   * @brief Create a new appsink element.
   * @param name the name of the element. If empty, a unique name is used.
   * @throws std::runtime_error if the appsink can not be created (e.g. the plugin is missing).
   */
  [[nodiscard]] static std::shared_ptr<AppSink> create(const std::string& name = "");

  /**
   * @brief Wrap an Element that is an appsink, e.g. one found by Bin::getElementByName().
   * The AppSink shares the GstAppSink with the Element.
   * @throws std::invalid_argument if the element is no appsink.
   */
  [[nodiscard]] static std::shared_ptr<AppSink> fromElement(const Element& element);

  /**
   * @brief get the GstAppSinkSPtr of the AppSink
   * @return the internal GstAppSink.
   */
  [[nodiscard]] GstAppSinkSPtr getGstAppSink() const;

  /**
   * @brief Set the caps the appsink accepts.
   * @param caps the accepted caps. nullptr accepts any caps.
   */
  void setCaps(const GstCapsSPtr& caps);

  /**
   * @brief Get the caps the appsink accepts.
   * @return the accepted caps, nullptr if any caps are accepted.
   */
  [[nodiscard]] GstCapsSPtr getCaps() const;

  /**
   * @brief Check whether the appsink reached end-of-stream and no samples are queued any more.
   */
  [[nodiscard]] bool isEos() const;

  /**
   * @brief Set the maximum number of queued buffers.
   * If the queue is full, the streaming thread blocks, or old buffers are dropped if "drop" is set.
   * @param maxBuffers the maximum number of buffers, 0 means unlimited.
   */
  void setMaxBuffers(unsigned int maxBuffers);

  /**
   * @brief Get the maximum number of queued buffers, 0 means unlimited.
   */
  [[nodiscard]] unsigned int getMaxBuffers() const;

  /**
   * @brief Pull the next sample. Blocks until a sample is available, end-of-stream is reached
   * or the appsink is set to READY or NULL.
   * Do not call it from the "new-sample" callback (connectNewSample() already passes the sample).
   * @return the sample, nullptr on end-of-stream or if the appsink is not PAUSED or PLAYING.
   */
  [[nodiscard]] GstSampleSPtr pullSample();

  /**
   * @brief Pull the next sample, wait at most timeout.
   * @param timeout how long to wait for a sample. 0 returns immediately,
   *        GST_CLOCK_TIME_NONE waits like pullSample().
   * @return the sample, nullptr on timeout, end-of-stream or if the appsink is not PAUSED/PLAYING.
   */
  [[nodiscard]] GstSampleSPtr tryPullSample(GstClockTime timeout);

  /**
   * @brief Get the preroll sample. Blocks until the appsink prerolled, end-of-stream is reached
   * or the appsink is set to READY or NULL.
   * This consumes the preroll sample: a second call waits for the next preroll,
   * after a flush or state change.
   * The preroll sample is also delivered by pullSample() when the appsink is PLAYING.
   * @return the preroll sample, nullptr on end-of-stream or if the appsink is not PAUSED/PLAYING.
   */
  [[nodiscard]] GstSampleSPtr pullPreroll();

  /**
   * @brief Get the preroll sample, wait at most timeout.
   * @param timeout see tryPullSample()
   * @return the preroll sample, nullptr in the same cases as tryPullSample().
   */
  [[nodiscard]] GstSampleSPtr tryPullPreroll(GstClockTime timeout);

  // callbacks
  // They are called from the streaming thread. Only one callback per kind can be connected at once,
  // see CallbackSlot for the thread safety guarantees.

  /**
   * @brief Connect a callback that is called with every new sample.
   * The sample is pulled from the appsink before the callback is called.
   * While no callback is connected, samples are queued and can be pulled with pullSample().
   * If the callback throws, the exception is logged and GST_FLOW_ERROR is returned to the
   * streaming thread, which stops the pipeline with an error.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a new-sample callback is already connected to this GstAppSink
   */
  [[nodiscard]] SignalConnection connectNewSample(std::function<void(GstSampleSPtr)> callback);

  /**
   * @brief Connect a callback that is called with the preroll sample.
   * Exceptions are handled as in connectNewSample().
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a new-preroll callback is already connected to this GstAppSink
   */
  [[nodiscard]] SignalConnection connectNewPreroll(std::function<void(GstSampleSPtr)> callback);

  /**
   * @brief Connect a callback that is called when end-of-stream is reached.
   * With "wait-on-eos" set (the default), this happens only after all queued samples were pulled.
   * If the callback throws, the exception is logged.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if an end-of-stream callback is already connected to this GstAppSink
   */
  [[nodiscard]] SignalConnection connectEndOfStream(std::function<void()> callback);

private:
  [[nodiscard]] GstAppSink* getRawGstAppSink();
  [[nodiscard]] const GstAppSink* getRawGstAppSink() const;
};

} // namespace dh::gst

#endif // DH_GST_APPSINK_HPP
