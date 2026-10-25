/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file test_messageparser.cpp
 * @author Sandro Stiller
 * @date 2024-10-11
 */

#include "messageparser.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>
#include <thread>

using namespace dh::gst;


class MessageParserTest
{
public:
  // Setup before first test case
  MessageParserTest()
  {
    // Set G_DEBUG to fatal_warnings to make warnings crash the program
    setenv("G_DEBUG", "fatal_warnings", 1);
    gst_init(nullptr, nullptr);  // Initialize GStreamer
  }

};

BOOST_FIXTURE_TEST_CASE(EndOfStreamSignalEmitted, MessageParserTest)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string sourceName;

  auto conn = parser->connectSignalEndOfStream([&](const std::string& src)
  {
    signalCalled = true;
    sourceName = src;
  });
  BOOST_REQUIRE(conn.connected());

  GstMessage* message = gst_message_new_eos(nullptr);
  parser->parse(*message);

  BOOST_CHECK(signalCalled);
  BOOST_CHECK_EQUAL(sourceName, "unknown");

  gst_message_unref(message);
}

BOOST_FIXTURE_TEST_CASE(ErrorSignalEmitted, MessageParserTest)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  std::string receivedErrorMessage;
  std::string receivedDebugInfo;

  auto conn = parser->connectSignalError(
    [&](const std::string& src, const std::string& err, const std::string& debug)
    {
      signalCalled = true;
      receivedSourceName = src;
      receivedErrorMessage = err;
      receivedDebugInfo = debug;
    }
  );
  BOOST_REQUIRE(conn.connected());

  GError* error = g_error_new_literal(g_quark_from_static_string("test_error"), 1, "Test error message");
  GstMessage* message = gst_message_new_error(nullptr, error, "Test debug info");
  parser->parse(*message);

  BOOST_CHECK(signalCalled);
  BOOST_CHECK_EQUAL(receivedSourceName, "unknown");
  BOOST_CHECK_EQUAL(receivedErrorMessage, "Test error message");
  BOOST_CHECK_EQUAL(receivedDebugInfo, "Test debug info");

  gst_message_unref(message);
  g_error_free(error);
}

BOOST_FIXTURE_TEST_CASE(WarningSignalEmitted, MessageParserTest)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  std::string receivedWarningMessage;
  std::string receivedDebugInfo;

  auto conn = parser->connectSignalWarning(
    [&](const std::string& src, const std::string& warn, const std::string& debug)
    {
      signalCalled = true;
      receivedSourceName = src;
      receivedWarningMessage = warn;
      receivedDebugInfo = debug;
    }
  );
  BOOST_REQUIRE(conn.connected());

  GError* warning = g_error_new_literal(g_quark_from_static_string("test_warning"), 1, "Test warning message");
  GstMessage* message = gst_message_new_warning(nullptr, warning, "Test warning debug info");
  parser->parse(*message);

  BOOST_CHECK(signalCalled);
  BOOST_CHECK_EQUAL(receivedSourceName, "unknown");
  BOOST_CHECK_EQUAL(receivedWarningMessage, "Test warning message");
  BOOST_CHECK_EQUAL(receivedDebugInfo, "Test warning debug info");

  gst_message_unref(message);
  g_error_free(warning);
}

BOOST_FIXTURE_TEST_CASE(InfoSignalEmitted, MessageParserTest)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  std::string receivedInfoMessage;
  std::string receivedDebugInfo;

  auto conn = parser->connectSignalInfo(
    [&](const std::string& src, const std::string& info, const std::string& debug)
    {
      signalCalled = true;
      receivedSourceName = src;
      receivedInfoMessage = info;
      receivedDebugInfo = debug;
    }
  );
  BOOST_REQUIRE(conn.connected());

  GError* info = g_error_new_literal(g_quark_from_static_string("test_info"), 1, "Test info message");
  GstMessage* message = gst_message_new_info(nullptr, info, "Test info debug");
  parser->parse(*message);

  BOOST_CHECK(signalCalled);
  BOOST_CHECK_EQUAL(receivedSourceName, "unknown");
  BOOST_CHECK_EQUAL(receivedInfoMessage, "Test info message");
  BOOST_CHECK_EQUAL(receivedDebugInfo, "Test info debug");

  gst_message_unref(message);
  g_error_free(info);
}

BOOST_FIXTURE_TEST_CASE(DurationChangedSignalEmitted, MessageParserTest)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;

  auto conn = parser->connectSignalDurationChanged(
    [&](const std::string& src)
    {
      signalCalled = true;
      receivedSourceName = src;
    }
  );
  BOOST_REQUIRE(conn.connected());

  GstElement* element = gst_element_factory_make("fakesrc", "test_source");
  GstMessage* message = gst_message_new_duration_changed(GST_OBJECT(element));
  parser->parse(*message);

  BOOST_CHECK(signalCalled);
  BOOST_CHECK_EQUAL(receivedSourceName, "test_source");

  gst_message_unref(message);
  gst_object_unref(element);
}

