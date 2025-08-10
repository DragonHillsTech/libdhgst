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

#ifndef DH_GST_MESSAGEPARSER_HPP
#define DH_GST_MESSAGEPARSER_HPP

#include <boost/signals2.hpp>

#include <string>
#include <stdexcept>

#include <gst/gst.h>

namespace bs2 = boost::signals2;

namespace dh::gst
{

/**
 * @brief parser for GstMessages. After parsing, the matching signal is emitted.
 */
class MessageParser
{
private:
 MessageParser();
public:
  [[nodiscard]] static std::shared_ptr<MessageParser> create();

  using AsyncHandler = std::function<void(std::function<void()>)>;
 /**
  * @brief Sets a custom asynchronous handler for parse().
  * @param handler A function that takes a callable and posts it to the desired main loop.
  * Example boost.asio:
  * @code
  * create[&ioContext](auto task){boost::asio::post(ioContext, std::move(task));});
  * @endcode
  * Example glib:
  * @code
  * create(
  *   [](auto task)
  *   {
  *     // Use g_main_context_invoke to post the task to the default main context.
  *     g_main_context_invoke(nullptr, [](gpointer userData) -> gboolean
  *     {
  *       auto taskPtr = static_cast<std::function<void()>*>(userData);
  *       (*taskPtr)(); // Call the task
  *       delete taskPtr; // Clean up
  *       return G_SOURCE_REMOVE; // Remove the source after it's executed
  *     },
  *     new std::function<void()>(std::move(task)));
  *   }
  * );
  * @endcode
  */
  [[nodiscard]] static std::shared_ptr<MessageParser> create(AsyncHandler handler);

  ~MessageParser();

  /**
   * @brief Parses a GStreamer message and emits corresponding signals based on its type.
   * If no async handler is set, the message is processed synchronously.
   * @param message The GStreamer message to parse.
   */
  void parse(const GstMessage& message);

private:
  void parseSync(const GstMessage& message);

public:

  /**
   * @brief Signal emitted when an End-Of-Stream (EOS) message is received.
   * @param sourceName The name of the element that generated the message.
   */
  bs2::signal<void(const std::string& sourceName)> endOfStreamSignal;

  /**
   * @brief Signal emitted when an error message is received.
   * @param sourceName The name of the element that generated the message.
   * @param errorMessage The error message.
   * @param debugInfo Additional debug information.
   */
  bs2::signal<void(const std::string& sourceName, const std::string& errorMessage, const std::string& debugInfo)> errorSignal;

  /**
   * @brief Signal emitted when a state change message is received.
   * @param sourceName The name of the element that generated the message.
   * @param oldState The previous state.
   * @param newState The new state.
   * @param pendingState The pending state.
   */
  bs2::signal<void(const std::string& sourceName, GstState oldState, GstState newState, GstState pendingState)> stateChangedSignal;

  /**
   * @brief Signal emitted when a warning message is received.
   * @param sourceName The name of the element that generated the message.
   * @param warningMessage The warning message.
   * @param debugInfo Additional debug information.
   */
  bs2::signal<void(const std::string& sourceName, const std::string& warningMessage, const std::string& debugInfo)> warningSignal;

  /**
   * @brief Signal emitted when a duration change message is received.
   * @param sourceName The name of the element that generated the message.
   */
  bs2::signal<void(const std::string& sourceName)> durationChangedSignal;

  /**
   * @brief Signal emitted when an info message is received.
   * @param sourceName The name of the element that generated the message.
   * @param infoMessage The info message.
   * @param debugInfo Additional debug information.
   */
  bs2::signal<void(const std::string& sourceName, const std::string& infoMessage, const std::string& debugInfo)> infoSignal;

  /**
   * @brief Signal emitted when a stream status message is received.
   * @param sourceName The name of the element that generated the message.
   * @param statusType The stream status type.
   * @param ownerName The owner element of the message source.
   */
  bs2::signal<void(const std::string& sourceName, GstStreamStatusType statusType, const std::string& ownerName)> streamStatusSignal;

 /**
  * @brief Signal emitted when a stream has started
  * @param sourceName The name of the element that generated the message.
  */
  bs2::signal<void(const std::string& sourceName)> streamStartSignal;

  /**
   * @brief an element specific message was received.
   */
  bs2::signal<void(const std::string& sourceName, const GstStructure* structure)> elementMessageSignal;

  /**
   * @brief Signal emitted when an ASYNC_DONE message is received.
   * @param sourceName The name of the element that generated the message.
   * @param runningTime The running time associated with the async done message (in nanoseconds).
   */
  bs2::signal<void(const std::string& sourceName, GstClockTime runningTime)> asyncDoneSignal;


private:
  std::string getSourceName(const GstMessage& message) const;
  class Private;
  std::unique_ptr<Private> prv;
};

} // dh::gst

#endif //DH_GST_MESSAGEPARSER_HPP
