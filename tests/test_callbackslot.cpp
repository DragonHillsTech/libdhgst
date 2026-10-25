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

#include "internal/callbackslot.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>
#include "timedthread.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

using namespace dh::gst;

using IntSlot = CallbackSlot<int>;

BOOST_AUTO_TEST_CASE(CallWithoutCallbackDoesNothing)
{
  auto slot = IntSlot::create("test");
  BOOST_CHECK_NO_THROW(slot->call(1));
}

BOOST_AUTO_TEST_CASE(ConnectAndCall)
{
  auto slot = CallbackSlot<int, const std::string&>::create("test");
  int receivedNumber = 0;
  std::string receivedText;
  auto conn = slot->connect(
    [&](int number, const std::string& text)
    {
      receivedNumber = number;
      receivedText = text;
    }
  );
  BOOST_CHECK(conn.connected());

  slot->call(42, "hello");
  BOOST_CHECK_EQUAL(receivedNumber, 42);
  BOOST_CHECK_EQUAL(receivedText, "hello");
}

BOOST_AUTO_TEST_CASE(EmptyFunctionThrows)
{
  auto slot = IntSlot::create("test");
  BOOST_CHECK_THROW((void)slot->connect(nullptr), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(OnlyOneCallbackAtOnce)
{
  auto slot = IntSlot::create("test");
  int firstCalls = 0;
  int secondCalls = 0;
  auto conn = slot->connect([&](int) { ++firstCalls; });

  BOOST_CHECK_THROW((void)slot->connect([&](int) { ++secondCalls; }), std::logic_error);
  BOOST_CHECK(conn.connected());
  slot->call(1);
  BOOST_CHECK_EQUAL(firstCalls, 1);
  BOOST_CHECK_EQUAL(secondCalls, 0);

  // after disconnecting, a new callback can be connected; the old connection stays disconnected
  conn.disconnect();
  auto newConn = slot->connect([&](int) { ++secondCalls; });
  slot->call(1);
  BOOST_CHECK_EQUAL(firstCalls, 1);
  BOOST_CHECK_EQUAL(secondCalls, 1);
  BOOST_CHECK(!conn.connected());
  BOOST_CHECK(newConn.connected());
}

BOOST_AUTO_TEST_CASE(ConnectionDisconnectsOnDestruction)
{
  auto slot = IntSlot::create("test");
  int calls = 0;
  {
    auto conn = slot->connect([&](int) { ++calls; });
    slot->call(1);
  }
  slot->call(1);
  BOOST_CHECK_EQUAL(calls, 1);
}

BOOST_AUTO_TEST_CASE(ConnectionReportsDisconnectedAfterSlotDestroyed)
{
  SignalConnection conn;
  {
    auto slot = IntSlot::create("test");
    conn = slot->connect([](int) {});
    BOOST_CHECK(conn.connected());
  }
  BOOST_CHECK(!conn.connected());
  BOOST_CHECK_NO_THROW(conn.disconnect());
}

BOOST_AUTO_TEST_CASE(ExceptionPropagatesOutOfCall)
{
  auto slot = IntSlot::create("test");
  auto conn = slot->connect([](int) { throw std::runtime_error("from callback"); });
  BOOST_CHECK_THROW(slot->call(1), std::runtime_error);

  // the slot is still usable: the locks were released
  conn.disconnect();
  BOOST_CHECK(!conn.connected());
}

// --- thread safety and disconnect guarantees ---

BOOST_AUTO_TEST_CASE(ConcurrentConnectDisconnectAndCall)
{
  auto slot = IntSlot::create("test");
  std::atomic<bool> stop{false};

  auto callLoop = [&]()
  {
    while(!stop)
    {
      slot->call(1);
    }
  };
  std::thread caller1(callLoop);
  std::thread caller2(callLoop);

  for(int i = 0; i < 20000; ++i)
  {
    auto conn = slot->connect([](int) {});
    BOOST_REQUIRE(conn.connected());
    conn.disconnect();
  }

  stop = true;
  caller1.join();
  caller2.join();
}

BOOST_AUTO_TEST_CASE(DisconnectWaitsForRunningCallback)
{
  // an object that disconnects in its destructor must not be used by its callback after disconnect() returned
  auto slot = IntSlot::create("test");
  std::atomic<bool> entered{false};
  std::atomic<bool> finished{false};
  auto conn = slot->connect(
    [&](int)
    {
      entered = true;
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      finished = true;
    }
  );

  std::thread caller([&]() { slot->call(1); });
  while(!entered)
  {
    std::this_thread::yield();
  }

  conn.disconnect();
  BOOST_CHECK(finished); // disconnect() returned only after the running call finished

  caller.join();
}

BOOST_AUTO_TEST_CASE(NoCallAfterDisconnectReturned)
{
  auto slot = IntSlot::create("test");
  std::atomic<bool> stop{false};
  std::atomic<int> callsAfterDisconnect{0};

  auto callLoop = [&]()
  {
    while(!stop)
    {
      slot->call(1);
    }
  };
  std::thread caller1(callLoop);
  std::thread caller2(callLoop);

  for(int i = 0; i < 2000; ++i)
  {
    // stands for the state of an object that is destroyed after disconnecting
    auto alive = std::make_shared<std::atomic<bool>>(true);
    auto conn = slot->connect(
      [alive, &callsAfterDisconnect](int)
      {
        if(!*alive)
        {
          ++callsAfterDisconnect;
        }
      }
    );
    std::this_thread::yield();
    conn.disconnect();
    *alive = false;
  }

  stop = true;
  caller1.join();
  caller2.join();
  BOOST_CHECK_EQUAL(callsAfterDisconnect.load(), 0);
}

BOOST_AUTO_TEST_CASE(ConnectedDoesNotWaitForRunningCallback)
{
  auto slot = IntSlot::create("test");
  SignalConnection conn;
  std::promise<void> queried;
  bool connectedReturned = false;
  std::optional<TimedThread> queryThread;

  conn = slot->connect(
    [&](int)
    {
      // while this callback runs, another thread asks whether it is connected
      queryThread.emplace([&]() { (void)conn.connected(); queried.set_value(); });
      connectedReturned = queried.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready;
    }
  );

  slot->call(1);
  queryThread->join();
  BOOST_CHECK(connectedReturned);
}

BOOST_AUTO_TEST_CASE(CallbackMayDisconnectItself)
{
  // must neither deadlock nor destroy the callback while it runs
  auto slot = IntSlot::create("test");
  SignalConnection conn;
  auto token = std::make_shared<int>(42);
  int seen = 0;
  conn = slot->connect(
    [&conn, &seen, token](int)
    {
      conn.disconnect();
      seen = *token; // the captures must still be alive after disconnecting itself
    }
  );

  std::thread caller([&]() { slot->call(1); });
  caller.join();
  BOOST_CHECK_EQUAL(seen, 42);
  BOOST_CHECK(!conn.connected());

  // disconnected: not called again
  seen = 0;
  slot->call(1);
  BOOST_CHECK_EQUAL(seen, 0);
}

BOOST_AUTO_TEST_CASE(SelfDisconnectedCallbackIsDestroyedAfterUnlocking)
{
  // the captures of a callback may run arbitrary code when destroyed, e.g. wait for a thread
  // that uses the slot: they must not be destroyed while the slot is locked
  struct DestructorProbe
  {
    std::function<void()> onDestroy;
    ~DestructorProbe() { onDestroy(); }
  };

  auto slot = IntSlot::create("test");
  bool slotUsableWhileDestroyed = false;
  std::promise<void> called;
  std::optional<TimedThread> otherThread;

  auto probe = std::make_shared<DestructorProbe>();
  probe->onDestroy = [&]()
  {
    otherThread.emplace([&]() { slot->call(2); called.set_value(); });
    slotUsableWhileDestroyed = called.get_future().wait_for(std::chrono::seconds(2)) == std::future_status::ready;
  };

  SignalConnection conn;
  conn = slot->connect(
    [&conn, probe](int)
    {
      conn.disconnect(); // the copy in the running call is now the last reference to this callback
    }
  );
  probe.reset();

  slot->call(1);
  otherThread->join(); // finishes once the slot is unlocked
  BOOST_CHECK(slotUsableWhileDestroyed);
}

BOOST_AUTO_TEST_CASE(CallbackMayDestroyTheSlot)
{
  // e.g. a MessageParser callback that tears everything down when the stream ends:
  // the slot must stay alive until call() has returned and released its locks
  auto slot = IntSlot::create("test");
  const std::weak_ptr<IntSlot> weakSlot = slot;
  bool aliveDuringCallback = false;
  auto conn = slot->connect(
    [&](int)
    {
      slot.reset(); // drops the last owner
      aliveDuringCallback = !weakSlot.expired();
    }
  );

  slot->call(1); // no other owner: the callback releases the only one
  BOOST_CHECK(aliveDuringCallback);
  BOOST_CHECK(weakSlot.expired()); // destroyed once call() returned
  BOOST_CHECK(!conn.connected());
}