BOOST_FIXTURE_TEST_CASE(StateChangedSignalEmitted, MessageParserTest)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  GstState oldState, newState, pendingState;

  auto conn = parser->connectSignalStateChanged(
    [&](const std::string& src, GstState oldS, GstState newS, GstState pendingS)
    {
      signalCalled = true;
      receivedSourceName = src;
      oldState = oldS;
      newState = newS;
      pendingState = pendingS;
    }
  );
  BOOST_REQUIRE(conn.connected());

  GstElement* element = gst_element_factory_make("fakesrc", "test_source");
  GstMessage* message = gst_message_new_state_changed(GST_OBJECT(element), GST_STATE_NULL, GST_STATE_READY, GST_STATE_VOID_PENDING);
  parser->parse(*message);

  BOOST_CHECK(signalCalled);
  BOOST_CHECK_EQUAL(receivedSourceName, "test_source");
  BOOST_CHECK_EQUAL(oldState, GST_STATE_NULL);
  BOOST_CHECK_EQUAL(newState, GST_STATE_READY);
  BOOST_CHECK_EQUAL(pendingState, GST_STATE_VOID_PENDING);

  gst_message_unref(message);
  gst_object_unref(element);
}

BOOST_FIXTURE_TEST_CASE(StreamStatusSignalEmitted, MessageParserTest)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  GstStreamStatusType receivedStatusType;
  std::string receivedElementName;

  auto conn = parser->connectSignalStreamStatus(
    [&](const std::string& source, GstStreamStatusType statusType, const std::string& elementName)
    {
      signalCalled = true;
      receivedSourceName = source;
      receivedStatusType = statusType;
      receivedElementName = elementName;
    }
  );
  BOOST_REQUIRE(conn.connected());

  GstElement* element = gst_element_factory_make("fakesrc", "test_element");
  GstMessage* message = gst_message_new_stream_status(GST_OBJECT(element), GST_STREAM_STATUS_TYPE_ENTER, element);
  parser->parse(*message);

  BOOST_CHECK(signalCalled);
  BOOST_CHECK_EQUAL(receivedSourceName, "test_element");
  BOOST_CHECK_EQUAL(receivedStatusType, GST_STREAM_STATUS_TYPE_ENTER);
  BOOST_CHECK_EQUAL(receivedElementName, "test_element");

  gst_message_unref(message);
  gst_object_unref(element);
}

BOOST_FIXTURE_TEST_CASE(StreamStartSignalEmitted, MessageParserTest)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;

  auto conn = parser->connectSignalStreamStart(
    [&](const std::string& sourceName)
    {
      signalCalled = true;
      receivedSourceName = sourceName;
    }
  );
  BOOST_REQUIRE(conn.connected());

  GstElement* element = gst_element_factory_make("fakesrc", "test_element");
  GstMessage* message = gst_message_new_stream_start(GST_OBJECT(element));
  parser->parse(*message);

  BOOST_CHECK(signalCalled);
  BOOST_CHECK_EQUAL(receivedSourceName, "test_element");

  gst_message_unref(message);
  gst_object_unref(element);
}

BOOST_FIXTURE_TEST_CASE(ElementMessageSignalEmitted, MessageParserTest)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  const GstStructure* receivedStructure = nullptr;

  auto conn = parser->connectSignalElementMessage(
    [&](const std::string& sourceName, const GstStructure* structure)
    {
      signalCalled = true;
      receivedSourceName = sourceName;
      receivedStructure = structure;
    }
  );
  BOOST_REQUIRE(conn.connected());

  GstElement* element = gst_element_factory_make("fakesrc", "test_element");
  GstStructure* structure = gst_structure_new("test_structure", "field", G_TYPE_STRING, "value", nullptr);
  GstMessage* message = gst_message_new_element(GST_OBJECT(element), structure);
  parser->parse(*message);

  BOOST_CHECK(signalCalled);
  BOOST_CHECK_EQUAL(receivedSourceName, "test_element");
  BOOST_REQUIRE_NE(receivedStructure, nullptr);
  BOOST_CHECK_EQUAL(gst_structure_get_name(receivedStructure), "test_structure");
  BOOST_CHECK(gst_structure_has_field(receivedStructure, "field"));

  const gchar* fieldValue = gst_structure_get_string(receivedStructure, "field");
  BOOST_CHECK_EQUAL(fieldValue, "value");

  gst_message_unref(message);
  gst_object_unref(element);
}

