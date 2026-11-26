/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file test_bus.cpp
 * @author Sandro Stiller
 * @date 2024-10-05
 */

#include "bus.hpp"
#define BOOST_TEST_MODULE libdhgst_tests
#include "fatalgliblog.hpp"
#include <boost/test/included/unit_test.hpp>
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibWarnings);

#include <chrono>
#include <string>
#include <thread>

using namespace dh::gst;

namespace
{

constexpr GstClockTime shortTimeout{10 * GST_MSECOND};
constexpr auto postDelay{std::chrono::milliseconds(50)};
// bounded instead of GST_CLOCK_TIME_NONE, so a bug fails the test instead of hanging it
constexpr GstClockTime waitLimit{5 * GST_SECOND};

void postApplicationMessage(Bus& bus, const char* name)
{
  // gst_message_new_application: takes the structure (transfer:full), returns transfer:full
  const GstMessageSPtr message{makeGstSharedPtr(
    gst_message_new_application(nullptr, gst_structure_new_empty(name)),
    TransferType::full
  )};
  bus.post(*message);
}

std::string structureName(const GstMessageSPtr& message)
{
  return gst_structure_get_name(gst_message_get_structure(message.get()));
}

} // namespace

/**
 * @brief Test fixture for setting up and tearing down the test environment
 */
class BusTest
{
public:
  // Setup before first test case
  BusTest()
  {
    gst_init(nullptr, nullptr); // Initialize GStreamer
  }
};

BOOST_FIXTURE_TEST_CASE(BusReceivesMessageFromPost, BusTest)
{
  auto bus = Bus::create(gst_bus_new(), TransferType::full);
  bool signalReceived = false;
  // Connect to the sync message signal from the Bus
  auto syncMessageConnection = bus->connectSignalNewSyncMessage(
    [&](GstMessageSPtr message)
    {
      signalReceived = true;
      // Check message type and content
      BOOST_REQUIRE_EQUAL(
        std::string(gst_structure_get_name(gst_message_get_structure(message.get()))),
        "TestMessage"
      );
    }
  );
  BOOST_REQUIRE(syncMessageConnection.connected());
  // Create a mock message to post to the bus
  // gst_structure_new_empty returns transfer:full
  // gst_message_new_application takes GstMessage transfertype:full and returns full
  GstMessageSPtr mockMessage = makeGstSharedPtr(
    gst_message_new_application(nullptr, gst_structure_new_empty("TestMessage")),
    TransferType::full
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

  auto bus = Bus::create(rawBus, TransferType::full);
  auto connection = bus->connectSignalNewSyncMessage(
    [](GstMessageSPtr)
    {
    }
  );
  BOOST_REQUIRE(connection.connected());

  // drop the last reference held by the application: only the connection is left
  bus.reset();
  BOOST_CHECK(weakBus == nullptr);
  BOOST_CHECK(!connection.connected());

  if (weakBus)
  {
    g_object_remove_weak_pointer(G_OBJECT(weakBus), &weakBus);
  }
}

BOOST_FIXTURE_TEST_CASE(DisconnectDisablesSyncMessageEmission, BusTest)
{
  auto bus = Bus::create(gst_bus_new(), TransferType::full);

  // a plain GLib handler only receives sync-message while sync emission is enabled
  int rawEmissions = 0;
  const gulong rawHandler = g_signal_connect(
    bus->getGstBus().get(),
    "sync-message",
    G_CALLBACK(+[](GstBus*, GstMessage*, gpointer userData)
               {
                 ++*static_cast<int*>(userData);
               }),
    &rawEmissions
  );

  auto postMessage = [&bus]()
  {
    const auto message = makeGstSharedPtr(
      gst_message_new_application(nullptr, gst_structure_new_empty("TestMessage")),
      TransferType::full
    );
    bus->post(*message);
  };

  {
    auto connection = bus->connectSignalNewSyncMessage(
      [](GstMessageSPtr)
      {
      }
    );
    postMessage();
    BOOST_CHECK_EQUAL(rawEmissions, 1);
  }

  // the connection is gone, so sync emission must be disabled again
  postMessage();
  BOOST_CHECK_EQUAL(rawEmissions, 1);

  g_signal_handler_disconnect(bus->getGstBus().get(), rawHandler);
}

BOOST_FIXTURE_TEST_CASE(PopEmptyBusReturnsNullptr, BusTest)
{
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  BOOST_CHECK(!bus->pop());
  BOOST_CHECK(!bus->timedPop(0));
}

BOOST_FIXTURE_TEST_CASE(PopTakesMessagesInOrder, BusTest)
{
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  postApplicationMessage(*bus, "First");
  postApplicationMessage(*bus, "Second");

  const GstMessageSPtr first{bus->pop()};
  BOOST_REQUIRE(first);
  BOOST_CHECK_EQUAL(structureName(first), "First");
  const GstMessageSPtr second{bus->pop()};
  BOOST_REQUIRE(second);
  BOOST_CHECK_EQUAL(structureName(second), "Second");
  BOOST_CHECK(!bus->pop());
}

BOOST_FIXTURE_TEST_CASE(PopFilteredDiscardsOtherTypes, BusTest)
{
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  postApplicationMessage(*bus, "Skipped");
  // gst_message_new_eos: transfer:full
  const GstMessageSPtr eos{makeGstSharedPtr(gst_message_new_eos(nullptr), TransferType::full)};
  bus->post(*eos);
  postApplicationMessage(*bus, "Remaining");

  const GstMessageSPtr message{bus->pop(GST_MESSAGE_EOS)};
  BOOST_REQUIRE(message);
  BOOST_CHECK_EQUAL(GST_MESSAGE_TYPE(message.get()), GST_MESSAGE_EOS);
  // the application message before the EOS is gone, the one after it is still queued
  const GstMessageSPtr remaining{bus->pop()};
  BOOST_REQUIRE(remaining);
  BOOST_CHECK_EQUAL(structureName(remaining), "Remaining");
  BOOST_CHECK(!bus->pop());
}

BOOST_FIXTURE_TEST_CASE(TimedPopTimesOut, BusTest)
{
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  postApplicationMessage(*bus, "NotMatching");
  BOOST_CHECK(!bus->timedPop(shortTimeout, GST_MESSAGE_EOS));
  // the filter discarded the message, although nothing matched
  BOOST_CHECK(!bus->pop());
}

BOOST_FIXTURE_TEST_CASE(TimedPopWaitsForMessage, BusTest)
{
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  std::thread poster{[&bus]()
                     {
                       std::this_thread::sleep_for(postDelay);
                       postApplicationMessage(*bus, "Late");
                     }};
  const GstMessageSPtr message{bus->timedPop(waitLimit)};
  poster.join();
  BOOST_REQUIRE(message);
  BOOST_CHECK_EQUAL(structureName(message), "Late");
}
