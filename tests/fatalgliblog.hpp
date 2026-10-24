/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/* Copyright (C) 2024 Sandro Stiller <sandro.stiller@dragonhills.de>
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

#ifndef DH_GST_TESTS_FATALGLIBLOG_HPP
#define DH_GST_TESTS_FATALGLIBLOG_HPP

#include <glib.h>

/*
 * Global Boost.Test fixtures that make GLib/GStreamer log messages abort the test.
 * Register one per test module, after including the Boost.Test header:
 *
 *   BOOST_TEST_GLOBAL_FIXTURE(FatalGlibCriticals);
 *
 * This replaces setting G_DEBUG=fatal-criticals in the test: GLib reads G_DEBUG only once at
 * program start, so it can not be set from inside the test. Setting the fatal mask in code also
 * works when a test binary is started by hand.
 */

/// GLib criticals (e.g. failed g_return_if_fail checks) abort the test
struct FatalGlibCriticals
{
  FatalGlibCriticals()
  {
    g_log_set_always_fatal(static_cast<GLogLevelFlags>(G_LOG_FATAL_MASK | G_LOG_LEVEL_CRITICAL));
  }
};

/// GLib warnings and criticals abort the test
struct FatalGlibWarnings
{
  FatalGlibWarnings()
  {
    g_log_set_always_fatal(
      static_cast<GLogLevelFlags>(G_LOG_FATAL_MASK | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING)
    );
  }
};

#endif // DH_GST_TESTS_FATALGLIBLOG_HPP