BOOST_FIXTURE_TEST_CASE(AsyncDoneSignalEmittedWithRunningTime, MessageParserTest)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  GstClockTime receivedRunningTime = GST_CLOCK_TIME_NONE;

  auto conn = parser->connectSignalAsyncDone(
    [&](const std::string& sourceName, GstClockTime runningTime)
    {
      signalCalled = true;
      receivedSourceName = sourceName;
      receivedRunningTime = runningTime;
    }
  );
  BOOST_REQUIRE(conn.connected());

  GstElement* element = gst_element_factory_make("fakesrc", "test_element");
  GstClockTime expectedRunningTime = 123456789;
  GstMessage* message = gst_message_new_async_done(GST_OBJECT(element), expectedRunningTime);
  parser->parse(*message);

  BOOST_CHECK(signalCalled);
  BOOST_CHECK_EQUAL(receivedSourceName, "test_element");
  BOOST_CHECK_EQUAL(receivedRunningTime, expectedRunningTime);

  gst_message_unref(message);
  gst_object_unref(element);
}

BOOST_FIXTURE_TEST_CASE(SignalDisconnectionPreventsEmission, MessageParserTest)
{
  auto parser = MessageParser::create();
  int callCount = 0;

  auto conn = parser->connectSignalEndOfStream([&](const std::string&)
  {
    ++callCount;
  });
  BOOST_REQUIRE(conn.connected());

  GstMessage* message1 = gst_message_new_eos(nullptr);
  parser->parse(*message1);
  gst_message_unref(message1);
  BOOST_CHECK_EQUAL(callCount, 1);

  conn.disconnect();
  BOOST_CHECK(!conn.connected());

  GstMessage* message2 = gst_message_new_eos(nullptr);
  parser->parse(*message2);
  gst_message_unref(message2);
  BOOST_CHECK_EQUAL(callCount, 1);
}

BOOST_FIXTURE_TEST_CASE(SignalScopeDisconnection, MessageParserTest)
{
  auto parser = MessageParser::create();
  int callCount = 0;

  {
    auto conn = parser->connectSignalEndOfStream([&](const std::string&)
    {
      ++callCount;
    });
    BOOST_REQUIRE(conn.connected());

    GstMessage* message1 = gst_message_new_eos(nullptr);
    parser->parse(*message1);
    gst_message_unref(message1);
    BOOST_CHECK_EQUAL(callCount, 1);
  }

  GstMessage* message2 = gst_message_new_eos(nullptr);
  parser->parse(*message2);
  gst_message_unref(message2);
  BOOST_CHECK_EQUAL(callCount, 1);
}

BOOST_FIXTURE_TEST_CASE(ParserDestroyedFirstSafety, MessageParserTest)
{
  SignalConnection conn;
  {
    auto parser = MessageParser::create();
    conn = parser->connectSignalEndOfStream([](const std::string&) {});
    BOOST_CHECK(conn.connected());
  }

  // Disconnecting or destructing after parser is destroyed should be safe
  BOOST_CHECK_NO_THROW(conn.disconnect());
  BOOST_CHECK(!conn.connected());
}

BOOST_FIXTURE_TEST_CASE(ConnectionReportsDisconnectedAfterParserDestroyed, MessageParserTest)
{
  SignalConnection conn;
  {
    auto parser = MessageParser::create();
    conn = parser->connectSignalEndOfStream([](const std::string&) {});
    BOOST_CHECK(conn.connected());
  }

  // no callback can arrive anymore, without calling disconnect()
  BOOST_CHECK(!conn.connected());
}

