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

#ifndef DH_GST_GVALUETRAITS_HPP
#define DH_GST_GVALUETRAITS_HPP

// local includes
#include "sharedptrs.hpp"
#include "typetraits.hpp"

// std
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

// C
#include <glib-object.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

namespace dh::gst
{

/**
 * @brief Maps a GStreamer C type to its GType.
 */
template<typename T>
struct GstTypeOf
{
  static_assert(!sizeof(T*), "No GType known for this GStreamer type, see GstTypeOf");
};

template<> struct GstTypeOf<GstObject> { static GType get() { return GST_TYPE_OBJECT; } };
template<> struct GstTypeOf<GstElement> { static GType get() { return GST_TYPE_ELEMENT; } };
template<> struct GstTypeOf<GstPad> { static GType get() { return GST_TYPE_PAD; } };
template<> struct GstTypeOf<GstAppSink> { static GType get() { return GST_TYPE_APP_SINK; } };
template<> struct GstTypeOf<GstAppSrc> { static GType get() { return GST_TYPE_APP_SRC; } };
template<> struct GstTypeOf<GstBin> { static GType get() { return GST_TYPE_BIN; } };
template<> struct GstTypeOf<GstBus> { static GType get() { return GST_TYPE_BUS; } };
template<> struct GstTypeOf<GstClock> { static GType get() { return GST_TYPE_CLOCK; } };
template<> struct GstTypeOf<GstDeviceMonitor> { static GType get() { return GST_TYPE_DEVICE_MONITOR; } };
template<> struct GstTypeOf<GstDevice> { static GType get() { return GST_TYPE_DEVICE; } };
template<> struct GstTypeOf<GstElementFactory> { static GType get() { return GST_TYPE_ELEMENT_FACTORY; } };
template<> struct GstTypeOf<GstPadTemplate> { static GType get() { return GST_TYPE_PAD_TEMPLATE; } };
template<> struct GstTypeOf<GstPipeline> { static GType get() { return GST_TYPE_PIPELINE; } };
template<> struct GstTypeOf<GstPlugin> { static GType get() { return GST_TYPE_PLUGIN; } };
template<> struct GstTypeOf<GstPluginFeature> { static GType get() { return GST_TYPE_PLUGIN_FEATURE; } };
// GstMiniObject based types
template<> struct GstTypeOf<GstCaps> { static GType get() { return GST_TYPE_CAPS; } };
template<> struct GstTypeOf<GstMessage> { static GType get() { return GST_TYPE_MESSAGE; } };
template<> struct GstTypeOf<GstEvent> { static GType get() { return GST_TYPE_EVENT; } };
template<> struct GstTypeOf<GstBuffer> { static GType get() { return GST_TYPE_BUFFER; } };

/**
 * @brief Maps a C++ type to GObject properties, used by Object::getProperty and Object::setProperty.
 *
 * Each specialization provides
 *  - `static bool matches(GType propertyType)`: true if the C++ type exactly matches the property type
 *  - `static T get(const GValue* value)`
 *  - `static void set(GValue* value, const T& x)`
 *
 * Supported types:
 *  - bool (gboolean properties)
 *  - integer types: size and signedness must match the property (e.g. gint, guint, gint64, guint64)
 *  - float, double
 *  - std::string (and const char* for setProperty only)
 *  - C++ enums for GEnum and GFlags properties (the concrete enum type can not be verified)
 *  - std::shared_ptr<T> for GstMiniObject based types: GstCapsSPtr, GstMessageSPtr, GstEventSPtr, GstBufferSPtr
 *  - std::shared_ptr<T> for GstObject based types (property type must be exactly T)
 */
template<typename T, typename = void>
struct GValueTraits
{
  static_assert(
    !sizeof(T*),
    "Unsupported property type. Use bool, an integer type matching the property size and signedness, "
    "float, double, std::string, an enum or std::shared_ptr<GstObject or GstMiniObject type>, see GValueTraits"
  );
};

template<> struct GValueTraits<bool>
{
  static bool matches(GType propertyType) { return propertyType == G_TYPE_BOOLEAN; }
  static bool get(const GValue* value) { return g_value_get_boolean(value) != FALSE; }
  static void set(GValue* value, bool x) { g_value_set_boolean(value, x ? TRUE : FALSE); }
};

template<typename T>
struct GValueTraits<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>>
{
  static bool matches(GType propertyType)
  {
    constexpr std::size_t size = sizeof(T);
    constexpr bool isSigned = std::is_signed_v<T>;
    switch(propertyType)
    {
      case G_TYPE_CHAR:   return size == sizeof(gint8) && isSigned;
      case G_TYPE_UCHAR:  return size == sizeof(guint8) && !isSigned;
      case G_TYPE_INT:    return size == sizeof(gint) && isSigned;
      case G_TYPE_UINT:   return size == sizeof(guint) && !isSigned;
      case G_TYPE_LONG:   return size == sizeof(glong) && isSigned;
      case G_TYPE_ULONG:  return size == sizeof(gulong) && !isSigned;
      case G_TYPE_INT64:  return size == sizeof(gint64) && isSigned;
      case G_TYPE_UINT64: return size == sizeof(guint64) && !isSigned;
      default:            return false;
    }
  }

