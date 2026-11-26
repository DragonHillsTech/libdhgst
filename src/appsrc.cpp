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

// local includes
#include "appsrc.hpp"
#include "element.hpp"
#include "internal/appcallbacks.hpp"
#include "internal/callbackslot.hpp"
#include "sharedptrs.hpp"
#include "signalconnection.hpp"
#include "transfertype.hpp"

// std
#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

namespace dh::gst
{

namespace
{

using NeedDataSlot = CallbackSlot<unsigned int>;
using EnoughDataSlot = CallbackSlot<>;
// the callback stores its result in the bool&, CallbackSlot only supports callbacks without return value
using SeekDataSlot = CallbackSlot<std::uint64_t, bool&>;

/**
 * @brief The callback slots of one GstAppSrc, shared by all AppSrc wrappers of it, see appcallbacks.hpp.
 */
struct CallbackSlots
{
  const std::shared_ptr<NeedDataSlot> needDataSlot{NeedDataSlot::create("need-data")};
  const std::shared_ptr<EnoughDataSlot> enoughDataSlot{EnoughDataSlot::create("enough-data")};
  const std::shared_ptr<SeekDataSlot> seekDataSlot{SeekDataSlot::create("seek-data")};

  /// false until the callbacks are installed, and again after gst_app_src_set_callbacks replaced them
  std::atomic<bool> installed{false};
};

GQuark callbackSlotsQuark()
{
  static const GQuark quark = g_quark_from_static_string("dh-gst-appsrc-callback-slots");
  return quark;
}

// We are called from C code (the streaming thread), so exceptions must not unwind through it.

void onNeedData(GstAppSrc* appSrc, guint length, gpointer userData)
{
  try
  {
    slotsFromUserData<CallbackSlots>(userData).needDataSlot->call(length);
  }
  catch(const std::exception& e)
  {
    GST_ERROR_OBJECT(appSrc, "Exception in need-data callback: %s", e.what());
  }
  catch(...)
  {
    GST_ERROR_OBJECT(appSrc, "Unknown exception in need-data callback");
  }
}

void onEnoughData(GstAppSrc* appSrc, gpointer userData)
{
  try
  {
    slotsFromUserData<CallbackSlots>(userData).enoughDataSlot->call();
  }
  catch(const std::exception& e)
  {
    GST_ERROR_OBJECT(appSrc, "Exception in enough-data callback: %s", e.what());
  }
  catch(...)
  {
    GST_ERROR_OBJECT(appSrc, "Unknown exception in enough-data callback");
  }
}

gboolean onSeekData(GstAppSrc* appSrc, guint64 offset, gpointer userData)
{
  // stays false if no callback is connected: like a seek-data signal without handler
  bool success = false;
  try
  {
    slotsFromUserData<CallbackSlots>(userData).seekDataSlot->call(offset, success);
  }
  catch(const std::exception& e)
  {
    GST_ERROR_OBJECT(appSrc, "Exception in seek-data callback: %s", e.what());
    return FALSE;
  }
  catch(...)
  {
    GST_ERROR_OBJECT(appSrc, "Unknown exception in seek-data callback");
    return FALSE;
  }
  return success;
}

/**
 * @brief Get the CallbackSlots of the GstAppSrc, create them and install the callbacks if needed.
 */
std::shared_ptr<CallbackSlots> getCallbackSlots(GstAppSrc& appSrc)
{
  GstAppSrcCallbacks callbacks{};
  callbacks.need_data = &onNeedData;
  callbacks.enough_data = &onEnoughData;
  callbacks.seek_data = &onSeekData;
  return getInstalledCallbackSlots<CallbackSlots>(appSrc, callbackSlotsQuark(), callbacks, &gst_app_src_set_callbacks);
}

} // namespace

AppSrc::AppSrc(const GstAppSrcSPtr& gstAppSrc)
  : Element(
      GST_ELEMENT_CAST(gstAppSrc.get()),
      TransferType::none
    ) // no pointer_cast because C inheritance
{
}

AppSrc::AppSrc(GstAppSrc* gstAppSrc, TransferType transferType)
: Element(GST_ELEMENT_CAST(gstAppSrc), transferType)
{
}

std::shared_ptr<AppSrc> AppSrc::create(const GstAppSrcSPtr& gstAppSrc)
{
  std::shared_ptr<AppSrc> appSrc(new AppSrc(gstAppSrc));
  appSrc->requireType<GstAppSrc>();
  return appSrc;
}

std::shared_ptr<AppSrc> AppSrc::create(GstAppSrc* gstAppSrc, TransferType transferType)
{
  std::shared_ptr<AppSrc> appSrc(new AppSrc(gstAppSrc, transferType));
  appSrc->requireType<GstAppSrc>();
  return appSrc;
}

std::shared_ptr<AppSrc> AppSrc::create(const std::string& name)
{
  // gst_element_factory_make: transfer:floating, nullable
  GstElement* gstElement = gst_element_factory_make("appsrc", name.empty() ? nullptr : name.c_str());
  if(!gstElement)
  {
    throw std::runtime_error("Failed to create appsrc element");
  }
  return create(GST_APP_SRC_CAST(gstElement), TransferType::floating);
}

std::shared_ptr<AppSrc> AppSrc::fromElement(const Element& element)
{
  return create(GST_APP_SRC_CAST(element.getGstElement().get()), TransferType::none);
}

GstAppSrcSPtr AppSrc::getGstAppSrc() const
{
  return makeGstSharedPtr(GST_APP_SRC_CAST(getGstObject().get()), TransferType::none);
}

void AppSrc::setCaps(const GstCapsSPtr& caps)
{
  gst_app_src_set_caps(getRawGstAppSrc(), caps.get());
}

GstCapsSPtr AppSrc::getCaps() const
{
  // gst_app_src_get_caps: transfer:full, nullable
  return makeGstSharedPtr(
    gst_app_src_get_caps(const_cast<GstAppSrc*>(getRawGstAppSrc())),
    TransferType::full
  );
}

void AppSrc::setStreamType(GstAppStreamType streamType)
{
  gst_app_src_set_stream_type(getRawGstAppSrc(), streamType);
}

GstAppStreamType AppSrc::getStreamType() const
{
  return gst_app_src_get_stream_type(const_cast<GstAppSrc*>(getRawGstAppSrc()));
}

void AppSrc::setSize(std::int64_t size)
{
  gst_app_src_set_size(getRawGstAppSrc(), size);
}

std::int64_t AppSrc::getSize() const
{
  return gst_app_src_get_size(const_cast<GstAppSrc*>(getRawGstAppSrc()));
}

void AppSrc::setMaxBytes(std::uint64_t maxBytes)
{
  gst_app_src_set_max_bytes(getRawGstAppSrc(), maxBytes);
}

std::uint64_t AppSrc::getMaxBytes() const
{
  return gst_app_src_get_max_bytes(const_cast<GstAppSrc*>(getRawGstAppSrc()));
}

std::uint64_t AppSrc::getCurrentLevelBytes() const
{
  return gst_app_src_get_current_level_bytes(const_cast<GstAppSrc*>(getRawGstAppSrc()));
}

GstFlowReturn AppSrc::pushBuffer(const GstBufferSPtr& buffer)
{
  if(!buffer)
  {
    throw std::invalid_argument("AppSrc::pushBuffer: buffer is nullptr");
  }
  // gst_app_src_push_buffer: transfer:full --> ref before forwarding
  return gst_app_src_push_buffer(getRawGstAppSrc(), gst_buffer_ref(buffer.get()));
}

GstFlowReturn AppSrc::pushSample(const GstSampleSPtr& sample)
{
  if(!sample || !gst_sample_get_buffer(sample.get()))
  {
    throw std::invalid_argument("AppSrc::pushSample: sample is nullptr or has no buffer");
  }
  // gst_app_src_push_sample: transfer:none
  return gst_app_src_push_sample(getRawGstAppSrc(), sample.get());
}

GstFlowReturn AppSrc::endOfStream()
{
  return gst_app_src_end_of_stream(getRawGstAppSrc());
}

SignalConnection AppSrc::connectNeedData(std::function<void(unsigned int length)> callback)
{
  return getCallbackSlots(*getRawGstAppSrc())->needDataSlot->connect(std::move(callback));
}

SignalConnection AppSrc::connectEnoughData(std::function<void()> callback)
{
  return getCallbackSlots(*getRawGstAppSrc())->enoughDataSlot->connect(std::move(callback));
}

SignalConnection AppSrc::connectSeekData(std::function<bool(std::uint64_t offset)> callback)
{
  // check here: the adapter below is never empty
  if(!callback)
  {
    throw std::invalid_argument("empty signal callback");
  }
  return getCallbackSlots(*getRawGstAppSrc())
    ->seekDataSlot->connect(
      [callback = std::move(callback)](std::uint64_t offset, bool& success)
      {
        success = callback(offset);
      }
    );
}

GstAppSrc* AppSrc::getRawGstAppSrc()
{
  return GST_APP_SRC_CAST(getRawGstObject());
}

const GstAppSrc* AppSrc::getRawGstAppSrc() const
{
  return GST_APP_SRC_CAST(getRawGstObject());
}

} // namespace dh::gst
