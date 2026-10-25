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

#ifndef DH_GST_MESSAGEPARSER_HPP
#define DH_GST_MESSAGEPARSER_HPP

#include "signalconnection.hpp"

#include <functional>
#include <memory>
#include <string>

#include <gst/gst.h>

namespace dh::gst
{

/**
 * @brief parser for GstMessages. After parsing, the callback for the message type is called.
 *
 * One callback per message type: connecting a second callback for the same type throws
 * std::logic_error until the first one is disconnected. Callbacks for different types are independent.
 *
 * To replace a callback, disconnect the old connection first:
 * @code
 * connection.disconnect();
 * connection = parser->connectSignalEndOfStream(newCallback);
 * @endcode
 * Assigning directly (`connection = parser->connectSignalEndOfStream(newCallback);`) throws,
 * because the new callback is connected before the assignment releases the old connection.
 *
 * Thread safety: parse(), connecting and disconnecting may be called from different threads.
 * When SignalConnection::disconnect() (or its destructor) returns, the callback is not running
 * and will not be called again; if it runs in another thread, disconnect() waits for it.
 * Message types are independent: a running callback never blocks callbacks, connections or
 * disconnections of other message types. A callback is never called concurrently with itself:
 * if parse() is called from several threads, the callbacks of one message type run one after the other.
 * A callback may disconnect itself or connect and disconnect other callbacks.
 * Because disconnecting waits for a running callback of its own type, do not disconnect while holding
 * a lock that this callback needs, and do not let two concurrently running callbacks disconnect each other.
 *
 * Callbacks must not throw. parse() is typically called from C code: a bus sync handler,
 * a bus watch, or a main loop source that runs the task of the async handler.
 * An exception from a callback would propagate out of parse() and unwind through these C frames,
 * which terminates the program or leaves GStreamer/GLib locks held.
 * Catch exceptions inside the callback.
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
  *
  * Example boost.asio:
  * @code
  * create[&ioContext](auto task){boost::asio::post(ioContext, std::move(task));});
  * @endcode
  *
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
   * @brief Parses a GStreamer message and calls the callback for its type, if one is connected.
   * If no async handler is set, the message is processed synchronously.
   * The callbacks must not throw, see the class description.
   * @param message The GStreamer message to parse.
   */
  void parse(const GstMessage& message);

  /**
   * @brief Connect a callback to the End-Of-Stream (EOS) signal.
   * @param callback The callback invoked when an EOS message is received.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a callback for this message type is already connected
   */
  [[nodiscard]] SignalConnection connectSignalEndOfStream(std::function<void(const std::string& sourceName)> callback);

  /**
   * @brief Connect a callback to the error signal.
   * @param callback The callback invoked when an error message is received.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a callback for this message type is already connected
   */
  [[nodiscard]] SignalConnection connectSignalError(std::function<void(const std::string& sourceName, const std::string& errorMessage, const std::string& debugInfo)> callback);

  /**
   * @brief Connect a callback to the state change signal.
   * @param callback The callback invoked when a state change message is received.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a callback for this message type is already connected
   */
  [[nodiscard]] SignalConnection connectSignalStateChanged(std::function<void(const std::string& sourceName, GstState oldState, GstState newState, GstState pendingState)> callback);

  /**
   * @brief Connect a callback to the warning signal.
   * @param callback The callback invoked when a warning message is received.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a callback for this message type is already connected
   */
  [[nodiscard]] SignalConnection connectSignalWarning(std::function<void(const std::string& sourceName, const std::string& warningMessage, const std::string& debugInfo)> callback);

  /**
   * @brief Connect a callback to the duration change signal.
   * @param callback The callback invoked when a duration change message is received.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a callback for this message type is already connected
   */
  [[nodiscard]] SignalConnection connectSignalDurationChanged(std::function<void(const std::string& sourceName)> callback);

  /**
   * @brief Connect a callback to the info signal.
   * @param callback The callback invoked when an info message is received.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a callback for this message type is already connected
   */
  [[nodiscard]] SignalConnection connectSignalInfo(std::function<void(const std::string& sourceName, const std::string& infoMessage, const std::string& debugInfo)> callback);

  /**
   * @brief Connect a callback to the stream status signal.
   * @param callback The callback invoked when a stream status message is received.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a callback for this message type is already connected
   */
  [[nodiscard]] SignalConnection connectSignalStreamStatus(std::function<void(const std::string& sourceName, GstStreamStatusType statusType, const std::string& ownerName)> callback);

  /**
   * @brief Connect a callback to the stream start signal.
   * @param callback The callback invoked when a stream start message is received.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a callback for this message type is already connected
   */
  [[nodiscard]] SignalConnection connectSignalStreamStart(std::function<void(const std::string& sourceName)> callback);

  /**
   * @brief Connect a callback to the element message signal.
   * @param callback The callback invoked when an element specific message is received.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a callback for this message type is already connected
   */
  [[nodiscard]] SignalConnection connectSignalElementMessage(std::function<void(const std::string& sourceName, const GstStructure* structure)> callback);

  /**
   * @brief Connect a callback to the ASYNC_DONE signal.
   * @param callback The callback invoked when an ASYNC_DONE message is received.
   * @throws std::invalid_argument if the callback is empty
   * @throws std::logic_error if a callback for this message type is already connected
   */
  [[nodiscard]] SignalConnection connectSignalAsyncDone(std::function<void(const std::string& sourceName, GstClockTime runningTime)> callback);

private:
  void parseSync(const GstMessage& message);
  std::string getSourceName(const GstMessage& message) const;
  class Private;
  std::shared_ptr<Private> prv;
};

} // dh::gst

#endif //DH_GST_MESSAGEPARSER_HPP

