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

#ifndef DH_GST_INTERNAL_SIGNAL_HPP
#define DH_GST_INTERNAL_SIGNAL_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace dh::gst
{

/**
 * @brief Lightweight multi-cast signal template for internal callback dispatching.
 *
 * Thread-safe: connect(), disconnect() and emission may be called concurrently from different threads.
 *
 * Disconnect guarantee: when disconnect() returns, the slot is not running and will not be called again.
 * If the slot is running in another thread, disconnect() waits until that call has finished.
 * This makes it safe to disconnect in the destructor of an object that the slot uses.
 * A slot may disconnect itself or other slots while it runs; a slot disconnected during an emission
 * is not called by that emission anymore.
 *
 * Because disconnect() may wait for a running slot, do not call it while holding a lock
 * that the slot needs, and do not disconnect each other's slots from two slots running
 * concurrently in different threads: both would deadlock.
 * A slot is never called concurrently with itself; concurrent emissions call it one after the other.
 */
template<typename... Args>
class Signal
{
public:
  using Slot = std::function<void(Args...)>;
  using Id = uint64_t;

  Id connect(Slot slot)
  {
    auto state = std::make_shared<SlotState>(std::move(slot));
    std::lock_guard<std::mutex> lock(mutex);
    const Id id = nextId++;
    slots.emplace_back(id, std::move(state));
    return id;
  }

  /**
   * @brief Check whether the slot with the given id is still connected.
   */
  [[nodiscard]] bool isConnected(Id id) const
  {
    std::lock_guard<std::mutex> lock(mutex);
    return std::any_of(slots.begin(), slots.end(), [id](const auto& pair) { return pair.first == id; });
  }

  void disconnect(Id id)
  {
    // destroy the removed slot outside the locks, its captures may run arbitrary code
    std::vector<std::pair<Id, SharedSlotState>> removed;
    {
      std::lock_guard<std::mutex> lock(mutex);
      const auto it = std::stable_partition(
        slots.begin(),
        slots.end(),
        [id](const auto& pair) { return pair.first != id; }
      );
      removed.assign(std::make_move_iterator(it), std::make_move_iterator(slots.end()));
      slots.erase(it, slots.end());
    }

    for(const auto& [removedId, state] : removed)
    {
      // waits for a running call in another thread; recursive, so a slot can disconnect itself
      std::lock_guard<std::recursive_mutex> callLock(state->callMutex);
      state->connected = false;
    }
  }

  void operator()(Args... args) const
  {
    std::vector<std::pair<Id, SharedSlotState>> slotsCopy;
    {
      std::lock_guard<std::mutex> lock(mutex);
      slotsCopy = slots;
    }

    for(const auto& [id, state] : slotsCopy)
    {
      std::lock_guard<std::recursive_mutex> callLock(state->callMutex);
      if(state->connected && state->slot)
      {
        state->slot(args...);
      }
    }
  }

private:
  struct SlotState
  {
    explicit SlotState(Slot slot)
    : slot(std::move(slot))
    {
    }

    const Slot slot;
    std::recursive_mutex callMutex; // held while the slot runs
    bool connected{true}; // guarded by callMutex
  };
  using SharedSlotState = std::shared_ptr<SlotState>;

  mutable std::mutex mutex;
  Id nextId{1}; // guarded by mutex
  std::vector<std::pair<Id, SharedSlotState>> slots; // guarded by mutex
};

} // namespace dh::gst

#endif // DH_GST_INTERNAL_SIGNAL_HPP
