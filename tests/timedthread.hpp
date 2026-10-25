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

#ifndef DH_GST_TESTS_TIMEDTHREAD_HPP
#define DH_GST_TESTS_TIMEDTHREAD_HPP

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <thread>
#include <utility>

/**
 * @brief A thread for tests that check for deadlocks: join() waits at most a timeout.
 *
 * If the thread does not finish in time, it is deadlocked. The test can not continue safely:
 * the thread is still blocked and may later use the test's local variables. So join() ends the
 * test binary with a failure message instead of hanging until a global timeout.
 * The destructor joins the thread the same way if join() was not called.
 */
class TimedThread
{
public:
  template<typename Function>
  explicit TimedThread(Function function)
  : thread(
      [this, function = std::move(function)]() mutable
      {
        function();
        finished.set_value();
      }
    )
  {
  }

  TimedThread(const TimedThread&) = delete;
  TimedThread& operator=(const TimedThread&) = delete;

  ~TimedThread()
  {
    if(thread.joinable())
    {
      join();
    }
  }

  /// wait for the thread; terminates the test binary if it does not finish within the timeout
  void join(std::chrono::seconds timeout = std::chrono::seconds(10))
  {
    if(finishedFuture.wait_for(timeout) != std::future_status::ready)
    {
      std::cerr << "FATAL in \"" << boost::unit_test::framework::current_test_case().p_name.get()
                << "\": thread did not finish within " << timeout.count() << " s, deadlock" << std::endl;
      std::_Exit(EXIT_FAILURE);
    }
    thread.join();
  }

private:
  // declared before thread: they must exist before the thread function runs
  std::promise<void> finished;
  std::future<void> finishedFuture = finished.get_future();
  std::thread thread;
};

#endif // DH_GST_TESTS_TIMEDTHREAD_HPP
