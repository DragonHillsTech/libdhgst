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
 * Each emission works on a snapshot of the connected slots, taken when the emission starts.
 * Therefore a slot disconnected while an emission is running (in this or another thread)
 * may still be called once by that emission, also after disconnect() has returned.
 * Slots are called without holding the internal lock, so they may connect and disconnect slots themselves.
 */
template<typename... Args>
class Signal
{
public:
  using Slot = std::function<void(Args...)>;
  using Id = uint64_t;

  Id connect(Slot slot)
  {
    auto sharedSlot = std::make_shared<const Slot>(std::move(slot));
    std::lock_guard<std::mutex> lock(mutex);
    const Id id = nextId++;
    slots.emplace_back(id, std::move(sharedSlot));
    return id;
  }

  void disconnect(Id id)
  {
    // destroy the removed slot outside the lock, its captures may run arbitrary code
    std::vector<std::pair<Id, SharedSlot>> removed;
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
  }

  void operator()(Args... args) const
  {
    std::vector<std::pair<Id, SharedSlot>> slotsCopy;
    {
      std::lock_guard<std::mutex> lock(mutex);
      slotsCopy = slots;
    }

    for(const auto& [id, slot] : slotsCopy)
    {
      if(slot && *slot)
      {
        (*slot)(args...);
      }
    }
  }

private:
  using SharedSlot = std::shared_ptr<const Slot>;

  mutable std::mutex mutex;
  Id nextId{1}; // guarded by mutex
  std::vector<std::pair<Id, SharedSlot>> slots; // guarded by mutex
};

} // namespace dh::gst

#endif // DH_GST_INTERNAL_SIGNAL_HPP
