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
#include "internal/signal.hpp"
#include "object.hpp"
#include "sharedptrs.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace dh::gst
{

namespace
{

/**
 * @brief Name of a GstObject, or "unknown" if there is no object or it has no name.
 * Object::getName() takes the object lock: the object may be renamed by another thread.
 */
std::string getObjectNameOrUnknown(GstObject* object)
{
  if(! object)
  {
    return "unknown";
  }
  const std::string name = Object::create(object, TransferType::None)->getName();
  return name.empty() ? "unknown" : name;
}

} // namespace

class MessageParser::Private
{
public:
  MessageParser::AsyncHandler asyncHandler;

  Signal<const std::string&> endOfStreamSignal;
  Signal<const std::string&, const std::string&, const std::string&> errorSignal;
  Signal<const std::string&, GstState, GstState, GstState> stateChangedSignal;
  Signal<const std::string&, const std::string&, const std::string&> warningSignal;
  Signal<const std::string&> durationChangedSignal;
  Signal<const std::string&, const std::string&, const std::string&> infoSignal;
  Signal<const std::string&, GstStreamStatusType, const std::string&> streamStatusSignal;
  Signal<const std::string&> streamStartSignal;
  Signal<const std::string&, const GstStructure*> elementMessageSignal;
  Signal<const std::string&, GstClockTime> asyncDoneSignal;

  template<typename SignalType, typename CallbackType>
  SignalConnection connect(std::shared_ptr<Private> self, SignalType Private::* signalMember, CallbackType callback)
  {
    if(!callback)
    {
      throw std::invalid_argument("empty signal callback");
    }

    auto& signal = (*this).*signalMember;
    const auto id = signal.connect(std::move(callback));

    std::weak_ptr<Private> weakSelf = self;
    return SignalConnection(
      [weakSelf, signalMember, id]()
      {
        if(auto locked = weakSelf.lock())
        {
          ((*locked).*signalMember).disconnect(id);
        }
      },
      [weakSelf, signalMember, id]()
      {
        const auto locked = weakSelf.lock();
        return locked && ((*locked).*signalMember).isConnected(id);
      }
    );
  }
};

MessageParser::MessageParser()
: prv{std::make_shared<Private>()}
{
}

MessageParser::~MessageParser() = default;

SignalConnection MessageParser::connectSignalEndOfStream(std::function<void(const std::string& sourceName)> callback)
{
  return prv->connect(prv, &Private::endOfStreamSignal, std::move(callback));
}

SignalConnection MessageParser::connectSignalError(std::function<void(const std::string& sourceName, const std::string& errorMessage, const std::string& debugInfo)> callback)
{
  return prv->connect(prv, &Private::errorSignal, std::move(callback));
}

SignalConnection MessageParser::connectSignalStateChanged(std::function<void(const std::string& sourceName, GstState oldState, GstState newState, GstState pendingState)> callback)
{
  return prv->connect(prv, &Private::stateChangedSignal, std::move(callback));
}

SignalConnection MessageParser::connectSignalWarning(std::function<void(const std::string& sourceName, const std::string& warningMessage, const std::string& debugInfo)> callback)
{
  return prv->connect(prv, &Private::warningSignal, std::move(callback));
}

SignalConnection MessageParser::connectSignalDurationChanged(std::function<void(const std::string& sourceName)> callback)
{
  return prv->connect(prv, &Private::durationChangedSignal, std::move(callback));
}

SignalConnection MessageParser::connectSignalInfo(std::function<void(const std::string& sourceName, const std::string& infoMessage, const std::string& debugInfo)> callback)
{
  return prv->connect(prv, &Private::infoSignal, std::move(callback));
}

SignalConnection MessageParser::connectSignalStreamStatus(std::function<void(const std::string& sourceName, GstStreamStatusType statusType, const std::string& ownerName)> callback)
{
  return prv->connect(prv, &Private::streamStatusSignal, std::move(callback));
}

SignalConnection MessageParser::connectSignalStreamStart(std::function<void(const std::string& sourceName)> callback)
{
  return prv->connect(prv, &Private::streamStartSignal, std::move(callback));
}

SignalConnection MessageParser::connectSignalElementMessage(std::function<void(const std::string& sourceName, const GstStructure* structure)> callback)
{
  return prv->connect(prv, &Private::elementMessageSignal, std::move(callback));
}

SignalConnection MessageParser::connectSignalAsyncDone(std::function<void(const std::string& sourceName, GstClockTime runningTime)> callback)
{
  return prv->connect(prv, &Private::asyncDoneSignal, std::move(callback));
}

void MessageParser::parse(const GstMessage& message)
{
  if(prv->asyncHandler)
  {
    // The task may run after this parser is destroyed, or never: keep the message alive
    // only as long as the task exists and do not touch the parser if it is already gone.
    auto messageSPtr = makeGstSharedPtr(const_cast<GstMessage*>(&message), TransferType::None);
    std::weak_ptr<Private> weakPrv = prv;
    prv->asyncHandler(
      [weakPrv, messageSPtr = std::move(messageSPtr)]()
      {
        if(auto lockedPrv = weakPrv.lock())
        {
          parseSync(*lockedPrv, *messageSPtr);
        }
      }
    );
  }
  else
  {
    parseSync(*prv, message);
  }
}

void MessageParser::parseSync(Private& prv, const GstMessage& message)
{
  const std::string sourceName = getSourceName(message);
  auto* messagePtr = const_cast<GstMessage*>(&message);

  switch (GST_MESSAGE_TYPE(&message))
  {
    case GST_MESSAGE_EOS:
    {
      prv.endOfStreamSignal(sourceName);
      break;
    }

    case GST_MESSAGE_ERROR:
    {
      GError* error = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_error(messagePtr, &error, &debugInfo);
      prv.errorSignal(sourceName, error->message, debugInfo ? debugInfo : "");
      g_clear_error(&error);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_WARNING:
    {
      GError* warning = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_warning(messagePtr, &warning, &debugInfo);
      prv.warningSignal(sourceName, warning->message, debugInfo ? debugInfo : "");
      g_clear_error(&warning);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_INFO:
    {
      GError* info = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_info(messagePtr, &info, &debugInfo);
      prv.infoSignal(sourceName, info->message, debugInfo ? debugInfo : "");
      g_clear_error(&info);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_STATE_CHANGED:
    {
      GstState oldState, newState, pendingState;
      gst_message_parse_state_changed(messagePtr, &oldState, &newState, &pendingState);
      prv.stateChangedSignal(sourceName, oldState, newState, pendingState);
      break;
    }

    case GST_MESSAGE_DURATION_CHANGED:
    {
      prv.durationChangedSignal(sourceName);
      break;
    }

    case GST_MESSAGE_STREAM_STATUS:
    {
      GstStreamStatusType statusType;
      GstElement* ownerElement;
      gst_message_parse_stream_status(messagePtr, &statusType, &ownerElement);
      const std::string ownerName = getObjectNameOrUnknown(GST_OBJECT_CAST(ownerElement));
      prv.streamStatusSignal(sourceName, statusType, ownerName);
      break;
    }

    case GST_MESSAGE_STREAM_START:
    {
      prv.streamStartSignal(sourceName);
      break;
    }

    case GST_MESSAGE_ELEMENT:
    {
      const GstStructure* structure = gst_message_get_structure(messagePtr);
      prv.elementMessageSignal(sourceName, structure);
      break;
    }

    case GST_MESSAGE_ASYNC_DONE:
    {
      GstClockTime runningTime;
      gst_message_parse_async_done(messagePtr, &runningTime);
      prv.asyncDoneSignal(sourceName, runningTime);
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

std::string MessageParser::getSourceName(const GstMessage& message)
{
  return getObjectNameOrUnknown(GST_MESSAGE_SRC(&message));
}


} // dh::gst
