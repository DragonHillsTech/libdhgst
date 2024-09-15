/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
/**
 * @file sharedptrs.cpp
 * @author Sandro Stiller
 * @date 2024-09-01
 */

#ifndef DH_GST_SHAREDPTRS_HPP
#define DH_GST_SHAREDPTRS_HPP

#include "transfertype.hpp"       // Include the external header for TransferType

#include <gst/app/gstappsink.h>   // For GstAppSink
#include <gst/app/gstappsrc.h>    // For GstAppSrc
#include <gst/gst.h>              // General GStreamer types and functions
#include <glib.h>                 // For GError, GList, GHashTable, etc.

#include <memory>                 // For std::shared_ptr
#include <stdexcept>
#include <type_traits>            // For std::type_traits


namespace dh::gst {

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

/**
 * @brief Universal Deleter for GStreamer objects.
 * This deleter is designed to handle various GStreamer types
 * and release resources correctly using appropriate unref functions.
 */
struct GstObjectDeleter
{
  /**
   * @brief Templated operator() to unref GStreamer objects.
   * @tparam T Type of GStreamer object.
   * @param obj Pointer to the GStreamer object.
   */
  template <typename T>
  void operator()(T* obj) const
  {
    if (!obj)
    {
      return;
    }

    if constexpr (IsGstObject<T>::value)
    {
      // Set GstElement state to NULL before unref if it's the last ref to the a GstElement
      if(GST_IS_ELEMENT(obj) && GST_OBJECT_REFCOUNT(obj) == 1)
      {
        gst_element_set_state(GST_ELEMENT(obj), GST_STATE_NULL);
      }

      gst_object_unref(GST_OBJECT(obj));
    }
    else
    {
      // Handle non-GObject types specifically
      if constexpr (std::is_same_v<T, GstCaps>)
      {
        gst_caps_unref(obj);
      } else if constexpr (std::is_same_v<T, GstStructure>)
      {
        gst_structure_free(obj);
      }
      else if constexpr(std::is_same_v<T, GstBuffer>)
      {
        gst_buffer_unref(obj);
      }
      else if constexpr (std::is_same_v<T, GstEvent>)
      {
        gst_event_unref(obj);
      }
      else if constexpr(std::is_same_v<T, GstMessage>)
      {
        gst_message_unref(obj);
      }
      else if constexpr(std::is_same_v<T, GstSample>)
      {
        gst_sample_unref(obj);
      }
      else
      {
        // Static assert for unhandled types to ensure all cases are covered
        static_assert(!sizeof(T*), "Unhandled GStreamer type in GstObjectDeleter");
      }
    }
  }
};

// Shared pointer typedefs for common GStreamer types
using GstObjectSPtr = std::shared_ptr<GstObject>;
using GstElementSPtr = std::shared_ptr<GstElement>;
using GstPadSPtr = std::shared_ptr<GstPad>;
using GstCapsSPtr = std::shared_ptr<GstCaps>;
using GstBufferSPtr = std::shared_ptr<GstBuffer>;
using GstEventSPtr = std::shared_ptr<GstEvent>;
using GstMessageSPtr = std::shared_ptr<GstMessage>;
using GstAppSinkSPtr = std::shared_ptr<GstAppSink>;
using GstAppSrcSPtr = std::shared_ptr<GstAppSrc>;
using GstBinSPtr = std::shared_ptr<GstBin>;
using GstBusSPtr = std::shared_ptr<GstBus>;
using GstClockSPtr = std::shared_ptr<GstClock>;
using GstDeviceMonitorSPtr = std::shared_ptr<GstDeviceMonitor>;
using GstDeviceSPtr = std::shared_ptr<GstDevice>;
using GstElementFactorySPtr = std::shared_ptr<GstElementFactory>;
using GstIteratorSPtr = std::shared_ptr<GstIterator>;
using GstPadTemplateSPtr = std::shared_ptr<GstPadTemplate>;
using GstPipelineSPtr = std::shared_ptr<GstPipeline>;
using GstPluginSPtr = std::shared_ptr<GstPlugin>;
using GstSampleSPtr = std::shared_ptr<GstSample>;
using GstStructureSPtr = std::shared_ptr<GstStructure>;
using GstPluginFeatureSPtr = std::shared_ptr<GstPluginFeature>;
/**
 * @brief Creates a shared_ptr for a GStreamer object with the correct deleter.
 * This function only participates in overload resolution if T is a known GStreamer type.
 * @tparam T Type of GStreamer object, must be or be typedef-compatible with known GStreamer types.
 * @param obj Raw pointer to the GStreamer object.
 * @param transferType Enum value indicating the ownership transfer type.
 * @return std::shared_ptr<T> A shared pointer to the GStreamer object with a custom deleter.
 */
template <typename T>
std::enable_if_t<IsGstObject<T>::value, std::shared_ptr<T>>
makeGstSharedPtr(T* obj, TransferType transferType)
{
  if(!obj)
  {
    return nullptr; // Handle null objects safely
  }

  switch(transferType)
  {
    case TransferType::Full:
      // Full ownership is transferred; nothing extra to do
      break;
    case TransferType::None:
      // No ownership is transferred; we need to increase the ref count
      gst_object_ref(GST_OBJECT(obj));
      break;
    case TransferType::Floating:
      // Handle floating references properly
      if (g_object_is_floating(G_OBJECT(obj)))
      {
        g_object_ref_sink(G_OBJECT(obj)); // Sink the floating reference
      }
      else
      {
        gst_object_ref(GST_OBJECT(obj));  // Increase ref count if not floating
      }
      break;
  }

  return std::shared_ptr<T>(obj, GstObjectDeleter());
}

/**
 * @brief Specialization of makeGstSharedPtr for non-GObject types like GstCaps.
 */
template <typename T>
std::enable_if_t<!IsGstObject<T>::value, std::shared_ptr<T>>
makeGstSharedPtr(T* obj, TransferType transferType)
{
  if(!obj)
  {
    return nullptr; // Handle null objects safely
  }

  if(transferType == TransferType::None)
  {
    // Increment reference manually for non-GObject types
    if constexpr(std::is_same_v<T, GstCaps>)
    {
      gst_caps_ref(obj);
    }
    // For GstStructure and similar types, manual reference management may vary
  }

  // Return a shared pointer with a custom deleter
  return std::shared_ptr<T>(obj, GstObjectDeleter());
}


// glib types
/**
 * @brief Universal Deleter for GLib objects such as GError, GList, and GHashTable.
 * This deleter ensures the correct freeing of GLib resources.
 */
struct GlibDeleter
{
  /**
   * @brief Templated operator() to free GLib objects.
   * @tparam T Type of the GLib object.
   * @param obj Pointer to the GLib object.
   */
  template <typename T>
  void operator()(T* obj) const
  {
    if (!obj)
    {
      return;
    }

    if constexpr (std::is_same_v<T, GError>)
    {
      g_error_free(obj);  // Free GError
    }
    else if constexpr (std::is_same_v<T, GList>)
    {
      g_list_free(obj);  // Free GList
    }
    else if constexpr (std::is_same_v<T, GHashTable>)
    {
      g_hash_table_destroy(obj);  // Destroy GHashTable
    }
    else
    {
      // Static assert for unhandled types to ensure all cases are covered
      static_assert(!sizeof(T*), "Unhandled GLib type in GlibDeleter");
    }
  }
};

// Shared pointer typedefs for common GLib types
using GErrorSPtr = std::shared_ptr<GError>;
using GListSPtr = std::shared_ptr<GList>;
using GHashTableSPtr = std::shared_ptr<GHashTable>;

/**
 * @brief Creates a shared_ptr for a GLib object with the correct deleter.
 * This function only participates in overload resolution if T is a known GLib type.
 * @tparam T Type of GLib object.
 * @param obj Raw pointer to the GLib object.
 * @return std::shared_ptr<T> A shared pointer to the GLib object with a custom deleter.
 * @todo: test
 */
template <typename T>
std::enable_if_t<std::is_same_v<T, GError> || std::is_same_v<T, GList> || std::is_same_v<T, GHashTable>, std::shared_ptr<T>>
makeGlibSharedPtr(T* obj, TransferType transferType = TransferType::Full)
{
  if (!obj)
  {
    return nullptr;  // Handle null objects safely
  }

  switch(transferType)
  {
    case TransferType::Full:
      // Full ownership is transferred; nothing extra to do
        break;
    case TransferType::None:
      // No ownership is transferred; we need to increase the ref count
      object_ref(obj);
      break;
    default:
      throw std::invalid_argument("makeGlibSharedPtr: Unsupoported transfer type for GObject");
  }
  return std::shared_ptr<T>(obj, GlibDeleter());
}
} // namespace dh::gst

#endif // DH_GST_SHAREDPTRS_HPP
