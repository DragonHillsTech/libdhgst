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

#include "bus.hpp"

namespace dh ::gst
{


Bus::Bus(GstBusSPtr gstBus)
: Object(GST_OBJECT_CAST(gstBus.get()), TransferType::None) // no pointer_cast because C inheritance
{
}

Bus::Bus(GstBus* gstBus, TransferType transferType)
: Object(GST_OBJECT_CAST(gstBus),  transferType)
{
}

std::shared_ptr<Bus> Bus::create(GstBusSPtr gstBus)
{
 return std::shared_ptr<Bus>(new Bus(gstBus));
}

std::shared_ptr<Bus> Bus::create(GstBus* gstBus, TransferType transferType)
{
 return std::shared_ptr<Bus>(new Bus(gstBus, transferType));
}

GstBusSPtr Bus::getGstBus()
{
  // we can not use *pointer_cast because these are C types
  return makeGstSharedPtr(GST_BUS_CAST(getGstObject().get()), TransferType::None);
}

const GstBusSPtr Bus::getGstBus() const
{
 // we can not use *pointer_cast because these are C types
 return makeGstSharedPtr(GST_BUS_CAST(getGstObject().get()), TransferType::None);
}

void Bus::post(const GstMessageSPtr message)
{
  assert(message);
  // [transfer: full]) for the GstMessage in gst_bus_post.
  // --> ref before forwarding
  gst_bus_post(getRawGstBus(), gst_message_ref(message.get()));
}

bs2::signal<void(GstMessageSPtr)>& Bus::newSyncMessageSignal() const
{
  //TODO: on disconnect, gst_bus_enable_sync_message_emission should be called as often as it was enabled to stop sync signal emission
  gst_bus_enable_sync_message_emission(const_cast<GstBus*>(getRawGstBus()));
  return connectGobjectSignal<GstMessageSPtr>("sync-message");
}

GstBus* Bus::getRawGstBus()
{
  return GST_BUS_CAST(getRawGstObject());
}

const GstBus* Bus::getRawGstBus() const
{
  return GST_BUS_CAST(getRawGstObject());
}


} // dh::gst