/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file gtest_messageparser.cpp
 * @author Sandro Stiller
 * @date 2024-10-11
 */

#include "messageparser.hpp"

#include <gtest/gtest.h>

using namespace dh::gst;


class MessageParserTest : public ::testing::Test
{
public:
  // Setup before first test case
  static void SetUpTestSuite()
  {
    // Set G_DEBUG to fatal_warnings to make warnings crash the program
    setenv("G_DEBUG", "fatal_warnings", 1);
    gst_init(nullptr, nullptr);  // Initialize GStreamer
  }

  // Cleanup after last test case
  static void TearDownTestSuite()
  {
    gst_deinit();
  }

};

TEST_F(MessageParserTest, EndOfStreamSignalEmitted)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string sourceName;

  parser->endOfStreamSignal.connect([&](const std::string& src)
  {
    signalCalled = true;
    sourceName = src;
  });

  GstMessage* message = gst_message_new_eos(nullptr);
  parser->parse(*message);

  EXPECT_TRUE(signalCalled);
  EXPECT_EQ(sourceName, "unknown");

  gst_message_unref(message);
}

TEST_F(MessageParserTest, ErrorSignalEmitted)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  std::string receivedErrorMessage;
  std::string receivedDebugInfo;

  parser->errorSignal.connect(
    [&](const std::string& src, const std::string& err, const std::string& debug)
    {
      signalCalled = true;
      receivedSourceName = src;
      receivedErrorMessage = err;
      receivedDebugInfo = debug;
    }
  );

  GError* error = g_error_new_literal(g_quark_from_static_string("test_error"), 1, "Test error message");
  GstMessage* message = gst_message_new_error(nullptr, error, "Test debug info");
  parser->parse(*message);

  EXPECT_TRUE(signalCalled);
  EXPECT_EQ(receivedSourceName, "unknown");
  EXPECT_EQ(receivedErrorMessage, "Test error message");
  EXPECT_EQ(receivedDebugInfo, "Test debug info");

  gst_message_unref(message);
  g_error_free(error);
}

TEST_F(MessageParserTest, StateChangedSignalEmitted)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  GstState oldState, newState, pendingState;

  parser->stateChangedSignal.connect(
    [&](const std::string& src, GstState oldS, GstState newS, GstState pendingS)
    {
      signalCalled = true;
      receivedSourceName = src;
      oldState = oldS;
      newState = newS;
      pendingState = pendingS;
    }
  );

  GstElement* element = gst_element_factory_make("fakesrc", "test_source");
  GstMessage* message = gst_message_new_state_changed(GST_OBJECT(element), GST_STATE_NULL, GST_STATE_READY, GST_STATE_VOID_PENDING);
  parser->parse(*message);

  EXPECT_TRUE(signalCalled);
  EXPECT_EQ(receivedSourceName, "test_source");
  EXPECT_EQ(oldState, GST_STATE_NULL);
  EXPECT_EQ(newState, GST_STATE_READY);
  EXPECT_EQ(pendingState, GST_STATE_VOID_PENDING);

  gst_message_unref(message);
  gst_object_unref(element);
}

TEST_F(MessageParserTest, StreamStatusSignalEmitted)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  GstStreamStatusType receivedStatusType;
  std::string receivedElementName;

  parser->streamStatusSignal.connect(
    [&](const std::string& source, GstStreamStatusType statusType, const std::string& elementName)
    {
      signalCalled = true;
      receivedSourceName = source;
      receivedStatusType = statusType;
      receivedElementName = elementName;
    }
  );

  GstElement* element = gst_element_factory_make("fakesrc", "test_element");
  GstMessage* message = gst_message_new_stream_status(GST_OBJECT(element), GST_STREAM_STATUS_TYPE_ENTER, element);
  parser->parse(*message);

  EXPECT_TRUE(signalCalled);
  EXPECT_EQ(receivedSourceName, "test_element");
  EXPECT_EQ(receivedStatusType, GST_STREAM_STATUS_TYPE_ENTER);
  EXPECT_EQ(receivedElementName, "test_element");

  gst_message_unref(message);
  gst_object_unref(element);
}

TEST_F(MessageParserTest, StreamStartSignalEmitted)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;

  parser->streamStartSignal.connect(
    [&](const std::string& sourceName)
    {
      signalCalled = true;
      receivedSourceName = sourceName;
    }
  );

  GstElement* element = gst_element_factory_make("fakesrc", "test_element");
  GstMessage* message = gst_message_new_stream_start(GST_OBJECT(element));
  parser->parse(*message);

  EXPECT_TRUE(signalCalled);
  EXPECT_EQ(receivedSourceName, "test_element");

  gst_message_unref(message);
  gst_object_unref(element);
}

TEST_F(MessageParserTest, ElementMessageSignalEmitted)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  const GstStructure* receivedStructure = nullptr;

  parser->elementMessageSignal.connect(
    [&](const std::string& sourceName, const GstStructure* structure)
    {
      signalCalled = true;
      receivedSourceName = sourceName;
      receivedStructure = structure;
    }
  );

  GstElement* element = gst_element_factory_make("fakesrc", "test_element");
  GstStructure* structure = gst_structure_new("test_structure", "field", G_TYPE_STRING, "value", nullptr);
  GstMessage* message = gst_message_new_element(GST_OBJECT(element), structure);
  parser->parse(*message);

  EXPECT_TRUE(signalCalled);
  EXPECT_EQ(receivedSourceName, "test_element");
  ASSERT_NE(receivedStructure, nullptr);
  EXPECT_STREQ(gst_structure_get_name(receivedStructure), "test_structure");
  EXPECT_TRUE(gst_structure_has_field(receivedStructure, "field"));

  const gchar* fieldValue = gst_structure_get_string(receivedStructure, "field");
  EXPECT_STREQ(fieldValue, "value");

  gst_message_unref(message);
  gst_object_unref(element);
}

TEST_F(MessageParserTest, AsyncDoneSignalEmittedWithRunningTime)
{
  auto parser = MessageParser::create();
  bool signalCalled = false;
  std::string receivedSourceName;
  GstClockTime receivedRunningTime = GST_CLOCK_TIME_NONE;

  parser->asyncDoneSignal.connect(
    [&](const std::string& sourceName, GstClockTime runningTime)
    {
      signalCalled = true;
      receivedSourceName = sourceName;
      receivedRunningTime = runningTime;
    }
  );

  GstElement* element = gst_element_factory_make("fakesrc", "test_element");
  GstClockTime expectedRunningTime = 123456789;
  GstMessage* message = gst_message_new_async_done(GST_OBJECT(element), expectedRunningTime);
  parser->parse(*message);

  EXPECT_TRUE(signalCalled);
  EXPECT_EQ(receivedSourceName, "test_element");
  EXPECT_EQ(receivedRunningTime, expectedRunningTime);

  gst_message_unref(message);
  gst_object_unref(element);
}
