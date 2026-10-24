/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/* Copyright (C) 2026 Sandro Stiller <sandro.stiller@dragonhills.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This file is part of Libdhgst <https://dragonhills.de/>.
 *
 * Libdhgst is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Libdhgst is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Libdhgst. If not, see <http://www.gnu.org/licenses/>.
 */

#include "signalconnection.hpp"
#include "bin.hpp"
#include "elementfactory.hpp"
#include "sharedptrs.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>
#include "fatalgliblog.hpp"
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibWarnings);

#include <glib-object.h>
#include <gst/gst.h>

#include <utility>

using namespace dh::gst;

class SignalConnectionTest
{
public:
  SignalConnectionTest()
  {
    gst_init(nullptr, nullptr);
  }
};

BOOST_FIXTURE_TEST_CASE(DefaultConstruction, SignalConnectionTest)
{
  SignalConnection conn;
  BOOST_CHECK(!conn.connected());
  BOOST_CHECK(!static_cast<bool>(conn));
  BOOST_CHECK_NO_THROW(conn.disconnect());
  BOOST_CHECK(!conn.connected());
}

BOOST_FIXTURE_TEST_CASE(ConnectAndDisconnectManual, SignalConnectionTest)
{
  GstElement* element = gst_element_factory_make("fakesrc", "test-src");
  BOOST_REQUIRE(element != nullptr);

  gulong handlerId = g_signal_connect(
    element,
    "notify::name",
    G_CALLBACK(+[](GObject*, GParamSpec*, gpointer) {}),
    nullptr
  );
  BOOST_REQUIRE_GT(handlerId, 0UL);

  SignalConnection conn(G_OBJECT(element), handlerId);
  BOOST_CHECK(conn.connected());
  BOOST_CHECK(static_cast<bool>(conn));
  BOOST_CHECK(g_signal_handler_is_connected(G_OBJECT(element), handlerId));

  conn.disconnect();
  BOOST_CHECK(!conn.connected());
  BOOST_CHECK(!static_cast<bool>(conn));
  BOOST_CHECK(!g_signal_handler_is_connected(G_OBJECT(element), handlerId));

  // Multiple disconnects should be safe and idempotent
  BOOST_CHECK_NO_THROW(conn.disconnect());

  gst_object_unref(element);
}

BOOST_FIXTURE_TEST_CASE(DestructionDisconnectsSignal, SignalConnectionTest)
{
  GstElement* element = gst_element_factory_make("fakesrc", "test-src");
  BOOST_REQUIRE(element != nullptr);

  gulong handlerId = g_signal_connect(
    element,
    "notify::name",
    G_CALLBACK(+[](GObject*, GParamSpec*, gpointer) {}),
    nullptr
  );
  BOOST_REQUIRE_GT(handlerId, 0UL);

  {
    SignalConnection conn(G_OBJECT(element), handlerId);
    BOOST_CHECK(conn.connected());
    BOOST_CHECK(g_signal_handler_is_connected(G_OBJECT(element), handlerId));
  }

  // Destructor should have disconnected the handler
  BOOST_CHECK(!g_signal_handler_is_connected(G_OBJECT(element), handlerId));

  gst_object_unref(element);
}

BOOST_FIXTURE_TEST_CASE(MoveConstructorTransfersOwnership, SignalConnectionTest)
{
  GstElement* element = gst_element_factory_make("fakesrc", "test-src");
  BOOST_REQUIRE(element != nullptr);

  gulong handlerId = g_signal_connect(
    element,
    "notify::name",
    G_CALLBACK(+[](GObject*, GParamSpec*, gpointer) {}),
    nullptr
  );
  BOOST_REQUIRE_GT(handlerId, 0UL);

  SignalConnection source(G_OBJECT(element), handlerId);
  BOOST_CHECK(source.connected());

  SignalConnection destination(std::move(source));
  BOOST_CHECK(!source.connected());
  BOOST_CHECK(destination.connected());
  BOOST_CHECK(g_signal_handler_is_connected(G_OBJECT(element), handlerId));

  destination.disconnect();
  BOOST_CHECK(!destination.connected());
  BOOST_CHECK(!g_signal_handler_is_connected(G_OBJECT(element), handlerId));

  gst_object_unref(element);
}

BOOST_FIXTURE_TEST_CASE(MoveAssignmentTransfersAndDisconnectsOld, SignalConnectionTest)
{
  GstElement* element1 = gst_element_factory_make("fakesrc", "test-src-1");
  GstElement* element2 = gst_element_factory_make("fakesrc", "test-src-2");
  BOOST_REQUIRE(element1 != nullptr);
  BOOST_REQUIRE(element2 != nullptr);

  gulong handlerId1 = g_signal_connect(
    element1,
    "notify::name",
    G_CALLBACK(+[](GObject*, GParamSpec*, gpointer) {}),
    nullptr
  );
  gulong handlerId2 = g_signal_connect(
    element2,
    "notify::name",
    G_CALLBACK(+[](GObject*, GParamSpec*, gpointer) {}),
    nullptr
  );

  SignalConnection conn1(G_OBJECT(element1), handlerId1);
  SignalConnection conn2(G_OBJECT(element2), handlerId2);

  BOOST_CHECK(conn1.connected());
  BOOST_CHECK(conn2.connected());

  // Assigning conn1 into conn2 should disconnect conn2's old handler
  conn2 = std::move(conn1);

  BOOST_CHECK(!conn1.connected());
  BOOST_CHECK(conn2.connected());
  BOOST_CHECK(!g_signal_handler_is_connected(G_OBJECT(element2), handlerId2));
  BOOST_CHECK(g_signal_handler_is_connected(G_OBJECT(element1), handlerId1));

  conn2.disconnect();
  BOOST_CHECK(!conn2.connected());
  BOOST_CHECK(!g_signal_handler_is_connected(G_OBJECT(element1), handlerId1));

  gst_object_unref(element1);
  gst_object_unref(element2);
}

