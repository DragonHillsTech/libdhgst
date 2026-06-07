/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file gtest_bus.cpp
 * @author Sandro Stiller
 * @date 2024-10-05
 */

#include "bus.hpp"
#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>

using namespace dh::gst;

/**
 * @brief Test fixture for setting up and tearing down the test environment
 */
class BusTest
{
public:
 // Setup before first test case
 BusTest()
 {
  // Set G_DEBUG to fatal_warnings to make warnings crash the program
  setenv("G_DEBUG", "fatal_warnings", 1);
  gst_init(nullptr, nullptr);  // Initialize GStreamer
 }
};

BOOST_FIXTURE_TEST_CASE(BusReceivesMessageFromPost, BusTest)
{
  auto bus = Bus::create(gst_bus_new(), TransferType::Full);
  bool signalReceived = false;
  // Connect to the sync message signal from the Bus
  bus->newSyncMessageSignal().connect(
    [&](GstMessageSPtr message)
    {
      signalReceived = true;
      // Check message type and content
      BOOST_REQUIRE_EQUAL(std::string(gst_structure_get_name(gst_message_get_structure(message.get()))), "TestMessage");
    }
  );
  // Create a mock message to post to the bus
  // gst_structure_new_empty returns transfer:full
  // gst_message_new_application takes GstMessage transfertype:full and returns full
  GstMessageSPtr mockMessage = makeGstSharedPtr(
    gst_message_new_application(nullptr, gst_structure_new_empty("TestMessage")),
    TransferType::Full
  );
  // Post the message to the bus using the post function
  bus->post(mockMessage);
  BOOST_REQUIRE(signalReceived);
}

