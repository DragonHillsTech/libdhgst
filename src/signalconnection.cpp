/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
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

#include "signalconnection.hpp"

#include <memory>
#include <utility>

namespace dh::gst
{

namespace
{

struct GObjectDeleter
{
  void operator()(GObject* obj) const noexcept
  {
    if(obj)
    {
      g_object_unref(obj);
    }
  }
};

using GObjectUniquePtr = std::unique_ptr<GObject, GObjectDeleter>;

} // namespace

SignalConnection::SignalConnection(GObject* object, gulong handlerId)
: handlerId(handlerId)
{
  if(object)
  {
    g_weak_ref_init(&weakObject, object);
    weakRefInitialized = true;
  }
}

SignalConnection::SignalConnection(
  std::function<void()> disconnectHandler,
  std::function<bool()> isConnectedHandler
)
: customDisconnectHandler(std::move(disconnectHandler))
, customIsConnectedHandler(std::move(isConnectedHandler))
{
}

SignalConnection::SignalConnection(SignalConnection&& other) noexcept
{
  moveFrom(std::move(other));
}

SignalConnection& SignalConnection::operator=(SignalConnection&& other) noexcept
{
  if(this != &other)
  {
    disconnect();
    clearWeakRef();
    moveFrom(std::move(other));
  }
  return *this;
}

SignalConnection::~SignalConnection()
{
  disconnect();
  clearWeakRef();
}

void SignalConnection::disconnect()
{
  if(customDisconnectHandler)
  {
    auto handler = std::move(customDisconnectHandler);
    customDisconnectHandler = nullptr;
    customIsConnectedHandler = nullptr;
    handler();
    return;
  }

  if(handlerId == 0 || !weakRefInitialized)
  {
    handlerId = 0;
    return;
  }

  GObjectUniquePtr object(static_cast<GObject*>(g_weak_ref_get(&weakObject)));
  if(object)
  {
    if(g_signal_handler_is_connected(object.get(), handlerId))
    {
      g_signal_handler_disconnect(object.get(), handlerId);
    }
  }
  handlerId = 0;
}

bool SignalConnection::connected() const
{
  if(customDisconnectHandler)
  {
    return customIsConnectedHandler ? customIsConnectedHandler() : true;
  }

  if(handlerId == 0 || !weakRefInitialized)
  {
    return false;
  }

  GObjectUniquePtr object(static_cast<GObject*>(g_weak_ref_get(const_cast<GWeakRef*>(&weakObject))));
  if(!object)
  {
    return false;
  }

  return g_signal_handler_is_connected(object.get(), handlerId);
}

SignalConnection::operator bool() const
{
  return connected();
}

void SignalConnection::clearWeakRef()
{
  if(weakRefInitialized)
  {
    g_weak_ref_clear(&weakObject);
    weakRefInitialized = false;
  }
}

void SignalConnection::moveFrom(SignalConnection&& other) noexcept
{
  handlerId = other.handlerId;
  customDisconnectHandler = std::move(other.customDisconnectHandler);
  other.customDisconnectHandler = nullptr;
  customIsConnectedHandler = std::move(other.customIsConnectedHandler);
  other.customIsConnectedHandler = nullptr;

  if(other.weakRefInitialized)
  {
    GObjectUniquePtr object(static_cast<GObject*>(g_weak_ref_get(&other.weakObject)));
    if(object)
    {
      g_weak_ref_init(&weakObject, object.get());
      weakRefInitialized = true;
    }
    other.clearWeakRef();
  }

  other.handlerId = 0;
}

} // namespace dh::gst
