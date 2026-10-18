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

#include "bus.hpp"

#include <cassert>
#include <memory>
#include <utility>

namespace dh::gst
{

namespace
{

class SyncMessageEmission
{
public:
  explicit SyncMessageEmission(GstBusSPtr bus)
  : bus(std::move(bus))
  {
    assert(this->bus != nullptr);
    gst_bus_enable_sync_message_emission(this->bus.get());
  }

  ~SyncMessageEmission()
  {
    gst_bus_disable_sync_message_emission(bus.get());
  }

  SyncMessageEmission(const SyncMessageEmission&) = delete;
  SyncMessageEmission& operator=(const SyncMessageEmission&) = delete;

private:
  GstBusSPtr bus;
};

} // namespace


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

SignalConnection Bus::connectSignalNewSyncMessage(std::function<void(GstMessageSPtr)> callback) const
{
  auto emission = std::make_shared<SyncMessageEmission>(getGstBus());
  return connectGobjectSignal<GstMessageSPtr>(
    "sync-message",
    std::function<void(GstMessageSPtr)>(
      [callback = std::move(callback), emission = std::move(emission)](GstMessageSPtr message)
      {
        (void)emission;
        callback(std::move(message));
      }
    )
  );
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