  static T get(const GValue* value)
  {
    switch(G_VALUE_TYPE(value))
    {
      case G_TYPE_CHAR:   return static_cast<T>(g_value_get_schar(value));
      case G_TYPE_UCHAR:  return static_cast<T>(g_value_get_uchar(value));
      case G_TYPE_INT:    return static_cast<T>(g_value_get_int(value));
      case G_TYPE_UINT:   return static_cast<T>(g_value_get_uint(value));
      case G_TYPE_LONG:   return static_cast<T>(g_value_get_long(value));
      case G_TYPE_ULONG:  return static_cast<T>(g_value_get_ulong(value));
      case G_TYPE_INT64:  return static_cast<T>(g_value_get_int64(value));
      case G_TYPE_UINT64: return static_cast<T>(g_value_get_uint64(value));
      default:            throw std::logic_error("GValueTraits: GValue does not hold an integer");
    }
  }

  static void set(GValue* value, T x)
  {
    switch(G_VALUE_TYPE(value))
    {
      case G_TYPE_CHAR:   g_value_set_schar(value, static_cast<gint8>(x)); break;
      case G_TYPE_UCHAR:  g_value_set_uchar(value, static_cast<guchar>(x)); break;
      case G_TYPE_INT:    g_value_set_int(value, static_cast<gint>(x)); break;
      case G_TYPE_UINT:   g_value_set_uint(value, static_cast<guint>(x)); break;
      case G_TYPE_LONG:   g_value_set_long(value, static_cast<glong>(x)); break;
      case G_TYPE_ULONG:  g_value_set_ulong(value, static_cast<gulong>(x)); break;
      case G_TYPE_INT64:  g_value_set_int64(value, static_cast<gint64>(x)); break;
      case G_TYPE_UINT64: g_value_set_uint64(value, static_cast<guint64>(x)); break;
      default:            throw std::logic_error("GValueTraits: GValue does not hold an integer");
    }
  }
};

template<> struct GValueTraits<float>
{
  static bool matches(GType propertyType) { return propertyType == G_TYPE_FLOAT; }
  static float get(const GValue* value) { return g_value_get_float(value); }
  static void set(GValue* value, float x) { g_value_set_float(value, x); }
};

template<> struct GValueTraits<double>
{
  static bool matches(GType propertyType) { return propertyType == G_TYPE_DOUBLE; }
  static double get(const GValue* value) { return g_value_get_double(value); }
  static void set(GValue* value, double x) { g_value_set_double(value, x); }
};

template<> struct GValueTraits<std::string>
{
  static bool matches(GType propertyType) { return propertyType == G_TYPE_STRING; }
  static std::string get(const GValue* value)
  {
    const gchar* str = g_value_get_string(value);
    return str ? std::string(str) : std::string();
  }
  static void set(GValue* value, const std::string& x) { g_value_set_string(value, x.c_str()); }
};

/// for setProperty with string literals; there is no getter because the result would dangle
template<> struct GValueTraits<const char*>
{
  static bool matches(GType propertyType) { return propertyType == G_TYPE_STRING; }
  static void set(GValue* value, const char* x) { g_value_set_string(value, x); }
};

template<> struct GValueTraits<char*> : GValueTraits<const char*> {};

template<typename T>
struct GValueTraits<T, std::enable_if_t<std::is_enum_v<T>>>
{
  static bool matches(GType propertyType)
  {
    return G_TYPE_IS_ENUM(propertyType) || G_TYPE_IS_FLAGS(propertyType);
  }

  static T get(const GValue* value)
  {
    if(G_VALUE_HOLDS_FLAGS(value))
    {
      return static_cast<T>(g_value_get_flags(value));
    }
    return static_cast<T>(g_value_get_enum(value));
  }

  static void set(GValue* value, T x)
  {
    if(G_VALUE_HOLDS_FLAGS(value))
    {
      g_value_set_flags(value, static_cast<guint>(x));
    }
    else
    {
      g_value_set_enum(value, static_cast<gint>(x));
    }
  }
};

template<typename T>
struct GValueTraits<std::shared_ptr<T>, std::enable_if_t<IsGstMiniObject<T>::value>>
{
  static bool matches(GType propertyType) { return propertyType == GstTypeOf<T>::get(); }
  static std::shared_ptr<T> get(const GValue* value)
  {
    // g_value_dup_boxed returns a new reference
    return makeGstSharedPtr(static_cast<T*>(g_value_dup_boxed(value)), TransferType::Full);
  }
  static void set(GValue* value, const std::shared_ptr<T>& x) { g_value_set_boxed(value, x.get()); }
};

template<typename T>
struct GValueTraits<std::shared_ptr<T>, std::enable_if_t<IsGstObject<T>::value>>
{
  static bool matches(GType propertyType) { return propertyType == GstTypeOf<T>::get(); }
  static std::shared_ptr<T> get(const GValue* value)
  {
    // g_value_dup_object returns a new reference
    return makeGstSharedPtr(static_cast<T*>(g_value_dup_object(value)), TransferType::Full);
  }
  static void set(GValue* value, const std::shared_ptr<T>& x) { g_value_set_object(value, x.get()); }
};

namespace detail
{

/**
 * @brief RAII holder for an initialized GValue.
 */
class GValueHolder
{
public:
  explicit GValueHolder(GType type)
  {
    g_value_init(&value, type);
  }

  ~GValueHolder()
  {
    g_value_unset(&value);
  }

  GValueHolder(const GValueHolder&) = delete;
  GValueHolder& operator=(const GValueHolder&) = delete;
  GValueHolder(GValueHolder&&) = delete;
  GValueHolder& operator=(GValueHolder&&) = delete;

  [[nodiscard]] GValue* get() { return &value; }
  [[nodiscard]] const GValue* get() const { return &value; }

private:
  GValue value = G_VALUE_INIT;
};

} // namespace detail

} // namespace dh::gst

#endif // DH_GST_GVALUETRAITS_HPP
