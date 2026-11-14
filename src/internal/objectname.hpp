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

#ifndef DH_GST_INTERNAL_OBJECTNAME_HPP
#define DH_GST_INTERNAL_OBJECTNAME_HPP

#include <gst/gst.h>

#include <string>

namespace dh::gst
{

/**
 * @brief Name of a GstObject, or an empty string if it has no name.
 * gst_object_get_name() takes the object lock: the object may be renamed by another thread.
 */
inline std::string getObjectName(const GstObject& object)
{
  gchar* name = gst_object_get_name(const_cast<GstObject*>(&object));
  std::string ret = name ? name : "";
  g_free(name);
  return ret;
}

} // namespace dh::gst

#endif // DH_GST_INTERNAL_OBJECTNAME_HPP