BOOST_FIXTURE_TEST_CASE(EmptyCallbackThrowsInvalidArgument, MessageParserTest)
{
  auto parser = MessageParser::create();
  BOOST_CHECK_THROW((void)parser->connectSignalEndOfStream(nullptr), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(OnlyOneCallbackPerMessageType, MessageParserTest)
{
  auto parser = MessageParser::create();
  int firstCalls = 0;
  int secondCalls = 0;

  auto conn = parser->connectSignalEndOfStream([&](const std::string&) { ++firstCalls; });
  BOOST_CHECK_THROW(
    (void)parser->connectSignalEndOfStream([&](const std::string&) { ++secondCalls; }),
    std::logic_error
  );
  // the failed attempt does not affect the connected callback
  BOOST_CHECK(conn.connected());

  // other message types are independent
  auto errorConn = parser->connectSignalError([](const std::string&, const std::string&, const std::string&) {});
  BOOST_CHECK(errorConn.connected());

  GstMessage* message = gst_message_new_eos(nullptr);
  parser->parse(*message);
  BOOST_CHECK_EQUAL(firstCalls, 1);
  BOOST_CHECK_EQUAL(secondCalls, 0);

  // after disconnecting, a new callback can be connected
  conn.disconnect();
  auto newConn = parser->connectSignalEndOfStream([&](const std::string&) { ++secondCalls; });
  parser->parse(*message);
  BOOST_CHECK_EQUAL(firstCalls, 1);
  BOOST_CHECK_EQUAL(secondCalls, 1);

  gst_message_unref(message);
}

BOOST_FIXTURE_TEST_CASE(ReplaceCallbackByDisconnectingFirst, MessageParserTest)
{
  // the documented way to replace a callback
  auto parser = MessageParser::create();
  int firstCalls = 0;
  int secondCalls = 0;
  auto conn = parser->connectSignalEndOfStream([&](const std::string&) { ++firstCalls; });

  conn.disconnect();
  conn = parser->connectSignalEndOfStream([&](const std::string&) { ++secondCalls; });
  BOOST_CHECK(conn.connected());

  GstMessage* message = gst_message_new_eos(nullptr);
  parser->parse(*message);
  BOOST_CHECK_EQUAL(firstCalls, 0);
  BOOST_CHECK_EQUAL(secondCalls, 1);
  gst_message_unref(message);
}

BOOST_FIXTURE_TEST_CASE(ReplaceCallbackByAssignmentThrows, MessageParserTest)
{
  // documented: the new callback is connected before the assignment releases the old connection
  auto parser = MessageParser::create();
  int firstCalls = 0;
  auto conn = parser->connectSignalEndOfStream([&](const std::string&) { ++firstCalls; });

  BOOST_CHECK_THROW(conn = parser->connectSignalEndOfStream([](const std::string&) {}), std::logic_error);

  // the old callback stays connected
  BOOST_CHECK(conn.connected());
  GstMessage* message = gst_message_new_eos(nullptr);
  parser->parse(*message);
  BOOST_CHECK_EQUAL(firstCalls, 1);
  gst_message_unref(message);
}

// --- thread safety: the locking itself is tested in test_callbackslot.cpp, these tests check how MessageParser uses it ---

BOOST_FIXTURE_TEST_CASE(DisconnectWaitsForRunningCallback, MessageParserTest)
{
  // an object that disconnects in its destructor must not be used by its callback after disconnect() returned
  auto parser = MessageParser::create();
  std::atomic<bool> entered{false};
  std::atomic<bool> finished{false};

  auto conn = parser->connectSignalEndOfStream(
    [&](const std::string&)
    {
      entered = true;
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      finished = true;
    }
  );

  GstMessage* message = gst_message_new_eos(nullptr);
  std::thread parserThread([&]() { parser->parse(*message); });
  while(!entered)
  {
    std::this_thread::yield();
  }

  conn.disconnect();
  BOOST_CHECK(finished); // disconnect() returned only after the running call finished

  parserThread.join();
  gst_message_unref(message);
}

BOOST_FIXTURE_TEST_CASE(CallbacksOfDifferentTypesDoNotBlockEachOther, MessageParserTest)
{
  // a callback that waits for another thread that also parses (e.g. a streaming thread) must not deadlock
  auto parser = MessageParser::create();
  GstMessage* durationChanged = gst_message_new_duration_changed(nullptr);
  GstMessage* eos = gst_message_new_eos(nullptr);

  std::promise<void> durationHandled;
  auto durationConn = parser->connectSignalDurationChanged([&](const std::string&) { durationHandled.set_value(); });

  bool otherTypeRan = false;
  std::thread otherThread;
  auto eosConn = parser->connectSignalEndOfStream(
    [&](const std::string&)
    {
      otherThread = std::thread([&]() { parser->parse(*durationChanged); });
      otherTypeRan = durationHandled.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready;
    }
  );

  parser->parse(*eos);
  otherThread.join();
  BOOST_CHECK(otherTypeRan);

  gst_message_unref(eos);
  gst_message_unref(durationChanged);
}

BOOST_FIXTURE_TEST_CASE(DisconnectOfOtherTypeDoesNotWaitForRunningCallback, MessageParserTest)
{
  auto parser = MessageParser::create();
  GstMessage* eos = gst_message_new_eos(nullptr);
  auto errorConn = parser->connectSignalError([](const std::string&, const std::string&, const std::string&) {});

  std::promise<void> disconnected;
  bool disconnectReturned = false;
  std::thread otherThread;
  auto eosConn = parser->connectSignalEndOfStream(
    [&](const std::string&)
    {
      // while this callback runs, another thread disconnects a callback of a different type
      otherThread = std::thread([&]() { errorConn.disconnect(); disconnected.set_value(); });
      disconnectReturned = disconnected.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready;
    }
  );

  parser->parse(*eos);
  otherThread.join();
  BOOST_CHECK(disconnectReturned);
  BOOST_CHECK(!errorConn.connected());

  gst_message_unref(eos);
}
