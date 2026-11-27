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
#include "internal/callbackslot.hpp"
#include "internal/objectname.hpp"
#include "sharedptrs.hpp"
#include "signalconnection.hpp"
#include "transfertype.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace dh::gst
{

namespace
{

/**
 * @brief Name of a GstObject, or "unknown" if there is no object or it has no name.
 */
std::string getObjectNameOrUnknown(GstObject* object)
{
  if (!object)
  {
    return "unknown";
  }
  const std::string name = getObjectName(*object);
  return name.empty() ? "unknown" : name;
}

/**
 * @brief Message of a GError, or "" if there is none.
 * Error, warning and info messages may be posted without a GError, so the parsed GError
 * can be null.
 */
std::string getErrorMessageOrEmpty(const GError* error)
{
  return error && error->message ? std::string(error->message) : std::string();
}

} // namespace

class MessageParser::Private
{
public:
  MessageParser::AsyncHandler asyncHandler;

  // callback signatures, all start with the name of the message source
  using SourceSlot = CallbackSlot<const std::string&>;
  using TextMessageSlot = CallbackSlot<const std::string&, const std::string&, const std::string&>;
  using StateChangedSlot = CallbackSlot<const std::string&, GstState, GstState, GstState>;
  using StreamStatusSlot =
    CallbackSlot<const std::string&, GstStreamStatusType, const std::string&>;
  using ElementMessageSlot = CallbackSlot<const std::string&, const GstStructure*>;
  using AsyncDoneSlot = CallbackSlot<const std::string&, GstClockTime>;
  using BufferingSlot = CallbackSlot<const std::string&, int>;

  // one callback slot per message type
  const std::shared_ptr<SourceSlot> endOfStreamSlot = SourceSlot::create("EOS");
  const std::shared_ptr<TextMessageSlot> errorSlot = TextMessageSlot::create("ERROR");
  const std::shared_ptr<StateChangedSlot> stateChangedSlot =
    StateChangedSlot::create("STATE_CHANGED");
  const std::shared_ptr<TextMessageSlot> warningSlot = TextMessageSlot::create("WARNING");
  const std::shared_ptr<SourceSlot> durationChangedSlot = SourceSlot::create("DURATION_CHANGED");
  const std::shared_ptr<TextMessageSlot> infoSlot = TextMessageSlot::create("INFO");
  const std::shared_ptr<StreamStatusSlot> streamStatusSlot =
    StreamStatusSlot::create("STREAM_STATUS");
  const std::shared_ptr<SourceSlot> streamStartSlot = SourceSlot::create("STREAM_START");
  const std::shared_ptr<ElementMessageSlot> elementMessageSlot =
    ElementMessageSlot::create("ELEMENT");
  const std::shared_ptr<AsyncDoneSlot> asyncDoneSlot = AsyncDoneSlot::create("ASYNC_DONE");
  const std::shared_ptr<BufferingSlot> bufferingSlot{BufferingSlot::create("BUFFERING")};
  const std::shared_ptr<SourceSlot> clockLostSlot{SourceSlot::create("CLOCK_LOST")};
  const std::shared_ptr<SourceSlot> latencySlot{SourceSlot::create("LATENCY")};
};

MessageParser::MessageParser()
  : prv{std::make_shared<Private>()}
{
}

MessageParser::~MessageParser() = default;

SignalConnection
MessageParser::connectSignalEndOfStream(std::function<void(const std::string& sourceName)> callback)
{
  return prv->endOfStreamSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalError(
  std::function<void(
    const std::string& sourceName,
    const std::string& errorMessage,
    const std::string& debugInfo
  )> callback
)
{
  return prv->errorSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalStateChanged(
  std::function<
    void(const std::string& sourceName, GstState oldState, GstState newState, GstState pendingState)
  > callback
)
{
  return prv->stateChangedSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalWarning(
  std::function<void(
    const std::string& sourceName,
    const std::string& warningMessage,
    const std::string& debugInfo
  )> callback
)
{
  return prv->warningSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalDurationChanged(
  std::function<void(const std::string& sourceName)> callback
)
{
  return prv->durationChangedSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalInfo(
  std::function<void(
    const std::string& sourceName,
    const std::string& infoMessage,
    const std::string& debugInfo
  )> callback
)
{
  return prv->infoSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalStreamStatus(
  std::function<void(
    const std::string& sourceName,
    GstStreamStatusType statusType,
    const std::string& ownerName
  )> callback
)
{
  return prv->streamStatusSlot->connect(std::move(callback));
}

SignalConnection
MessageParser::connectSignalStreamStart(std::function<void(const std::string& sourceName)> callback)
{
  return prv->streamStartSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalElementMessage(
  std::function<void(const std::string& sourceName, const GstStructure* structure)> callback
)
{
  return prv->elementMessageSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalAsyncDone(
  std::function<void(const std::string& sourceName, GstClockTime runningTime)> callback
)
{
  return prv->asyncDoneSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalBuffering(
  std::function<void(const std::string& sourceName, int percent)> callback
)
{
  return prv->bufferingSlot->connect(std::move(callback));
}

SignalConnection
MessageParser::connectSignalClockLost(std::function<void(const std::string& sourceName)> callback)
{
  return prv->clockLostSlot->connect(std::move(callback));
}

SignalConnection
MessageParser::connectSignalLatency(std::function<void(const std::string& sourceName)> callback)
{
  return prv->latencySlot->connect(std::move(callback));
}

void MessageParser::parse(const GstMessage& message)
{
  if (prv->asyncHandler)
  {
    // The task may run after this parser is destroyed, or never: keep the message alive
    // only as long as the task exists and do not touch the parser if it is already gone.
    auto messageSPtr = makeGstSharedPtr(const_cast<GstMessage*>(&message), TransferType::none);
    prv->asyncHandler(
      [weakPrv = std::weak_ptr<Private>(prv), messageSPtr = std::move(messageSPtr)]()
      {
        if (auto lockedPrv = weakPrv.lock())
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
      prv.endOfStreamSlot->call(sourceName);
      break;
    }

    case GST_MESSAGE_ERROR:
    {
      // NOLINTNEXTLINE(misc-const-correctness): &error is passed as GError** to fill and clear it
      GError* error = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_error(messagePtr, &error, &debugInfo);
      prv.errorSlot
        ->call(sourceName, getErrorMessageOrEmpty(error), std::string(debugInfo ? debugInfo : ""));
      g_clear_error(&error);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_WARNING:
    {
      // NOLINTNEXTLINE(misc-const-correctness): &warning is passed as GError** to fill and clear it
      GError* warning = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_warning(messagePtr, &warning, &debugInfo);
      prv.warningSlot->call(
        sourceName,
        getErrorMessageOrEmpty(warning),
        std::string(debugInfo ? debugInfo : "")
      );
      g_clear_error(&warning);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_INFO:
    {
      // NOLINTNEXTLINE(misc-const-correctness): &info is passed as GError** to fill and clear it
      GError* info = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_info(messagePtr, &info, &debugInfo);
      prv.infoSlot
        ->call(sourceName, getErrorMessageOrEmpty(info), std::string(debugInfo ? debugInfo : ""));
      g_clear_error(&info);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_STATE_CHANGED:
    {
      GstState oldState{GST_STATE_VOID_PENDING};
      GstState newState{GST_STATE_VOID_PENDING};
      GstState pendingState{GST_STATE_VOID_PENDING};
      gst_message_parse_state_changed(messagePtr, &oldState, &newState, &pendingState);
      prv.stateChangedSlot->call(sourceName, oldState, newState, pendingState);
      break;
    }

    case GST_MESSAGE_DURATION_CHANGED:
    {
      prv.durationChangedSlot->call(sourceName);
      break;
    }

    case GST_MESSAGE_STREAM_STATUS:
    {
      GstStreamStatusType statusType{GST_STREAM_STATUS_TYPE_CREATE};
      GstElement* ownerElement{nullptr};
      gst_message_parse_stream_status(messagePtr, &statusType, &ownerElement);
      const std::string ownerName = getObjectNameOrUnknown(GST_OBJECT_CAST(ownerElement));
      prv.streamStatusSlot->call(sourceName, statusType, ownerName);
      break;
    }

    case GST_MESSAGE_STREAM_START:
    {
      prv.streamStartSlot->call(sourceName);
      break;
    }

    case GST_MESSAGE_ELEMENT:
    {
      const GstStructure* structure = gst_message_get_structure(messagePtr);
      prv.elementMessageSlot->call(sourceName, structure);
      break;
    }

    case GST_MESSAGE_ASYNC_DONE:
    {
      GstClockTime runningTime{GST_CLOCK_TIME_NONE};
      gst_message_parse_async_done(messagePtr, &runningTime);
      prv.asyncDoneSlot->call(sourceName, runningTime);
      break;
    }

    case GST_MESSAGE_BUFFERING:
    {
      gint percent{0};
      gst_message_parse_buffering(messagePtr, &percent);
      prv.bufferingSlot->call(sourceName, percent);
      break;
    }

    case GST_MESSAGE_CLOCK_LOST:
    {
      prv.clockLostSlot->call(sourceName);
      break;
    }

    case GST_MESSAGE_LATENCY:
    {
      prv.latencySlot->call(sourceName);
      break;
    }

      // Add more message types as needed...

    default:
      GST_WARNING("MessageParser: Unhandled message type '%s'", GST_MESSAGE_TYPE_NAME(messagePtr));
      // throw std::runtime_error("Unsupported message type: " +
      // std::string(GST_MESSAGE_TYPE_NAME(messagePtr)));
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

} // namespace dh::gst
