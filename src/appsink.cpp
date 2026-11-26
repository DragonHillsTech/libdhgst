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
#include "appsink.hpp"
#include "element.hpp"
#include "internal/appcallbacks.hpp"
#include "internal/callbackslot.hpp"
#include "sharedptrs.hpp"
#include "signalconnection.hpp"
#include "transfertype.hpp"

// std
#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

namespace dh::gst
{

namespace
{

using SampleSlot = CallbackSlot<GstSampleSPtr>;
using EndOfStreamSlot = CallbackSlot<>;

/**
 * @brief The callback slots of one GstAppSink, shared by all AppSink wrappers of it, see appcallbacks.hpp.
 */
struct CallbackSlots
{
  const std::shared_ptr<SampleSlot> newSampleSlot{SampleSlot::create("new-sample")};
  const std::shared_ptr<SampleSlot> newPrerollSlot{SampleSlot::create("new-preroll")};
  const std::shared_ptr<EndOfStreamSlot> endOfStreamSlot{EndOfStreamSlot::create("end-of-stream")};

  /// false until the callbacks are installed, and again after gst_app_sink_set_callbacks replaced them
  std::atomic<bool> installed{false};
};

GQuark callbackSlotsQuark()
{
  static const GQuark quark = g_quark_from_static_string("dh-gst-appsink-callback-slots");
  return quark;
}

/**
 * @brief Take the next sample with pull and pass it to the callback of slot.
 * We are called from C code (the streaming thread), so exceptions must not unwind through it.
 */
template<typename PullFunction>
GstFlowReturn callSampleSlot(GstAppSink* appSink, SampleSlot& slot, PullFunction pull, const char* slotName)
{
  // without a callback, leave the sample queued for pullSample()
  if(!slot.hasCallback())
  {
    return GST_FLOW_OK;
  }

  // try_pull with timeout 0: another thread may have pulled the sample already, don't block then
  GstSampleSPtr sample = makeGstSharedPtr(pull(appSink, 0), TransferType::full);
  if(!sample)
  {
    return GST_FLOW_OK;
  }

  try
  {
    slot.call(std::move(sample));
  }
  catch(const std::exception& e)
  {
    GST_ERROR_OBJECT(appSink, "Exception in %s callback: %s", slotName, e.what());
    return GST_FLOW_ERROR;
  }
  catch(...)
  {
    GST_ERROR_OBJECT(appSink, "Unknown exception in %s callback", slotName);
    return GST_FLOW_ERROR;
  }
  return GST_FLOW_OK;
}

GstFlowReturn onNewSample(GstAppSink* appSink, gpointer userData)
{
  return callSampleSlot(
    appSink, *slotsFromUserData<CallbackSlots>(userData).newSampleSlot, &gst_app_sink_try_pull_sample, "new-sample"
  );
}

GstFlowReturn onNewPreroll(GstAppSink* appSink, gpointer userData)
{
  return callSampleSlot(
    appSink, *slotsFromUserData<CallbackSlots>(userData).newPrerollSlot, &gst_app_sink_try_pull_preroll, "new-preroll"
  );
}

void onEndOfStream(GstAppSink* appSink, gpointer userData)
{
  try
  {
    slotsFromUserData<CallbackSlots>(userData).endOfStreamSlot->call();
  }
  catch(const std::exception& e)
  {
    GST_ERROR_OBJECT(appSink, "Exception in end-of-stream callback: %s", e.what());
  }
  catch(...)
  {
    GST_ERROR_OBJECT(appSink, "Unknown exception in end-of-stream callback");
  }
}

/**
 * @brief Get the CallbackSlots of the GstAppSink, create them and install the callbacks if needed.
 */
std::shared_ptr<CallbackSlots> getCallbackSlots(GstAppSink& appSink)
{
  GstAppSinkCallbacks callbacks{};
  callbacks.eos = &onEndOfStream;
  callbacks.new_preroll = &onNewPreroll;
  callbacks.new_sample = &onNewSample;
  return getInstalledCallbackSlots<CallbackSlots>(appSink, callbackSlotsQuark(), callbacks, &gst_app_sink_set_callbacks);
}

} // namespace

AppSink::AppSink(const GstAppSinkSPtr& gstAppSink)
  : Element(
      GST_ELEMENT_CAST(gstAppSink.get()),
      TransferType::none
    ) // no pointer_cast because C inheritance
{
}

AppSink::AppSink(GstAppSink* gstAppSink, TransferType transferType)
: Element(GST_ELEMENT_CAST(gstAppSink), transferType)
{
}

std::shared_ptr<AppSink> AppSink::create(const GstAppSinkSPtr& gstAppSink)
{
  std::shared_ptr<AppSink> appSink(new AppSink(gstAppSink));
  appSink->requireType<GstAppSink>();
  return appSink;
}

std::shared_ptr<AppSink> AppSink::create(GstAppSink* gstAppSink, TransferType transferType)
{
  std::shared_ptr<AppSink> appSink(new AppSink(gstAppSink, transferType));
  appSink->requireType<GstAppSink>();
  return appSink;
}

std::shared_ptr<AppSink> AppSink::create(const std::string& name)
{
  // gst_element_factory_make: transfer:floating, nullable
  GstElement* gstElement = gst_element_factory_make("appsink", name.empty() ? nullptr : name.c_str());
  if(!gstElement)
  {
    throw std::runtime_error("Failed to create appsink element");
  }
  return create(GST_APP_SINK_CAST(gstElement), TransferType::floating);
}

std::shared_ptr<AppSink> AppSink::fromElement(const Element& element)
{
  return create(GST_APP_SINK_CAST(element.getGstElement().get()), TransferType::none);
}

GstAppSinkSPtr AppSink::getGstAppSink() const
{
  return makeGstSharedPtr(GST_APP_SINK_CAST(getGstObject().get()), TransferType::none);
}

void AppSink::setCaps(const GstCapsSPtr& caps)
{
  gst_app_sink_set_caps(getRawGstAppSink(), caps.get());
}

GstCapsSPtr AppSink::getCaps() const
{
  // gst_app_sink_get_caps: transfer:full, nullable
  return makeGstSharedPtr(
    gst_app_sink_get_caps(const_cast<GstAppSink*>(getRawGstAppSink())),
    TransferType::full
  );
}

bool AppSink::isEos() const
{
  return gst_app_sink_is_eos(const_cast<GstAppSink*>(getRawGstAppSink()));
}

void AppSink::setMaxBuffers(unsigned int maxBuffers)
{
  gst_app_sink_set_max_buffers(getRawGstAppSink(), maxBuffers);
}

unsigned int AppSink::getMaxBuffers() const
{
  return gst_app_sink_get_max_buffers(const_cast<GstAppSink*>(getRawGstAppSink()));
}

GstSampleSPtr AppSink::pullSample()
{
  // gst_app_sink_pull_sample: transfer:full, nullable
  return makeGstSharedPtr(gst_app_sink_pull_sample(getRawGstAppSink()), TransferType::full);
}

GstSampleSPtr AppSink::tryPullSample(GstClockTime timeout)
{
  // gst_app_sink_try_pull_sample: transfer:full, nullable
  return makeGstSharedPtr(
    gst_app_sink_try_pull_sample(getRawGstAppSink(), timeout),
    TransferType::full
  );
}

GstSampleSPtr AppSink::pullPreroll()
{
  // gst_app_sink_pull_preroll: transfer:full, nullable
  return makeGstSharedPtr(gst_app_sink_pull_preroll(getRawGstAppSink()), TransferType::full);
}

GstSampleSPtr AppSink::tryPullPreroll(GstClockTime timeout)
{
  // gst_app_sink_try_pull_preroll: transfer:full, nullable
  return makeGstSharedPtr(
    gst_app_sink_try_pull_preroll(getRawGstAppSink(), timeout),
    TransferType::full
  );
}

SignalConnection AppSink::connectNewSample(std::function<void(GstSampleSPtr)> callback)
{
  return getCallbackSlots(*getRawGstAppSink())->newSampleSlot->connect(std::move(callback));
}

SignalConnection AppSink::connectNewPreroll(std::function<void(GstSampleSPtr)> callback)
{
  return getCallbackSlots(*getRawGstAppSink())->newPrerollSlot->connect(std::move(callback));
}

SignalConnection AppSink::connectEndOfStream(std::function<void()> callback)
{
  return getCallbackSlots(*getRawGstAppSink())->endOfStreamSlot->connect(std::move(callback));
}

GstAppSink* AppSink::getRawGstAppSink()
{
  return GST_APP_SINK_CAST(getRawGstObject());
}

const GstAppSink* AppSink::getRawGstAppSink() const
{
  return GST_APP_SINK_CAST(getRawGstObject());
}

} // namespace dh::gst
