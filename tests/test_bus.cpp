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
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

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

/**
 * @brief A main context that is the thread-default one while it exists, so a bus watch attaches to
 * it and the test dispatches it without running a main loop.
 */
class ThreadDefaultContext
{
public:
  ThreadDefaultContext()
  {
    g_main_context_push_thread_default(context);
  }

  ~ThreadDefaultContext()
  {
    g_main_context_pop_thread_default(context);
    g_main_context_unref(context);
  }

  ThreadDefaultContext(const ThreadDefaultContext&) = delete;
  ThreadDefaultContext(ThreadDefaultContext&&) = delete;
  ThreadDefaultContext& operator=(const ThreadDefaultContext&) = delete;
  ThreadDefaultContext& operator=(ThreadDefaultContext&&) = delete;

  void dispatchPending()
  {
    while (g_main_context_iteration(context, FALSE))
    {
    }
  }

private:
  GMainContext* context{g_main_context_new()};
};

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

BOOST_FIXTURE_TEST_CASE(WatchCallsCallbackForEachMessage, BusTest)
{
  ThreadDefaultContext context;
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  std::vector<std::string> received;
  const auto connection{bus->connectWatch(
    [&received](GstMessageSPtr message) -> bool
    {
      received.push_back(structureName(message));
      return true;
    }
  )};

  postApplicationMessage(*bus, "First");
  postApplicationMessage(*bus, "Second");
  context.dispatchPending();

  BOOST_CHECK(received == (std::vector<std::string>{"First", "Second"}));
  BOOST_CHECK(connection.connected());
}

BOOST_FIXTURE_TEST_CASE(WatchReturningFalseRemovesWatch, BusTest)
{
  ThreadDefaultContext context;
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  int calls{0};
  const auto connection{bus->connectWatch(
    [&calls](GstMessageSPtr /*message*/) -> bool
    {
      ++calls;
      return false;
    }
  )};

  postApplicationMessage(*bus, "First");
  postApplicationMessage(*bus, "Second");
  context.dispatchPending();

  BOOST_CHECK_EQUAL(calls, 1);
  BOOST_CHECK(!connection.connected());
  // the removed watch is released although its connection still exists
  BOOST_CHECK_NO_THROW((void)bus->connectWatch(
    [](GstMessageSPtr /*message*/) -> bool
    {
      return true;
    }
  ));
}

BOOST_FIXTURE_TEST_CASE(DisconnectRemovesWatch, BusTest)
{
  ThreadDefaultContext context;
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  int calls{0};
  auto connection{bus->connectWatch(
    [&calls](GstMessageSPtr /*message*/) -> bool
    {
      ++calls;
      return true;
    }
  )};

  connection.disconnect();
  BOOST_CHECK(!connection.connected());
  postApplicationMessage(*bus, "AfterDisconnect");
  context.dispatchPending();

  BOOST_CHECK_EQUAL(calls, 0);
  // the message stays on the bus
  BOOST_CHECK(bus->pop());
  BOOST_CHECK_NO_THROW((void)bus->connectWatch(
    [](GstMessageSPtr /*message*/) -> bool
    {
      return true;
    }
  ));
}

BOOST_FIXTURE_TEST_CASE(DestroyedConnectionRemovesWatch, BusTest)
{
  ThreadDefaultContext context;
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  int calls{0};
  {
    const auto connection{bus->connectWatch(
      [&calls](GstMessageSPtr /*message*/) -> bool
      {
        ++calls;
        return true;
      }
    )};
  }

  postApplicationMessage(*bus, "AfterDestruction");
  context.dispatchPending();

  BOOST_CHECK_EQUAL(calls, 0);
}

BOOST_FIXTURE_TEST_CASE(SecondWatchThrows, BusTest)
{
  ThreadDefaultContext context;
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  const auto connection{bus->connectWatch(
    [](GstMessageSPtr /*message*/) -> bool
    {
      return true;
    }
  )};

  BOOST_CHECK_THROW(
    (void)bus->connectWatch(
      [](GstMessageSPtr /*message*/) -> bool
      {
        return true;
      }
    ),
    std::runtime_error
  );
  BOOST_CHECK(connection.connected());
}

BOOST_FIXTURE_TEST_CASE(WatchWithEmptyCallbackThrows, BusTest)
{
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};

  BOOST_CHECK_THROW((void)bus->connectWatch({}), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(ExceptionInWatchCallbackKeepsWatch, BusTest)
{
  ThreadDefaultContext context;
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  int calls{0};
  const auto connection{bus->connectWatch(
    [&calls](GstMessageSPtr /*message*/) -> bool
    {
      ++calls;
      throw std::runtime_error("test exception");
    }
  )};

  postApplicationMessage(*bus, "First");
  postApplicationMessage(*bus, "Second");
  context.dispatchPending();

  BOOST_CHECK_EQUAL(calls, 2);
  BOOST_CHECK(connection.connected());
}

BOOST_FIXTURE_TEST_CASE(WatchConnectionOutlivesBusWrapper, BusTest)
{
  ThreadDefaultContext context;
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  auto connection{bus->connectWatch(
    [](GstMessageSPtr /*message*/) -> bool
    {
      return true;
    }
  )};

  bus.reset();
  BOOST_CHECK(connection.connected());
  connection.disconnect();
  BOOST_CHECK(!connection.connected());
}

BOOST_FIXTURE_TEST_CASE(WatchIsRemovedWithItsContext, BusTest)
{
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  SignalConnection connection;
  {
    ThreadDefaultContext context;
    connection = bus->connectWatch(
      [](GstMessageSPtr /*message*/) -> bool
      {
        return true;
      }
    );
  }

  BOOST_CHECK(!connection.connected());
  BOOST_CHECK_NO_THROW((void)bus->connectWatch(
    [](GstMessageSPtr /*message*/) -> bool
    {
      return true;
    }
  ));
}

BOOST_FIXTURE_TEST_CASE(WatchWithoutPushedContextUsesGlobalDefault, BusTest)
{
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  int calls{0};
  const auto connection{bus->connectWatch(
    [&calls](GstMessageSPtr /*message*/) -> bool
    {
      ++calls;
      return true;
    }
  )};

  postApplicationMessage(*bus, "Global");
  while (g_main_context_iteration(nullptr, FALSE))
  {
  }

  BOOST_CHECK_EQUAL(calls, 1);
}

BOOST_FIXTURE_TEST_CASE(WatchCallbackCanDisconnectItself, BusTest)
{
  ThreadDefaultContext context;
  auto bus{Bus::create(gst_bus_new(), TransferType::full)};
  int calls{0};
  SignalConnection connection;
  connection = bus->connectWatch(
    [&calls, &connection](GstMessageSPtr /*message*/) -> bool
    {
      ++calls;
      connection.disconnect();
      return true;
    }
  );

  postApplicationMessage(*bus, "First");
  postApplicationMessage(*bus, "Second");
  context.dispatchPending();

  BOOST_CHECK_EQUAL(calls, 1);
  BOOST_CHECK(!connection.connected());
  BOOST_CHECK_NO_THROW((void)bus->connectWatch(
    [](GstMessageSPtr /*message*/) -> bool
    {
      return true;
    }
  ));
}
