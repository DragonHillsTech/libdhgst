/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file test_bus.cpp
 * @author Sandro Stiller
 * @date 2024-10-05
 */

#include "bus.hpp"
#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>
#include "fatalgliblog.hpp"
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibWarnings);

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
  gst_init(nullptr, nullptr);  // Initialize GStreamer
 }
};

BOOST_FIXTURE_TEST_CASE(BusReceivesMessageFromPost, BusTest)
{
  auto bus = Bus::create(gst_bus_new(), TransferType::Full);
  bool signalReceived = false;
  // Connect to the sync message signal from the Bus
  auto syncMessageConnection = bus->connectSignalNewSyncMessage(
    [&](GstMessageSPtr message)
    {
      signalReceived = true;
      // Check message type and content
      BOOST_REQUIRE_EQUAL(std::string(gst_structure_get_name(gst_message_get_structure(message.get()))), "TestMessage");
    }
  );
  BOOST_REQUIRE(syncMessageConnection.connected());
  // Create a mock message to post to the bus
  // gst_structure_new_empty returns transfer:full
  // gst_message_new_application takes GstMessage transfertype:full and returns full
  GstMessageSPtr mockMessage = makeGstSharedPtr(
    gst_message_new_application(nullptr, gst_structure_new_empty("TestMessage")),
    TransferType::Full
  );
  // Post the message to the bus using the post function
  bus->post(*mockMessage);
  BOOST_REQUIRE(signalReceived);
}

BOOST_FIXTURE_TEST_CASE(SyncMessageConnectionDoesNotKeepBusAlive, BusTest)
{
  GstBus* rawBus = gst_bus_new();
  gpointer weakBus = rawBus;
  g_object_add_weak_pointer(G_OBJECT(rawBus), &weakBus);

  auto bus = Bus::create(rawBus, TransferType::Full);
  auto connection = bus->connectSignalNewSyncMessage([](GstMessageSPtr) {});
  BOOST_REQUIRE(connection.connected());

  // drop the last reference held by the application: only the connection is left
  bus.reset();
  BOOST_CHECK(weakBus == nullptr);
  BOOST_CHECK(!connection.connected());

  if(weakBus)
  {
    g_object_remove_weak_pointer(G_OBJECT(weakBus), &weakBus);
  }
}

BOOST_FIXTURE_TEST_CASE(DisconnectDisablesSyncMessageEmission, BusTest)
{
  auto bus = Bus::create(gst_bus_new(), TransferType::Full);

  // a plain GLib handler only receives sync-message while sync emission is enabled
  int rawEmissions = 0;
  const gulong rawHandler = g_signal_connect(
    bus->getGstBus().get(),
    "sync-message",
    G_CALLBACK(+[](GstBus*, GstMessage*, gpointer userData) { ++*static_cast<int*>(userData); }),
    &rawEmissions
  );

  auto postMessage = [&bus]()
  {
    bus->post(makeGstSharedPtr(
      gst_message_new_application(nullptr, gst_structure_new_empty("TestMessage")),
      TransferType::Full
    ));
  };

  {
    auto connection = bus->connectSignalNewSyncMessage([](GstMessageSPtr) {});
    postMessage();
    BOOST_CHECK_EQUAL(rawEmissions, 1);
  }

  // the connection is gone, so sync emission must be disabled again
  postMessage();
  BOOST_CHECK_EQUAL(rawEmissions, 1);

  g_signal_handler_disconnect(bus->getGstBus().get(), rawHandler);
}
