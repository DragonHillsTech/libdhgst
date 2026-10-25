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

#include "messageparser.hpp"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace dh::gst
{

class MessageParser::Private : public std::enable_shared_from_this<MessageParser::Private>
{
public:
  MessageParser::AsyncHandler asyncHandler;

  template<typename... Args>
  using Callback = std::shared_ptr<const std::function<void(Args...)>>;

  template<typename... Args>
  using WeakCallback = std::weak_ptr<const std::function<void(Args...)>>;

  /**
   * Guards all callbacks and is held while a callback runs. So disconnecting waits for a callback
   * that is running in another thread, and no callback is called after disconnect() returned.
   * It is recursive, so a callback may disconnect itself or connect and disconnect other callbacks.
   */
  std::recursive_mutex callbackMutex;

  // one callback per message type, nullptr if none is connected (guarded by callbackMutex)
  Callback<const std::string&> endOfStreamCallback;
  Callback<const std::string&, const std::string&, const std::string&> errorCallback;
  Callback<const std::string&, GstState, GstState, GstState> stateChangedCallback;
  Callback<const std::string&, const std::string&, const std::string&> warningCallback;
  Callback<const std::string&> durationChangedCallback;
  Callback<const std::string&, const std::string&, const std::string&> infoCallback;
  Callback<const std::string&, GstStreamStatusType, const std::string&> streamStatusCallback;
  Callback<const std::string&> streamStartCallback;
  Callback<const std::string&, const GstStructure*> elementMessageCallback;
  Callback<const std::string&, GstClockTime> asyncDoneCallback;

  template<typename... Args>
  SignalConnection connect(
    Callback<Args...> Private::* callbackSlot,
    std::function<void(Args...)> callbackFunction,
    std::string_view messageType
  )
  {
    if(!callbackFunction)
    {
      throw std::invalid_argument("empty signal callback");
    }

    const auto callback = std::make_shared<const std::function<void(Args...)>>(std::move(callbackFunction));
    storeCallback(callbackSlot, callback, messageType);

    // identify this connection by its callback, the slot may be connected again after a disconnect
    const std::weak_ptr<Private> weakSelf = weak_from_this();
    const WeakCallback<Args...> weakCallback = callback;

    auto disconnectHandler = [weakSelf, callbackSlot, weakCallback]()
    {
      if(const auto self = weakSelf.lock())
      {
        self->disconnect(callbackSlot, weakCallback);
      }
    };
    auto isConnectedHandler = [weakSelf, callbackSlot, weakCallback]()
    {
      const auto self = weakSelf.lock();
      return self && self->isConnected(callbackSlot, weakCallback);
    };
    return SignalConnection(std::move(disconnectHandler), std::move(isConnectedHandler));
  }

  /// store the callback in its slot, only one callback per message type is allowed
  template<typename... Args>
  void storeCallback(Callback<Args...> Private::* callbackSlot, const Callback<Args...>& callback, std::string_view messageType)
  {
    std::lock_guard<std::recursive_mutex> lock(callbackMutex);
    auto& slot = this->*callbackSlot;
    if(slot)
    {
      throw std::logic_error("MessageParser: a callback for " + std::string(messageType) + " is already connected");
    }
    slot = callback;
  }

  /// disconnect the callback, if it is still the one connected to the slot
  template<typename... Args>
  void disconnect(Callback<Args...> Private::* callbackSlot, const WeakCallback<Args...>& callback)
  {
    // destroyed after unlocking: the captures of the callback may run arbitrary code
    Callback<Args...> removed;
    std::lock_guard<std::recursive_mutex> lock(callbackMutex);
    auto& slot = this->*callbackSlot;
    if(isCurrentCallback(slot, callback))
    {
      removed = std::move(slot);
      slot = nullptr;
    }
  }

  /// check whether the callback is still the one connected to the slot
  template<typename... Args>
  bool isConnected(Callback<Args...> Private::* callbackSlot, const WeakCallback<Args...>& callback)
  {
    std::lock_guard<std::recursive_mutex> lock(callbackMutex);
    return isCurrentCallback(this->*callbackSlot, callback);
  }

  /// true if the slot holds exactly this callback (an empty slot never matches)
  template<typename... Args>
  static bool isCurrentCallback(const Callback<Args...>& slot, const WeakCallback<Args...>& callback)
  {
    return slot && slot == callback.lock();
  }

  /// call the callback of a message type, if one is connected
  template<typename... Args, typename... CallArgs>
  void call(Callback<Args...> Private::* callbackSlot, CallArgs&&... args)
  {
    std::lock_guard<std::recursive_mutex> lock(callbackMutex);
    // a copy keeps the callback alive if it disconnects itself
    const auto callback = this->*callbackSlot;
    if(callback)
    {
      (*callback)(std::forward<CallArgs>(args)...);
    }
  }
};

MessageParser::MessageParser()
: prv{std::make_shared<Private>()}
{
}

MessageParser::~MessageParser() = default;

SignalConnection MessageParser::connectSignalEndOfStream(std::function<void(const std::string& sourceName)> callback)
{
  return prv->connect(&Private::endOfStreamCallback, std::move(callback), "EOS");
}

SignalConnection MessageParser::connectSignalError(std::function<void(const std::string& sourceName, const std::string& errorMessage, const std::string& debugInfo)> callback)
{
  return prv->connect(&Private::errorCallback, std::move(callback), "ERROR");
}

SignalConnection MessageParser::connectSignalStateChanged(std::function<void(const std::string& sourceName, GstState oldState, GstState newState, GstState pendingState)> callback)
{
  return prv->connect(&Private::stateChangedCallback, std::move(callback), "STATE_CHANGED");
}

SignalConnection MessageParser::connectSignalWarning(std::function<void(const std::string& sourceName, const std::string& warningMessage, const std::string& debugInfo)> callback)
{
  return prv->connect(&Private::warningCallback, std::move(callback), "WARNING");
}

SignalConnection MessageParser::connectSignalDurationChanged(std::function<void(const std::string& sourceName)> callback)
{
  return prv->connect(&Private::durationChangedCallback, std::move(callback), "DURATION_CHANGED");
}

SignalConnection MessageParser::connectSignalInfo(std::function<void(const std::string& sourceName, const std::string& infoMessage, const std::string& debugInfo)> callback)
{
  return prv->connect(&Private::infoCallback, std::move(callback), "INFO");
}

SignalConnection MessageParser::connectSignalStreamStatus(std::function<void(const std::string& sourceName, GstStreamStatusType statusType, const std::string& ownerName)> callback)
{
  return prv->connect(&Private::streamStatusCallback, std::move(callback), "STREAM_STATUS");
}

SignalConnection MessageParser::connectSignalStreamStart(std::function<void(const std::string& sourceName)> callback)
{
  return prv->connect(&Private::streamStartCallback, std::move(callback), "STREAM_START");
}

SignalConnection MessageParser::connectSignalElementMessage(std::function<void(const std::string& sourceName, const GstStructure* structure)> callback)
{
  return prv->connect(&Private::elementMessageCallback, std::move(callback), "ELEMENT");
}

SignalConnection MessageParser::connectSignalAsyncDone(std::function<void(const std::string& sourceName, GstClockTime runningTime)> callback)
{
  return prv->connect(&Private::asyncDoneCallback, std::move(callback), "ASYNC_DONE");
}

void MessageParser::parse(const GstMessage& message)
{
  if(prv->asyncHandler)
  {
    // keep the message alive until processed
    gst_message_ref(const_cast<GstMessage*>(&message));
    prv->asyncHandler(
      [this, &message]()
      {
        parseSync(message);
        gst_message_unref(const_cast<GstMessage*>(&message));
      }
    );
  }
  else
  {
    parseSync(message);
  }
}

void MessageParser::parseSync(const GstMessage& message)
{
  const std::string sourceName = getSourceName(message);
  auto* messagePtr = const_cast<GstMessage*>(&message);

  switch (GST_MESSAGE_TYPE(&message))
  {
    case GST_MESSAGE_EOS:
    {
      prv->call(&Private::endOfStreamCallback, sourceName);
      break;
    }

    case GST_MESSAGE_ERROR:
    {
      GError* error = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_error(messagePtr, &error, &debugInfo);
      prv->call(&Private::errorCallback, sourceName, std::string(error->message), std::string(debugInfo ? debugInfo : ""));
      g_clear_error(&error);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_WARNING:
    {
      GError* warning = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_warning(messagePtr, &warning, &debugInfo);
      prv->call(&Private::warningCallback, sourceName, std::string(warning->message), std::string(debugInfo ? debugInfo : ""));
      g_clear_error(&warning);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_INFO:
    {
      GError* info = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_info(messagePtr, &info, &debugInfo);
      prv->call(&Private::infoCallback, sourceName, std::string(info->message), std::string(debugInfo ? debugInfo : ""));
      g_clear_error(&info);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_STATE_CHANGED:
    {
      GstState oldState, newState, pendingState;
      gst_message_parse_state_changed(messagePtr, &oldState, &newState, &pendingState);
      prv->call(&Private::stateChangedCallback, sourceName, oldState, newState, pendingState);
      break;
    }

    case GST_MESSAGE_DURATION_CHANGED:
    {
      prv->call(&Private::durationChangedCallback, sourceName);
      break;
    }

    case GST_MESSAGE_STREAM_STATUS:
    {
      GstStreamStatusType statusType;
      GstElement* ownerElement;
      gst_message_parse_stream_status(messagePtr, &statusType, &ownerElement);
      std::string ownerName = ownerElement ? GST_OBJECT_NAME(ownerElement) : "unknown";
      prv->call(&Private::streamStatusCallback, sourceName, statusType, ownerName);
      break;
    }

    case GST_MESSAGE_STREAM_START:
    {
      prv->call(&Private::streamStartCallback, sourceName);
      break;
    }

    case GST_MESSAGE_ELEMENT:
    {
      const GstStructure* structure = gst_message_get_structure(messagePtr);
      prv->call(&Private::elementMessageCallback, sourceName, structure);
      break;
    }

    case GST_MESSAGE_ASYNC_DONE:
    {
      GstClockTime runningTime;
      gst_message_parse_async_done(messagePtr, &runningTime);
      prv->call(&Private::asyncDoneCallback, sourceName, runningTime);
      break;
    }

    // Add more message types as needed...

    default:
      GST_WARNING("MessageParser: Unhandled message type '%s'", GST_MESSAGE_TYPE_NAME(messagePtr));
      //throw std::runtime_error("Unsupported message type: " + std::string(GST_MESSAGE_TYPE_NAME(messagePtr)));
  }
}

std::shared_ptr<MessageParser> MessageParser::create()
{
  return std::shared_ptr<MessageParser>(new MessageParser());
}

std::shared_ptr<MessageParser> MessageParser::create(AsyncHandler handler)
{
  auto ret = std::shared_ptr<MessageParser>(new MessageParser());
  ret->prv->asyncHandler = std::move(handler);
  return ret;
}

std::string MessageParser::getSourceName(const GstMessage& message) const
{
  if(GST_MESSAGE_SRC(&message))
  {
    const gchar* name = GST_OBJECT_NAME(GST_MESSAGE_SRC(&message));
    return name ? name : "unknown";
  }
  return "unknown";
}


} // dh::gst
