/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
/**
 * @file typetraits.hpp
 * @author Sandro Stiller
 * @date 2024-09-23
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
template <> struct IsGstObject<GstBuffer> : std::true_type {};
template <> struct IsGstObject<GstEvent> : std::true_type {};
template <> struct IsGstObject<GstMessage> : std::true_type {};
template <> struct IsGstObject<GstAppSink> : std::true_type {};
template <> struct IsGstObject<GstAppSrc> : std::true_type {};
template <> struct IsGstObject<GstBin> : std::true_type {};
template <> struct IsGstObject<GstBus> : std::true_type {};
template <> struct IsGstObject<GstClock> : std::true_type {};
template <> struct IsGstObject<GstDeviceMonitor> : std::true_type {};
template <> struct IsGstObject<GstDevice> : std::true_type {};
template <> struct IsGstObject<GstElementFactory> : std::true_type {};
template <> struct IsGstObject<GstIterator> : std::true_type {};
template <> struct IsGstObject<GstPadTemplate> : std::true_type {};
template <> struct IsGstObject<GstPipeline> : std::true_type {};
template <> struct IsGstObject<GstPlugin> : std::true_type {};
template <> struct IsGstObject<GstSample> : std::true_type {};
template <> struct IsGstObject<GstPluginFeature> : std::true_type {};

// Custom trait for non-GObject GStreamer types
template <> struct IsGstObject<GstCaps> : std::false_type {};
template <> struct IsGstObject<GstStructure> : std::false_type {};


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