BOOST_FIXTURE_TEST_CASE(SelfMoveAssignmentIsSafe, SignalConnectionTest)
{
  GstElement* element = gst_element_factory_make("fakesrc", "test-src");
  BOOST_REQUIRE(element != nullptr);

  gulong handlerId = g_signal_connect(
    element,
    "notify::name",
    G_CALLBACK(+[](GObject*, GParamSpec*, gpointer) {}),
    nullptr
  );

  SignalConnection conn(G_OBJECT(element), handlerId);
  BOOST_CHECK(conn.connected());

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
#endif
  auto& connRef = conn;
  conn = std::move(connRef);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  BOOST_CHECK(conn.connected());
  BOOST_CHECK(g_signal_handler_is_connected(G_OBJECT(element), handlerId));

  conn.disconnect();
  BOOST_CHECK(!conn.connected());
  BOOST_CHECK(!g_signal_handler_is_connected(G_OBJECT(element), handlerId));

  gst_object_unref(element);
}

BOOST_FIXTURE_TEST_CASE(TargetObjectDestroyedFirst, SignalConnectionTest)
{
  GstElement* element = gst_element_factory_make("fakesrc", "test-src");
  BOOST_REQUIRE(element != nullptr);

  gulong handlerId = g_signal_connect(
    element,
    "notify::name",
    G_CALLBACK(+[](GObject*, GParamSpec*, gpointer) {}),
    nullptr
  );

  SignalConnection conn(G_OBJECT(element), handlerId);
  BOOST_CHECK(conn.connected());

  // Unref element to destroy it
  gst_object_unref(element);

  // Weak reference should expire gracefully
  BOOST_CHECK(!conn.connected());
  BOOST_CHECK(!static_cast<bool>(conn));

  // Disconnecting an expired object connection should not crash
  BOOST_CHECK_NO_THROW(conn.disconnect());

  // Moving an expired object connection should also work cleanly
  SignalConnection movedConn(std::move(conn));
  BOOST_CHECK(!movedConn.connected());
}

BOOST_FIXTURE_TEST_CASE(SignalEmissionCallbackControl, SignalConnectionTest)
{
  auto bin = Bin::create("test-bin");
  int emissionCount = 0;

  auto connection = bin->connectSignalElementAdded(
    [&](GstElementSPtr)
    {
      ++emissionCount;
    }
  );

  BOOST_REQUIRE(connection.connected());

  auto element1 = ElementFactory::makeElement("fakesrc", "src-1");
  bin->addElement(*element1);

  GMainContext* context = g_main_context_default();
  g_main_context_iteration(context, FALSE);

  BOOST_CHECK_EQUAL(emissionCount, 1);

  // Disconnect the signal connection
  connection.disconnect();
  BOOST_CHECK(!connection.connected());

  auto element2 = ElementFactory::makeElement("fakesrc", "src-2");
  bin->addElement(*element2);

  g_main_context_iteration(context, FALSE);

  // emissionCount should remain 1 after disconnection
  BOOST_CHECK_EQUAL(emissionCount, 1);
}

BOOST_FIXTURE_TEST_CASE(CustomDisconnectHandler, SignalConnectionTest)
{
  bool disconnected = false;
  {
    SignalConnection conn([&]() { disconnected = true; });
    BOOST_CHECK(conn.connected());
    BOOST_CHECK(static_cast<bool>(conn));
  }
  BOOST_CHECK(disconnected);

  bool manualDisconnected = false;
  SignalConnection conn2([&]() { manualDisconnected = true; });
  BOOST_CHECK(conn2.connected());
  conn2.disconnect();
  BOOST_CHECK(manualDisconnected);
  BOOST_CHECK(!conn2.connected());

  // Repeated disconnect call should be safe and not re-invoke handler
  manualDisconnected = false;
  conn2.disconnect();
  BOOST_CHECK(!manualDisconnected);
}

BOOST_FIXTURE_TEST_CASE(CustomDisconnectHandlerMove, SignalConnectionTest)
{
  int disconnectCount = 0;
  SignalConnection conn1([&]() { ++disconnectCount; });
  BOOST_CHECK(conn1.connected());

  SignalConnection conn2(std::move(conn1));
  BOOST_CHECK(!conn1.connected());
  BOOST_CHECK(conn2.connected());
  BOOST_CHECK_EQUAL(disconnectCount, 0);

  conn2.disconnect();
  BOOST_CHECK_EQUAL(disconnectCount, 1);
  BOOST_CHECK(!conn2.connected());
}

BOOST_FIXTURE_TEST_CASE(CustomIsConnectedHandler, SignalConnectionTest)
{
  bool sourceAlive = true;
  int isConnectedCalls = 0;
  SignalConnection conn(
    []() {},
    [&]() { ++isConnectedCalls; return sourceAlive; }
  );
  BOOST_CHECK(conn.connected());

  // the source went away without disconnect() being called
  sourceAlive = false;
  BOOST_CHECK(!conn.connected());
  BOOST_CHECK(!static_cast<bool>(conn));

  // the handler is moved along
  SignalConnection moved(std::move(conn));
  sourceAlive = true;
  BOOST_CHECK(moved.connected());
  BOOST_CHECK(!conn.connected());

  // after disconnect() the handler is not asked anymore
  moved.disconnect();
  const int callsBefore = isConnectedCalls;
  BOOST_CHECK(!moved.connected());
  BOOST_CHECK_EQUAL(isConnectedCalls, callsBefore);
}
