/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
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

#ifndef DH_GST_TRANSFERTYPE_HPP
#define DH_GST_TRANSFERTYPE_HPP

namespace dh::gst
{

/// What happens to the reference that is passed together with a raw GObject or GLib pointer
enum class TransferType
{
  full, ///< take over the caller's reference
  none, ///< the caller keeps its reference, an own one is added
  floating ///< sink a floating reference, or add an own one if it is not floating
};

} // namespace dh::gst

#endif // DH_GST_TRANSFERTYPE_HPP
