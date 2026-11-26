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

#ifndef DH_GST_APPSRC_HPP
#define DH_GST_APPSRC_HPP

// local includes
#include "element.hpp"
#include "sharedptrs.hpp"

// std
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

// gstreamer
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

namespace dh::gst
{

/**
 * @class AppSrc
 * @brief A wrapper class for GstAppSrc, the source that feeds buffers from the application into a
 * pipeline.
 */
class AppSrc final : public Element
{
private:
  /**
   * @brief Create a new AppSrc object that wraps a GstAppSrcSPtr.
   * @param gstAppSrc
   */
  explicit AppSrc(const GstAppSrcSPtr& gstAppSrc);

  /**
   * @brief Create a new AppSrc object that wraps a GstAppSrc*.
   * @ref makeGstSharedPtr is used to wrap in a internal shared_ptr
   */
  AppSrc(GstAppSrc* gstAppSrc, TransferType transferType);

public:
  [[nodiscard]] static std::shared_ptr<AppSrc> create(const GstAppSrcSPtr& gstAppSrc);
  [[nodiscard]] static std::shared_ptr<AppSrc>
  create(GstAppSrc* gstAppSrc, TransferType transferType);

  /**
   * @brief Create a new appsrc element.
   * @param name the name of the element. If empty, a unique name is used.
   * @throws std::runtime_error if the appsrc can not be created (e.g. the plugin is missing).
   */
  [[nodiscard]] static std::shared_ptr<AppSrc> create(const std::string& name = "");

  /**
   * @brief Wrap an Element that is an appsrc, e.g. one found by Bin::getElementByName().
   * The AppSrc shares the GstAppSrc with the Element.
   * @throws std::invalid_argument if the element is no appsrc.
   */
  [[nodiscard]] static std::shared_ptr<AppSrc> fromElement(const Element& element);

  /**
   * @brief get the GstAppSrcSPtr of the AppSrc
   * @return the internal GstAppSrc.
   */
  [[nodiscard]] GstAppSrcSPtr getGstAppSrc() const;

  /**
   * @brief Set the caps of the pushed buffers.
   * @param caps the caps. nullptr means unknown: then the caps must be negotiated downstream
   *        or set with pushSample().
   */
  void setCaps(const GstCapsSPtr& caps);

  /**
   * @brief Get the caps of the pushed buffers, nullptr if not set.
   */
  [[nodiscard]] GstCapsSPtr getCaps() const;

  /**
   * @brief Set how the stream can be seeked. Default: GST_APP_STREAM_TYPE_STREAM (no seeking).
   * For the seekable types, connect a seek-data callback (connectSeekData()).
   */
  void setStreamType(GstAppStreamType streamType);
  [[nodiscard]] GstAppStreamType getStreamType() const;

  /**
   * @brief Set the size of the stream in bytes, -1 (default) if unknown.
   */
  void setSize(std::int64_t size);
  [[nodiscard]] std::int64_t getSize() const;

  /**
   * @brief Set the maximum number of bytes in the internal queue.
   * When the queue is full, the enough-data callback is called and, if the "block" property is set,
   * pushBuffer() blocks until there is space again.
   * @param maxBytes the maximum number of bytes, 0 means unlimited.
   */
  void setMaxBytes(std::uint64_t maxBytes);
  [[nodiscard]] std::uint64_t getMaxBytes() const;

  /**
   * @brief Get the number of bytes currently in the internal queue.
   */
  [[nodiscard]] std::uint64_t getCurrentLevelBytes() const;

  /**
   * @brief Push a buffer into the internal queue. appsrc takes its own reference.
   * Do not change the buffer after pushing it.
   * Buffers pushed before the appsrc is started are queued.
   * @return GST_FLOW_OK, GST_FLOW_FLUSHING if the appsrc is flushing, e.g. after it was stopped
   *         (the buffer is dropped), or GST_FLOW_EOS after endOfStream().
   * @throws std::invalid_argument if buffer is nullptr
   */
  GstFlowReturn pushBuffer(const GstBufferSPtr& buffer);

  /**
   * @brief Push the buffer of a sample into the internal queue. The caps of the sample replace the
   * caps of the appsrc (see setCaps()), if they are set.
   * @return see pushBuffer()
   * @throws std::invalid_argument if sample is nullptr or has no buffer
   */
  GstFlowReturn pushSample(const GstSampleSPtr& sample);

  /**
   * @brief Signal that no more buffers will be pushed.
   * The queued buffers are still sent downstream.
   * @return GST_FLOW_OK, or GST_FLOW_FLUSHING if the appsrc is flushing, e.g. after it was stopped.
   */
  GstFlowReturn endOfStream();

  // callbacks
  // They are called from the streaming thread, enough-data also from the thread of pushBuffer().
  // Only one callback per kind can be connected at once, see CallbackSlot for thread safety.

  /**
   * @brief Connect a callback that is called when the appsrc needs more data.
   * The parameter is the number of bytes needed, or UINT_MAX (-1 in GStreamer) if any amount is
   * fine. Push buffers until the enough-data callback is called; pushing can be done from the
   * callback, too. If the callback throws, the exception is logged.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a need-data callback is already connected to this GstAppSrc
   */
  [[nodiscard]] SignalConnection connectNeedData(std::function<void(unsigned int length)> callback);

  /**
   * @brief Connect a callback that is called when the internal queue is full. Stop pushing until
   * the need-data callback is called again.
   * If the callback throws, the exception is logged.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if an enough-data callback is already connected to this GstAppSrc
   */
  [[nodiscard]] SignalConnection connectEnoughData(std::function<void()> callback);

  /**
   * @brief Connect a callback that is called when the stream must continue at another position.
   * Only called for GST_APP_STREAM_TYPE_SEEKABLE and GST_APP_STREAM_TYPE_RANDOM_ACCESS streams.
   * The parameter is the new position: bytes for the default format GST_FORMAT_BYTES, otherwise
   * in the format of the "format" property (e.g. nanoseconds for GST_FORMAT_TIME).
   * The callback returns whether the seek succeeded. Without a connected callback, seeking fails.
   * If the callback throws, the exception is logged and the seek fails.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a seek-data callback is already connected to this GstAppSrc
   */
  [[nodiscard]] SignalConnection
  connectSeekData(std::function<bool(std::uint64_t offset)> callback);

private:
  [[nodiscard]] GstAppSrc* getRawGstAppSrc();
  [[nodiscard]] const GstAppSrc* getRawGstAppSrc() const;
};

} // namespace dh::gst

#endif // DH_GST_APPSRC_HPP
