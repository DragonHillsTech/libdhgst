/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file gtest_bus.cpp
 * @author Sandro Stiller
 * @date 2024-10-05
 */

#include "bus.hpp"
#include <gtest/gtest.h>

using namespace dh::gst;

/**
 * @brief Test fixture for setting up and tearing down the test environment for Element class.
 */
class BusTest : public ::testing::Test
{
public:
 // Setup before first test case
 static void SetUpTestSuite()
 {
  // Set G_DEBUG to fatal_criticals to make critical warnings crash the program
  setenv("G_DEBUG", "fatal_warnings", 1);
  gst_init(nullptr, nullptr);  // Initialize GStreamer
 }

 // Cleanup after last test case
 static void TearDownTestSuite()
 {
  gst_deinit();
 }
};

TEST_F(BusTest, BusReceivesMessageFromPost)
{
  auto bus = Bus::create(gst_bus_new(), TransferType::Full);
  bool signalReceived = false;
  // Connect to the sync message signal from the Bus
  bus->newSyncMessageSignal().connect(
    [&](GstMessageSPtr message)
    {
      signalReceived = true;
      // Check message type and content
      ASSERT_STREQ(gst_structure_get_name(gst_message_get_structure(message.get())), "TestMessage")
        << "Received unexpected message structure.";
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
  ASSERT_TRUE(signalReceived) << "The bus did not receive the posted message.";
}