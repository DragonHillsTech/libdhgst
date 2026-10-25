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

#include <memory>
#include <string>

namespace dh::gst
{

class MessageParser::Private
{
public:
  MessageParser::AsyncHandler asyncHandler;

  // callback signatures, all start with the name of the message source
  using SourceSlot = CallbackSlot<const std::string&>;
  using TextMessageSlot = CallbackSlot<const std::string&, const std::string&, const std::string&>;
  using StateChangedSlot = CallbackSlot<const std::string&, GstState, GstState, GstState>;
  using StreamStatusSlot = CallbackSlot<const std::string&, GstStreamStatusType, const std::string&>;
  using ElementMessageSlot = CallbackSlot<const std::string&, const GstStructure*>;
  using AsyncDoneSlot = CallbackSlot<const std::string&, GstClockTime>;

  // one callback slot per message type
  const std::shared_ptr<SourceSlot> endOfStreamSlot = SourceSlot::create("EOS");
  const std::shared_ptr<TextMessageSlot> errorSlot = TextMessageSlot::create("ERROR");
  const std::shared_ptr<StateChangedSlot> stateChangedSlot = StateChangedSlot::create("STATE_CHANGED");
  const std::shared_ptr<TextMessageSlot> warningSlot = TextMessageSlot::create("WARNING");
  const std::shared_ptr<SourceSlot> durationChangedSlot = SourceSlot::create("DURATION_CHANGED");
  const std::shared_ptr<TextMessageSlot> infoSlot = TextMessageSlot::create("INFO");
  const std::shared_ptr<StreamStatusSlot> streamStatusSlot = StreamStatusSlot::create("STREAM_STATUS");
  const std::shared_ptr<SourceSlot> streamStartSlot = SourceSlot::create("STREAM_START");
  const std::shared_ptr<ElementMessageSlot> elementMessageSlot = ElementMessageSlot::create("ELEMENT");
  const std::shared_ptr<AsyncDoneSlot> asyncDoneSlot = AsyncDoneSlot::create("ASYNC_DONE");
};

MessageParser::MessageParser()
: prv{std::make_shared<Private>()}
{
}

MessageParser::~MessageParser() = default;

SignalConnection MessageParser::connectSignalEndOfStream(std::function<void(const std::string& sourceName)> callback)
{
  return prv->endOfStreamSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalError(std::function<void(const std::string& sourceName, const std::string& errorMessage, const std::string& debugInfo)> callback)
{
  return prv->errorSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalStateChanged(std::function<void(const std::string& sourceName, GstState oldState, GstState newState, GstState pendingState)> callback)
{
  return prv->stateChangedSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalWarning(std::function<void(const std::string& sourceName, const std::string& warningMessage, const std::string& debugInfo)> callback)
{
  return prv->warningSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalDurationChanged(std::function<void(const std::string& sourceName)> callback)
{
  return prv->durationChangedSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalInfo(std::function<void(const std::string& sourceName, const std::string& infoMessage, const std::string& debugInfo)> callback)
{
  return prv->infoSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalStreamStatus(std::function<void(const std::string& sourceName, GstStreamStatusType statusType, const std::string& ownerName)> callback)
{
  return prv->streamStatusSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalStreamStart(std::function<void(const std::string& sourceName)> callback)
{
  return prv->streamStartSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalElementMessage(std::function<void(const std::string& sourceName, const GstStructure* structure)> callback)
{
  return prv->elementMessageSlot->connect(std::move(callback));
}

SignalConnection MessageParser::connectSignalAsyncDone(std::function<void(const std::string& sourceName, GstClockTime runningTime)> callback)
{
  return prv->asyncDoneSlot->connect(std::move(callback));
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
      prv->endOfStreamSlot->call(sourceName);
      break;
    }

    case GST_MESSAGE_ERROR:
    {
      GError* error = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_error(messagePtr, &error, &debugInfo);
      prv->errorSlot->call(sourceName, std::string(error->message), std::string(debugInfo ? debugInfo : ""));
      g_clear_error(&error);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_WARNING:
    {
      GError* warning = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_warning(messagePtr, &warning, &debugInfo);
      prv->warningSlot->call(sourceName, std::string(warning->message), std::string(debugInfo ? debugInfo : ""));
      g_clear_error(&warning);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_INFO:
    {
      GError* info = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_info(messagePtr, &info, &debugInfo);
      prv->infoSlot->call(sourceName, std::string(info->message), std::string(debugInfo ? debugInfo : ""));
      g_clear_error(&info);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_STATE_CHANGED:
    {
      GstState oldState, newState, pendingState;
      gst_message_parse_state_changed(messagePtr, &oldState, &newState, &pendingState);
      prv->stateChangedSlot->call(sourceName, oldState, newState, pendingState);
      break;
    }

    case GST_MESSAGE_DURATION_CHANGED:
    {
      prv->durationChangedSlot->call(sourceName);
      break;
    }

    case GST_MESSAGE_STREAM_STATUS:
    {
      GstStreamStatusType statusType;
      GstElement* ownerElement;
      gst_message_parse_stream_status(messagePtr, &statusType, &ownerElement);
      std::string ownerName = ownerElement ? GST_OBJECT_NAME(ownerElement) : "unknown";
      prv->streamStatusSlot->call(sourceName, statusType, ownerName);
      break;
    }

    case GST_MESSAGE_STREAM_START:
    {
      prv->streamStartSlot->call(sourceName);
      break;
    }

    case GST_MESSAGE_ELEMENT:
    {
      const GstStructure* structure = gst_message_get_structure(messagePtr);
      prv->elementMessageSlot->call(sourceName, structure);
      break;
    }

    case GST_MESSAGE_ASYNC_DONE:
    {
      GstClockTime runningTime;
      gst_message_parse_async_done(messagePtr, &runningTime);
      prv->asyncDoneSlot->call(sourceName, runningTime);
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
