/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
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
#include "element.hpp"
#include "elementfactory.hpp"
#include "object.hpp"
#include "pipeline.hpp"
#include "sharedptrs.hpp"
#include "signalconnection.hpp"
#include "transfertype.hpp"

// std
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace dh::gst
{

namespace
{

/**
 * @brief GstElementForeachPadFunc that appends a referenced pad to a std::vector<GstPadSPtr>.
 * The pad is only guaranteed to be valid during the callback, so take our own reference.
 */
gboolean collectPad(GstElement* /*element*/, GstPad* pad, gpointer userData)
{
  auto& pads = *static_cast<std::vector<GstPadSPtr>*>(userData);
  pads.push_back(makeGstSharedPtr(pad, TransferType::none));
  return TRUE; // continue iteration
}

/**
 * @brief Converts the result of a position or duration query.
 * Elements may answer a query successfully with -1 when the value is unknown.
 */
std::optional<std::int64_t> toQueryResult(gboolean answered, gint64 value)
{
  if (!answered || value == -1)
  {
    return std::nullopt;
  }
  return value;
}

bool isLive(GstElement& element, const Element::StateQuery& state)
{
  if (state.result == GST_STATE_CHANGE_NO_PREROLL)
  {
    return true;
  }
#if DH_GST_HAS_PIPELINE_IS_LIVE
  // during a state change, the result is ASYNC also for live elements, only a pipeline still knows
  return GST_IS_PIPELINE(&element) && gst_pipeline_is_live(GST_PIPELINE(&element));
#else
  (void)element;
  return false;
#endif
}

/**
 * @brief Whether sending an end-of-stream event in this state deadlocks: a prerolled, non-live
 * sink blocks the source until PLAYING, while the send holds the state lock PLAYING needs.
 */
bool endOfStreamWouldDeadlock(GstElement& element, const Element::StateQuery& state)
{
  const bool live{isLive(element, state)};
  const bool pausedOrChanging{
    state.current == GST_STATE_PAUSED || state.pending != GST_STATE_VOID_PENDING
  };
  return !live && pausedOrChanging;
}

} // namespace

Element::Element(GstElement* gstElement, TransferType transferType)
  : Object(GST_OBJECT_CAST(gstElement), transferType)
{
}

std::shared_ptr<Element> Element::create(const GstElementSPtr& gstElement)
{
  std::shared_ptr<Element> element(new Element(gstElement));
  element->requireType<GstElement>();
  return element;
}

std::shared_ptr<Element> Element::create(GstElement* gstElement, TransferType transferType)
{
  std::shared_ptr<Element> element(new Element(gstElement, transferType));
  element->requireType<GstElement>();
  return element;
}

Element::Element(const GstElementSPtr& gstElement)
  : Object(
      GST_OBJECT_CAST(gstElement.get()),
      TransferType::none
    ) // no pointer_cast because C inheritance
{
}

GstElementSPtr Element::getGstElement() const
{
  // we can not use *pointer_cast because these are C types
  return makeGstSharedPtr(GST_ELEMENT_CAST(getGstObject().get()), TransferType::none);
}

GstStateChangeReturn Element::setState(GstState newState)
{
  return gst_element_set_state(getRawGstElement(), newState);
}

std::vector<GstPadSPtr> Element::getPads()
{
  std::vector<GstPadSPtr> pads;
  gst_element_foreach_pad(getRawGstElement(), collectPad, &pads);
  return pads;
}

std::vector<GstPadSPtr> Element::getSinkPads()
{
  std::vector<GstPadSPtr> sinkPads;
  gst_element_foreach_sink_pad(getRawGstElement(), collectPad, &sinkPads);
  return sinkPads;
}

std::vector<GstPadSPtr> Element::getSrcPads()
{
  std::vector<GstPadSPtr> srcPads;
  gst_element_foreach_src_pad(getRawGstElement(), collectPad, &srcPads);
  return srcPads;
}

GstPadSPtr Element::getCompatiblePad(const GstPadSPtr& pad, const GstCapsSPtr& caps)
{
  if (!pad)
  {
    throw std::invalid_argument("getCompatiblePad: pad must not be nullptr");
  }
  // gst_element_get_compatible_pad: transfer full, nullable
  return makeGstSharedPtr(
    gst_element_get_compatible_pad(getRawGstElement(), pad.get(), caps.get()),
    TransferType::full
  );
}

GstPadSPtr Element::getStaticPad(const std::string& name)
{
  // gst_element_get_static_pad: transfer full, nullable
  return makeGstSharedPtr(
    gst_element_get_static_pad(getRawGstElement(), name.c_str()),
    TransferType::full
  );
}

Element& Element::link(Element& other)
{
  if (!gst_element_link(getRawGstElement(), other.getRawGstElement()))
  {
    std::string errorMessage = "Failed to link GstElements: ";
    errorMessage += getName() + " -> " + other.getName();

    // Throw an exception with the detailed error message
    throw std::runtime_error(errorMessage);
  }

  return other;
}

std::string Element::getFactoryName() const
{
  auto* gstElementFactoryRawPtr =
    gst_element_get_factory(const_cast<GstElement*>(getRawGstElement()));
  if (!gstElementFactoryRawPtr)
  {
    return {};
  }
  const auto factory = ElementFactory::create(gstElementFactoryRawPtr, TransferType::none);
  return factory->getName();
}

GstClockSPtr Element::getElementClock() const
{
  return makeGstSharedPtr(
    gst_element_get_clock(const_cast<GstElement*>(getRawGstElement())),
    TransferType::full
  );
}

GstClockTime Element::getStartTime() const
{
  return gst_element_get_start_time(const_cast<GstElement*>(getRawGstElement()));
}

GstState Element::getState(GstClockTime timeout) const
{
  const StateQuery query = queryState(timeout);
  if (query.result == GST_STATE_CHANGE_FAILURE)
  {
    throw std::runtime_error("Last state change of element " + getName() + " failed");
  }
  return query.current;
}

Element::StateQuery Element::queryState(GstClockTime timeout) const
{
  StateQuery query{GST_STATE_CHANGE_FAILURE, GST_STATE_VOID_PENDING, GST_STATE_VOID_PENDING};
  query.result = gst_element_get_state(
    const_cast<GstElement*>(getRawGstElement()),
    &query.current,
    &query.pending,
    timeout
  );
  return query;
}

std::optional<std::int64_t> Element::queryPosition(GstFormat format) const
{
  gint64 position{-1};
  const gboolean answered{
    gst_element_query_position(const_cast<GstElement*>(getRawGstElement()), format, &position)
  };
  return toQueryResult(answered, position);
}

std::optional<std::int64_t> Element::queryDuration(GstFormat format) const
{
  gint64 duration{-1};
  const gboolean answered{
    gst_element_query_duration(const_cast<GstElement*>(getRawGstElement()), format, &duration)
  };
  return toQueryResult(answered, duration);
}

std::optional<Element::SeekRange> Element::querySeeking(GstFormat format) const
{
  // gst_query_new_seeking: transfer full
  const std::unique_ptr<GstQuery, decltype(&gst_query_unref)> query{
    gst_query_new_seeking(format),
    &gst_query_unref
  };
  if (!gst_element_query(const_cast<GstElement*>(getRawGstElement()), query.get()))
  {
    return std::nullopt;
  }

  gboolean seekable{FALSE};
  gint64 start{-1};
  gint64 end{-1};
  gst_query_parse_seeking(query.get(), nullptr, &seekable, &start, &end);
  return SeekRange{seekable != FALSE, start, end};
}

void Element::seek(std::int64_t position, GstSeekFlags flags, GstFormat format)
{
  if (!gst_element_seek_simple(getRawGstElement(), format, flags, position))
  {
    throw std::runtime_error(
      "Failed to seek element " + getName() + " to position " + std::to_string(position)
    );
  }
}

void Element::sendEvent(const GstEventSPtr& event)
{
  if (!event)
  {
    throw std::invalid_argument("sendEvent: event must not be nullptr");
  }
  if (GST_EVENT_TYPE(event.get()) == GST_EVENT_EOS)
  {
    const StateQuery state{queryState()};
    if (endOfStreamWouldDeadlock(*getRawGstElement(), state))
    {
      throw std::logic_error(
        "Sending end-of-stream to element " + getName() + " would deadlock (state "
        + gst_element_state_get_name(state.current) + ", pending "
        + gst_element_state_get_name(state.pending) + ")"
      );
    }
  }
  // gst_element_send_event takes ownership (transfer full), the caller keeps its reference
  if (!gst_element_send_event(getRawGstElement(), gst_event_ref(event.get())))
  {
    throw std::runtime_error(
      std::string("Element ") + getName() + " did not handle the event "
      + GST_EVENT_TYPE_NAME(event.get())
    );
  }
}

void Element::sendEndOfStream()
{
  sendEvent(makeGstSharedPtr(gst_event_new_eos(), TransferType::full));
}

void Element::unlink(Element& other)
{
  gst_element_unlink(getRawGstElement(), other.getRawGstElement());
}

void Element::syncStateWithParent()
{
  const auto success = gst_element_sync_state_with_parent(getRawGstElement());
  if (!success)
  {
    throw std::runtime_error("Failed to sync Element " + getName() + " with parent");
  }
}

SignalConnection Element::connectSignalNoMorePads(std::function<void()> callback) const
{
  return connectGobjectSignal<>("no-more-pads", std::move(callback));
}

SignalConnection Element::connectSignalPadAdded(std::function<void(GstPadSPtr)> callback) const
{
  return connectGobjectSignal<GstPadSPtr>("pad-added", std::move(callback));
}

SignalConnection Element::connectSignalPadRemoved(std::function<void(GstPadSPtr)> callback) const
{
  return connectGobjectSignal<GstPadSPtr>("pad-removed", std::move(callback));
}

const GstElement* Element::getRawGstElement() const
{
  return GST_ELEMENT_CAST(getRawGstObject());
}

GstElement* Element::getRawGstElement()
{
  return GST_ELEMENT_CAST(getRawGstObject());
}

} // namespace dh::gst
