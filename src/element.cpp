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

// std
#include <stdexcept>
#include <string>
#include <utility>

// C
#include <unordered_set>

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
  pads.push_back(makeGstSharedPtr(pad, TransferType::None));
  return TRUE; // continue iteration
}

} // namespace

Element::Element(GstElement* gstElement, TransferType transferType)
: Object(GST_OBJECT_CAST(gstElement), transferType)
{
  requireType(GST_TYPE_ELEMENT);
}

std::shared_ptr<Element> Element::create(GstElementSPtr gstElement)
{
  return std::shared_ptr<Element>(new Element(gstElement));
}

std::shared_ptr<Element> Element::create(GstElement* gstElement, TransferType transferType)
{
  return std::shared_ptr<Element>(new Element(gstElement, transferType));
}

Element::Element(GstElementSPtr gstElement)
: Object(GST_OBJECT_CAST(gstElement.get()), TransferType::None) // no pointer_cast because C inheritance
{
  requireType(GST_TYPE_ELEMENT);
}

GstElementSPtr Element::getGstElement()
{
  // we can not use *pointer_cast because these are C types
  return makeGstSharedPtr(GST_ELEMENT_CAST(getGstObject().get()), TransferType::None);
}

const GstElementSPtr Element::getGstElement() const
{
  // we can not use *pointer_cast because these are C types
  return makeGstSharedPtr(GST_ELEMENT_CAST(getGstObject().get()), TransferType::None);
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
  if(! pad)
  {
    throw std::invalid_argument("getCompatiblePad: pad must not be nullptr");
  }
  // gst_element_get_compatible_pad: transfer full, nullable
  return makeGstSharedPtr(
    gst_element_get_compatible_pad(getRawGstElement(), pad.get(), caps.get()),
    TransferType::Full
  );
}

GstPadSPtr Element::getStaticPad(const std::string& name)
{
  // gst_element_get_static_pad: transfer full, nullable
  return makeGstSharedPtr(
    gst_element_get_static_pad(getRawGstElement(), name.c_str()),
    TransferType::Full
  );
}

std::shared_ptr<Element>& Element::link(std::shared_ptr<Element>& other)
{
  if (!gst_element_link(getRawGstElement(), other->getRawGstElement()))
  {
    std::string errorMessage = "Failed to link GstElements: ";
    errorMessage += getName() + " -> " + other->getName();

    // Throw an exception with the detailed error message
    throw std::runtime_error(errorMessage);
  }

  return other;
}

std::string Element::getFactoryName() const
{
  auto* gstElementFactoryRawPtr = gst_element_get_factory(
    const_cast<GstElement*>(getRawGstElement())
  );
  if(! gstElementFactoryRawPtr)
  {
    return {};
  }
  const auto factory = ElementFactory::create(gstElementFactoryRawPtr, TransferType::None);
  return factory->getName();
}

GstClockSPtr Element::getElementClock() const
{
  return makeGstSharedPtr(
    gst_element_get_clock(const_cast<GstElement*>(getRawGstElement())),
      TransferType::Full
    );
}

GstClockTime Element::getStartTime() const
{
  return gst_element_get_start_time(const_cast<GstElement*>(getRawGstElement()));
}

GstState Element::getState(GstClockTime timeout) const
{
  const StateQuery query = queryState(timeout);
  if(query.result == GST_STATE_CHANGE_FAILURE)
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

void Element::unlink(std::shared_ptr<Element>& other)
{
  gst_element_unlink(getRawGstElement(), other->getRawGstElement());
}

void Element::syncStateWithParent()
{
  const auto success = gst_element_sync_state_with_parent(getRawGstElement());
  if(! success)
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


} // dh::gst
