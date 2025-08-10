/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
/* Copyright (C) 2024 Sandro Stiller <sandro.stiller@dragonhills.de>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of Libdhgst <https://dragonhills.de/>.
 *
 * Libdhgst is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Libdhgst is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Libdhgst.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_GST_TYPETRAITS_HPP
#define DH_GST_TYPETRAITS_HPP

// std
#include <type_traits>            // For std::type_traits

// gst
#include <gst/gst.h>              // General GStreamer types and functions
#include <gst/app/gstappsink.h>   // For GstAppSink
#include <gst/app/gstappsrc.h>    // For GstAppSrc

// glib
#include <glib.h>                 // For GError, GList, GHashTable, etc.

namespace dh::gst
{

// Custom trait to check if a type is a GObject-based GStreamer type
template <typename T>
struct IsGstObject : std::false_type {};

// Specializations for GObject-based GStreamer types
template <> struct IsGstObject<GstObject> : std::true_type {};
template <> struct IsGstObject<GstElement> : std::true_type {};
template <> struct IsGstObject<GstPad> : std::true_type {};
template <> struct IsGstObject<GstAppSink> : std::true_type {};
template <> struct IsGstObject<GstAppSrc> : std::true_type {};
template <> struct IsGstObject<GstBin> : std::true_type {};
template <> struct IsGstObject<GstBus> : std::true_type {};
template <> struct IsGstObject<GstClock> : std::true_type {};
template <> struct IsGstObject<GstDeviceMonitor> : std::true_type {};
template <> struct IsGstObject<GstDevice> : std::true_type {};
template <> struct IsGstObject<GstElementFactory> : std::true_type {};
template <> struct IsGstObject<GstPadTemplate> : std::true_type {};
template <> struct IsGstObject<GstPipeline> : std::true_type {};
template <> struct IsGstObject<GstPlugin> : std::true_type {};
template <> struct IsGstObject<GstPluginFeature> : std::true_type {};

// Type traits to check for GstMiniObject
template <typename T>
struct IsGstMiniObject : std::false_type {};

// Specializations for GObject-based GStreamer types
template <> struct IsGstMiniObject<GstMessage> : std::true_type {};
template <> struct IsGstMiniObject<GstEvent> : std::true_type {};
template <> struct IsGstMiniObject<GstCaps> : std::true_type {};
template <> struct IsGstMiniObject<GstBuffer> : std::true_type {};

// Type trait to determine if a type is a GObject-derived type
template<typename T>
struct IsGObjectType : std::integral_constant<bool,
    std::is_same<T, GObject>::value || IsGstObject<T>::value> {};

// Add other GObject-derived types
/*template<>
  struct IsGObjectType<GtkWidget> : std::true_type {};
*/

} // dt::gst
#endif //DH_GST_TYPETRAITS_HPP
