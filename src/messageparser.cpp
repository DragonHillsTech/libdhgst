/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/* Copyright (C) 2024 Sandro Stiller <sandro.stiller@dragonhills.de>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of Libdhgst <https://dragonhills.de/>.
 *
 * Libdhgst is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Libdhgst is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Libdhgst.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "messageparser.hpp"

namespace dh::gst
{

class MessageParser::Private
{
public:
  MessageParser::AsyncHandler asyncHandler;
};

MessageParser::MessageParser()
: prv{std::make_unique<Private>()}
{
}

MessageParser::~MessageParser() = default;

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
  parseSync(message);
}


void MessageParser::parseSync(const GstMessage& message)
{
  const std::string sourceName = getSourceName(message);
  auto* messagePtr = const_cast<GstMessage*>(&message);

  switch (GST_MESSAGE_TYPE(&message))
  {
    case GST_MESSAGE_EOS:
    {
      endOfStreamSignal(sourceName);
      break;
    }

    case GST_MESSAGE_ERROR:
    {
      GError* error = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_error(messagePtr, &error, &debugInfo);
      errorSignal(sourceName, error->message, debugInfo ? debugInfo : "");
      g_clear_error(&error);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_WARNING:
    {
      GError* warning = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_warning(messagePtr, &warning, &debugInfo);
      warningSignal(sourceName, warning->message, debugInfo ? debugInfo : "");
      g_clear_error(&warning);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_INFO:
    {
      GError* info = nullptr;
      gchar* debugInfo = nullptr;
      gst_message_parse_info(messagePtr, &info, &debugInfo);
      infoSignal(sourceName, info->message, debugInfo ? debugInfo : "");
      g_clear_error(&info);
      g_free(debugInfo);
      break;
    }

    case GST_MESSAGE_STATE_CHANGED:
    {
      GstState oldState, newState, pendingState;
      gst_message_parse_state_changed(messagePtr, &oldState, &newState, &pendingState);
      stateChangedSignal(sourceName, oldState, newState, pendingState);
      break;
    }

    case GST_MESSAGE_DURATION_CHANGED:
    {
      durationChangedSignal(sourceName);
      break;
    }

    case GST_MESSAGE_STREAM_STATUS:
    {
      GstStreamStatusType statusType;
      GstElement* ownerElement;
      gst_message_parse_stream_status(messagePtr, &statusType, &ownerElement);
      std::string ownerName = ownerElement ? GST_OBJECT_NAME(ownerElement) : "unknown";
      streamStatusSignal(sourceName, statusType, ownerName);
      break;
    }

    case GST_MESSAGE_STREAM_START:
    {
      streamStartSignal(sourceName);
      break;
    }

    case GST_MESSAGE_ELEMENT:
    {
      const GstStructure* structure = gst_message_get_structure(messagePtr);
      elementMessageSignal(sourceName, structure);
      break;
    }

    case GST_MESSAGE_ASYNC_DONE:
    {
      GstClockTime runningTime;
      gst_message_parse_async_done(messagePtr, &runningTime);
      asyncDoneSignal(sourceName, runningTime);
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
  auto ret =  std::shared_ptr<MessageParser>(new MessageParser());
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
