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

#ifndef DH_GST_INTERNAL_CALLBACKSLOT_HPP
#define DH_GST_INTERNAL_CALLBACKSLOT_HPP

#include "signalconnection.hpp"

#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace dh::gst
{

/**
 * @brief Holds at most one callback and calls it, thread-safe.
 *
 * connect() returns a SignalConnection that disconnects the callback when it is destroyed.
 * Only one callback can be connected at once: connect() throws std::logic_error until the
 * connected callback is disconnected. To replace a callback, disconnect the old connection before
 * connecting the new one; `connection = slot->connect(newFunction);` throws, because the new
 * callback is connected before the assignment releases the old connection.
 *
 * Thread safety: call(), connect() and disconnecting may be called from different threads.
 * - When SignalConnection::disconnect() returns, the callback is not running and will not be
 *   called again; if it runs in another thread, disconnect() waits for it.
 * - The callback is never called concurrently with itself: concurrent call()s run it one after the other.
 * - connect() and SignalConnection::connected() never wait for a running callback.
 * - The callback may disconnect itself while it runs. Its captures are destroyed after all
 *   locks are released, so their destructors may use this slot again.
 * Because disconnecting may wait for the running callback, do not disconnect while holding a lock
 * that the callback needs, and do not let two concurrently running callbacks disconnect each other.
 *
 * Exceptions thrown by the callback propagate out of call().
 *
 * Connections keep a weak reference to the slot, so a slot is always owned by a std::shared_ptr,
 * see create(). After the slot is destroyed, its connections report connected() == false.
 */
template<typename... Args>
class CallbackSlot : public std::enable_shared_from_this<CallbackSlot<Args...>>
{
public:
  using Function = std::function<void(Args...)>;

  /**
   * @param name used in error messages, e.g. the message type the callback is for
   */
  [[nodiscard]] static std::shared_ptr<CallbackSlot> create(std::string name)
  {
    return std::shared_ptr<CallbackSlot>(new CallbackSlot(std::move(name)));
  }

  CallbackSlot(const CallbackSlot&) = delete;
  CallbackSlot& operator=(const CallbackSlot&) = delete;

  /**
   * @brief Connect the callback.
   * @throws std::invalid_argument if the function is empty
   * @throws std::logic_error if a callback is already connected
   */
  [[nodiscard]] SignalConnection connect(Function function)
  {
    if(!function)
    {
      throw std::invalid_argument("empty signal callback");
    }

    const auto callback = std::make_shared<const Function>(std::move(function));
    store(callback);

    // identify this connection by its callback, the slot may be connected again after a disconnect
    const std::weak_ptr<CallbackSlot> weakSelf = this->weak_from_this();
    const WeakCallback weakCallback = callback;

    auto disconnectHandler = [weakSelf, weakCallback]()
    {
      if(const auto self = weakSelf.lock())
      {
        self->disconnect(weakCallback);
      }
    };
    auto isConnectedHandler = [weakSelf, weakCallback]()
    {
      const auto self = weakSelf.lock();
      return self && self->isConnected(weakCallback);
    };
    return SignalConnection(std::move(disconnectHandler), std::move(isConnectedHandler));
  }

  /**
   * @brief Call the callback, if one is connected.
   */
  template<typename... CallArgs>
  void call(CallArgs&&... args)
  {
    // a copy keeps the callback alive if it disconnects itself. Declared before the locks,
    // so it is destroyed after unlocking: its captures may run arbitrary code
    Callback callback;
    std::lock_guard<std::recursive_mutex> callLock(callMutex);
    {
      std::lock_guard<std::mutex> lock(callbackMutex);
      callback = connectedCallback;
    }
    if(callback)
    {
      (*callback)(std::forward<CallArgs>(args)...);
    }
  }

private:
  using Callback = std::shared_ptr<const Function>;
  using WeakCallback = std::weak_ptr<const Function>;

  explicit CallbackSlot(std::string name)
  : name(std::move(name))
  {
  }

  /// store the callback, only one callback can be connected at once
  void store(const Callback& callback)
  {
    std::lock_guard<std::mutex> lock(callbackMutex);
    if(connectedCallback)
    {
      throw std::logic_error("a callback for " + name + " is already connected");
    }
    connectedCallback = callback;
  }

  /// disconnect the callback, if it is still the connected one
  void disconnect(const WeakCallback& callback)
  {
    // declared before the locks, so it is destroyed after unlocking: its captures may run arbitrary code
    Callback removed;
    std::lock_guard<std::recursive_mutex> callLock(callMutex); // waits for a running call
    std::lock_guard<std::mutex> lock(callbackMutex);
    if(isConnectedCallback(callback))
    {
      removed = std::exchange(connectedCallback, nullptr);
    }
  }

  /// check whether the callback is still the connected one
  [[nodiscard]] bool isConnected(const WeakCallback& callback) const
  {
    std::lock_guard<std::mutex> lock(callbackMutex);
    return isConnectedCallback(callback);
  }

  /// true if exactly this callback is connected (an empty slot never matches); callbackMutex must be held
  [[nodiscard]] bool isConnectedCallback(const WeakCallback& callback) const
  {
    return connectedCallback && connectedCallback == callback.lock();
  }

  const std::string name;

  /**
   * Held while the callback runs. disconnect() takes it, so it waits for a running call
   * and no call starts after it returned. Recursive, so the callback may disconnect itself.
   * Lock order: callMutex before callbackMutex.
   */
  std::recursive_mutex callMutex;

  /// guards connectedCallback; only held briefly, never while the callback runs
  mutable std::mutex callbackMutex;

  /// nullptr if no callback is connected
  Callback connectedCallback;
};

} // namespace dh::gst

#endif // DH_GST_INTERNAL_CALLBACKSLOT_HPP
