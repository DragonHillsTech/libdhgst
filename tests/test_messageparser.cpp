/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file test_messageparser.cpp
 * @author Sandro Stiller
 * @date 2024-10-11
 */

#include "messageparser.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>
#include "fatalgliblog.hpp"
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibWarnings);

#include <functional>
#include <vector>

using namespace dh::gst;


class MessageParserTest
{
public:
  // Setup before first test case
  MessageParserTest()
  {
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

    // a second connection to the same signal is independent
    auto otherConn = parser->connectSignalEndOfStream([](const std::string&) {});
    otherConn.disconnect();
    BOOST_CHECK(!otherConn.connected());
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

BOOST_FIXTURE_TEST_CASE(AsyncTaskRunAfterParserDestroyedIsSafe, MessageParserTest)
{
  std::vector<std::function<void()>> queuedTasks;
  bool signalCalled = false;

  auto parser = MessageParser::create(
    [&queuedTasks](std::function<void()> task)
    {
      queuedTasks.push_back(std::move(task));
    }
  );
  auto conn = parser->connectSignalEndOfStream(
    [&](const std::string&)
    {
      signalCalled = true;
    }
  );

  GstMessage* message = gst_message_new_eos(nullptr);
  parser->parse(*message);
  BOOST_REQUIRE_EQUAL(queuedTasks.size(), 1);

  // destroy the parser while the task is still queued (e.g. main loop drains after teardown)
  parser.reset();

  // running the queued task must neither touch the destroyed parser nor emit signals
  for(auto& task : queuedTasks)
  {
    task();
  }
  queuedTasks.clear(); // the task owns a message reference until it is destroyed
  BOOST_CHECK(!signalCalled);
  const int refCount = GST_MINI_OBJECT_REFCOUNT_VALUE(message);
  BOOST_CHECK_EQUAL(refCount, 1);

  gst_message_unref(message);
}

BOOST_FIXTURE_TEST_CASE(AsyncTaskDroppedDoesNotLeakMessage, MessageParserTest)
{
  // handler that discards the task without running it (e.g. main loop already stopped)
  auto parser = MessageParser::create(
    [](std::function<void()> /*task*/)
    {
    }
  );

  GstMessage* message = gst_message_new_eos(nullptr);
  parser->parse(*message);

  // the parser must not hold a reference to the message once the task is gone
  const int refCount = GST_MINI_OBJECT_REFCOUNT_VALUE(message);
  BOOST_CHECK_EQUAL(refCount, 1);

  gst_message_unref(message);
}
