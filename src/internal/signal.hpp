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
#include <utility>
#include <vector>

namespace dh::gst
{

/**
 * @brief Lightweight multi-cast signal template for internal callback dispatching.
 */
template<typename... Args>
class Signal
{
public:
  using Slot = std::function<void(Args...)>;
  using Id = uint64_t;

  Id connect(Slot slot)
  {
    const Id id = nextId++;
    slots.emplace_back(id, std::move(slot));
    return id;
  }

  void disconnect(Id id)
  {
    slots.erase(
      std::remove_if(slots.begin(), slots.end(), [id](const auto& pair) { return pair.first == id; }),
      slots.end()
    );
  }

  void operator()(Args... args) const
  {
    auto slotsCopy = slots;
    for(const auto& [id, slot] : slotsCopy)
    {
      if(slot)
      {
        slot(args...);
      }
    }
  }

private:
  Id nextId{1};
  std::vector<std::pair<Id, Slot>> slots;
};

} // namespace dh::gst

#endif // DH_GST_INTERNAL_SIGNAL_HPP
