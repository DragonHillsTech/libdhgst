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

#include "internal/signal.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

using namespace dh::gst;

BOOST_AUTO_TEST_CASE(EmptySignalInvocation)
{
  Signal<> sigNoArgs;
  BOOST_CHECK_NO_THROW(sigNoArgs());

  Signal<int, const std::string&> sigWithArgs;
  BOOST_CHECK_NO_THROW(sigWithArgs(42, "hello"));
}

BOOST_AUTO_TEST_CASE(SingleSlotInvocation)
{
  Signal<int, const std::string&> sig;
  int receivedInt = 0;
  std::string receivedStr;
  int callCount = 0;

  sig.connect(
    [&](int val, const std::string& str)
    {
      ++callCount;
      receivedInt = val;
      receivedStr = str;
    }
  );

  sig(123, "test_payload");
  BOOST_CHECK_EQUAL(callCount, 1);
  BOOST_CHECK_EQUAL(receivedInt, 123);
  BOOST_CHECK_EQUAL(receivedStr, "test_payload");

  sig(456, "second_call");
  BOOST_CHECK_EQUAL(callCount, 2);
  BOOST_CHECK_EQUAL(receivedInt, 456);
  BOOST_CHECK_EQUAL(receivedStr, "second_call");
}

BOOST_AUTO_TEST_CASE(MultipleSlotsInvocationOrder)
{
  Signal<int> sig;
  std::vector<int> executionOrder;

  sig.connect(
    [&](int val)
    {
      executionOrder.push_back(1 * 10 + val);
    }
  );

  sig.connect(
    [&](int val)
    {
      executionOrder.push_back(2 * 10 + val);
    }
  );

  sig.connect(
    [&](int val)
    {
      executionOrder.push_back(3 * 10 + val);
    }
  );

  sig(5);
  BOOST_REQUIRE_EQUAL(executionOrder.size(), 3UL);
  BOOST_CHECK_EQUAL(executionOrder[0], 15);
  BOOST_CHECK_EQUAL(executionOrder[1], 25);
  BOOST_CHECK_EQUAL(executionOrder[2], 35);
}

BOOST_AUTO_TEST_CASE(DisconnectSlot)
{
  Signal<int> sig;
  int count1 = 0;
  int count2 = 0;
  int count3 = 0;

  auto id1 = sig.connect([&](int) { ++count1; });
  auto id2 = sig.connect([&](int) { ++count2; });
  auto id3 = sig.connect([&](int) { ++count3; });

  sig(1);
  BOOST_CHECK_EQUAL(count1, 1);
  BOOST_CHECK_EQUAL(count2, 1);
  BOOST_CHECK_EQUAL(count3, 1);

  // Disconnect the middle slot
  sig.disconnect(id2);

  sig(2);
  BOOST_CHECK_EQUAL(count1, 2);
  BOOST_CHECK_EQUAL(count2, 1);
  BOOST_CHECK_EQUAL(count3, 2);

  // Disconnect remaining slots
  sig.disconnect(id1);
  sig.disconnect(id3);

  sig(3);
  BOOST_CHECK_EQUAL(count1, 2);
  BOOST_CHECK_EQUAL(count2, 1);
  BOOST_CHECK_EQUAL(count3, 2);
}

BOOST_AUTO_TEST_CASE(DisconnectNonExistentIsSafe)
{
  Signal<int> sig;
  int count = 0;
  auto id = sig.connect([&](int) { ++count; });

  BOOST_CHECK_NO_THROW(sig.disconnect(999999));
  BOOST_CHECK_NO_THROW(sig.disconnect(0));

  sig(1);
  BOOST_CHECK_EQUAL(count, 1);

  sig.disconnect(id);
  // Second disconnect of same id
  BOOST_CHECK_NO_THROW(sig.disconnect(id));
}

BOOST_AUTO_TEST_CASE(DisconnectionDuringEmission)
{
  Signal<int> sig;
  Signal<int>::Id id1 = 0;
  Signal<int>::Id id2 = 0;
  int count1 = 0;
  int count2 = 0;

  id1 = sig.connect(
    [&](int)
    {
      ++count1;
      sig.disconnect(id1);
      sig.disconnect(id2);
    }
  );

  id2 = sig.connect(
    [&](int)
    {
      ++count2;
    }
  );

  // When emitted, both callbacks from the snapshot run during the first emission
  sig(10);
  BOOST_CHECK_EQUAL(count1, 1);
  BOOST_CHECK_EQUAL(count2, 1);

  // Second emission: both slots were disconnected
  sig(20);
  BOOST_CHECK_EQUAL(count1, 1);
  BOOST_CHECK_EQUAL(count2, 1);
}

BOOST_AUTO_TEST_CASE(MultipleArgumentsAndTypes)
{
  Signal<const std::string&, int, double, void*> sig;
  int called = 0;
  void* dummyPtr = reinterpret_cast<void*>(0x1234);

  sig.connect(
    [&](const std::string& str, int i, double d, void* ptr)
    {
      ++called;
      BOOST_CHECK_EQUAL(str, "test");
      BOOST_CHECK_EQUAL(i, 42);
      BOOST_CHECK_CLOSE(d, 3.14159, 1e-5);
      BOOST_CHECK_EQUAL(ptr, dummyPtr);
    }
  );

  sig("test", 42, 3.14159, dummyPtr);
  BOOST_CHECK_EQUAL(called, 1);
}

BOOST_AUTO_TEST_CASE(ConcurrentConnectDisconnectAndEmission)
{
  // MessageParser emits on the thread calling parse() (e.g. a streaming thread)
  // while the application connects and disconnects on another thread.
  Signal<int> sig;
  std::atomic<bool> stop{false};
  std::atomic<int> persistentCalls{0};
  std::atomic<int> temporaryCalls{0};

  const auto persistentId = sig.connect([&](int) { ++persistentCalls; });

  auto emitter = [&]()
  {
    while(!stop)
    {
      sig(1);
    }
  };
  std::thread emitter1(emitter);
  std::thread emitter2(emitter);

  for(int i = 0; i < 20000; ++i)
  {
    const auto id = sig.connect([&](int) { ++temporaryCalls; });
    sig.disconnect(id);
  }

  stop = true;
  emitter1.join();
  emitter2.join();

  BOOST_CHECK_GT(persistentCalls.load(), 0);

  // after all emitters are done, only the persistent slot is left
  sig.disconnect(persistentId);
  const int callsBefore = temporaryCalls + persistentCalls;
  sig(1);
  BOOST_CHECK_EQUAL(temporaryCalls + persistentCalls, callsBefore);
}
