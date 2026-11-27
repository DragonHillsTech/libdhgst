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
#include "object.hpp"
#include "sharedptrs.hpp"
#include "signalconnection.hpp"
#include "transfertype.hpp"

#include <cassert>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace dh::gst
{

namespace
{

/**
 * @brief Enables sync message emission on a bus for its lifetime.
 * It is owned by the signal handler stored in the bus, so it only holds a weak reference:
 * a strong one would keep the bus alive as long as the handler is connected.
 */
class SyncMessageEmission
{
public:
  explicit SyncMessageEmission(GstBus& bus)
  {
    g_weak_ref_init(&weakBus, &bus);
    gst_bus_enable_sync_message_emission(&bus);
  }

  ~SyncMessageEmission()
  {
    // the bus is gone if the handler is destroyed while the bus is finalized
    if (auto* bus = static_cast<GstBus*>(g_weak_ref_get(&weakBus)))
    {
      gst_bus_disable_sync_message_emission(bus);
      gst_object_unref(bus);
    }
    g_weak_ref_clear(&weakBus);
  }

  SyncMessageEmission(const SyncMessageEmission&) = delete;
  SyncMessageEmission(SyncMessageEmission&&) = delete;
  SyncMessageEmission& operator=(const SyncMessageEmission&) = delete;
  SyncMessageEmission& operator=(SyncMessageEmission&&) = delete;

private:
  GWeakRef weakBus{};
};

/**
 * @brief Shared by a bus watch (as its user data) and the SignalConnection that removes it.
 */
struct WatchState
{
  std::function<bool(GstMessageSPtr)> callback;
  std::mutex mutex;
  // no reference: the bus accepts a new watch only after the old GSource is finalized
  GSource* source{nullptr};
  bool destroyed{false};
};

using WatchStateSPtr = std::shared_ptr<WatchState>;

gboolean onWatchMessage(GstBus* bus, GstMessage* message, gpointer userData)
{
  const auto& state{*static_cast<WatchStateSPtr*>(userData)};
  // We are called from C code (the main loop). Exceptions must not unwind through it.
  try
  {
    // GstBusFunc: message is transfer:none
    return state->callback(makeGstSharedPtr(message, TransferType::none)) ? G_SOURCE_CONTINUE
                                                                          : G_SOURCE_REMOVE;
  }
  catch (const std::exception& e)
  {
    GST_ERROR_OBJECT(bus, "Exception in bus watch callback: %s", e.what());
  }
  catch (...)
  {
    GST_ERROR_OBJECT(bus, "Unknown exception in bus watch callback");
  }
  return G_SOURCE_CONTINUE;
}

// called when the watch is removed, by the main loop or by the SignalConnection
void onWatchDestroyed(gpointer userData)
{
  const std::unique_ptr<WatchStateSPtr> owned{static_cast<WatchStateSPtr*>(userData)};
  const std::scoped_lock lock{(*owned)->mutex};
  (*owned)->source = nullptr;
  (*owned)->destroyed = true;
}

void removeWatch(WatchState& state)
{
  GSource* source{nullptr};
  {
    const std::scoped_lock lock{state.mutex};
    if (state.source)
    {
      // keeps the source valid until g_source_destroy, even if the main loop removes it meanwhile
      source = g_source_ref(state.source);
    }
  }
  if (source)
  {
    // calls onWatchDestroyed, so the mutex must not be held
    g_source_destroy(source);
    g_source_unref(source);
  }
}

} // namespace

Bus::Bus(const GstBusSPtr& gstBus)
  : Object(
      GST_OBJECT_CAST(gstBus.get()),
      TransferType::none
    ) // no pointer_cast because C inheritance
{
}

Bus::Bus(GstBus* gstBus, TransferType transferType)
  : Object(GST_OBJECT_CAST(gstBus), transferType)
{
}

std::shared_ptr<Bus> Bus::create(const GstBusSPtr& gstBus)
{
  std::shared_ptr<Bus> bus(new Bus(gstBus));
  bus->requireType<GstBus>();
  return bus;
}

std::shared_ptr<Bus> Bus::create(GstBus* gstBus, TransferType transferType)
{
  std::shared_ptr<Bus> bus(new Bus(gstBus, transferType));
  bus->requireType<GstBus>();
  return bus;
}

GstBusSPtr Bus::getGstBus() const
{
  // we can not use *pointer_cast because these are C types
  return makeGstSharedPtr(GST_BUS_CAST(getGstObject().get()), TransferType::none);
}

void Bus::post(GstMessage& message)
{
  // [transfer: full]) for the GstMessage in gst_bus_post.
  // --> ref before forwarding
  gst_bus_post(getRawGstBus(), gst_message_ref(&message));
}

GstMessageSPtr Bus::pop(GstMessageType types)
{
  // gst_bus_pop_filtered: transfer:full
  return makeGstSharedPtr(gst_bus_pop_filtered(getRawGstBus(), types), TransferType::full);
}

GstMessageSPtr Bus::timedPop(GstClockTime timeout, GstMessageType types)
{
  // gst_bus_timed_pop_filtered: transfer:full
  return makeGstSharedPtr(
    gst_bus_timed_pop_filtered(getRawGstBus(), timeout, types),
    TransferType::full
  );
}

SignalConnection
Bus::connectSignalNewSyncMessage(std::function<void(GstMessageSPtr)> callback) const
{
  auto emission = std::make_shared<SyncMessageEmission>(*getGstBus());
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

SignalConnection Bus::connectWatch(std::function<bool(GstMessageSPtr)> callback) const
{
  if (!callback)
  {
    throw std::invalid_argument("connectWatch: empty callback");
  }

  const auto state{std::make_shared<WatchState>()};
  state->callback = std::move(callback);
  // created before the watch is attached, so no allocation can fail after that
  SignalConnection connection{
    [state]()
    {
      removeWatch(*state);
    },
    [state]() -> bool
    {
      const std::scoped_lock lock{state->mutex};
      return state->source != nullptr;
    }
  };

  // owned by the watch, onWatchDestroyed frees it
  gpointer userData{std::make_unique<WatchStateSPtr>(state).release()};
  const guint sourceId{gst_bus_add_watch_full(
    getGstBus().get(),
    G_PRIORITY_DEFAULT,
    onWatchMessage,
    userData,
    onWatchDestroyed
  )};
  if (sourceId == 0)
  {
    // gst_bus_add_watch_full does not call the destroy notify on failure
    onWatchDestroyed(userData);
    throw std::runtime_error("Bus " + getName() + " already has a watch");
  }

  {
    const std::scoped_lock lock{state->mutex};
    // gst_bus_add_watch_full attaches to the thread-default main context. If another thread runs
    // that context, the watch may already be gone: then onWatchDestroyed has set destroyed.
    if (!state->destroyed)
    {
      state->source =
        g_main_context_find_source_by_id(g_main_context_get_thread_default(), sourceId);
    }
  }
  return connection;
}

GstBus* Bus::getRawGstBus()
{
  return GST_BUS_CAST(getRawGstObject());
}

const GstBus* Bus::getRawGstBus() const
{
  return GST_BUS_CAST(getRawGstObject());
}

} // namespace dh::gst
