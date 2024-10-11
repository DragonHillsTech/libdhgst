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

/**
 * @brief parser for GstMessages. After parsing, the matching signal is emitted.
 */
class MessageParser
{
private:
 MessageParser();
public:
  [[nodiscard]] static std::shared_ptr<MessageParser> create();
  virtual ~MessageParser();

  /**
   * @brief Parses a GStreamer message and emits corresponding signals based on its type.
   * This function is virtual to enable users to create their own implementation for posting the parsing to a main loop.
   * @param message The GStreamer message to parse.
   */
  virtual void parse(const GstMessage& message);

protected:
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
};

} // dh::gst

#endif //DH_GST_MESSAGEPARSER_HPP
