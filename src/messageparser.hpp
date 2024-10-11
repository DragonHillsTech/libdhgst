/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file messageparser.hpp
 * @author Sandro Stiller
 * @date 2024-10-11
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

class MessageParser
{
public:
  /**
   * @brief Parses a GStreamer message and emits corresponding signals based on its type.
   * @param message The GStreamer message to parse.
   * @throws std::runtime_error if the message type is unsupported.
   */
  void parse(const GstMessage& message);

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

private:
  std::string getSourceName(const GstMessage& message) const;
};

} // dh::gst

#endif //DH_GST_MESSAGEPARSER_HPP
