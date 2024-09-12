/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */

#ifndef DH_GST_SHAREDPTRS_HPP
#define DH_GST_SHAREDPTRS_HPP

#include "transfertype.hpp"            // Include the external header for TransferType

#include <gst/app/gstappsink.h>      // For GstAppSink
#include <gst/app/gstappsrc.h>       // For GstAppSrc
#include <gst/gst.h>                 // General GStreamer types and functions

#include <memory>                    // For std::shared_ptr
#include <type_traits>               // For std::type_traits


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
    if constexpr (IsGstObject<T>::value)
    {
      // For GObject-derived types
      if(obj)
      {
        gst_object_unref(GST_OBJECT(obj));
      }
    }
    else
    {
      // Handle non-GObject types specifically
      if constexpr (std::is_same_v<T, GstCaps>)
      {
        if(obj)
        {
          gst_caps_unref(obj);
        }
      } else if constexpr (std::is_same_v<T, GstStructure>)
      {
        if(obj)
        {
          gst_structure_free(obj);
        }
      }
      else if constexpr(std::is_same_v<T, GstBuffer>)
      {
        if (obj)
        {
          gst_buffer_unref(obj);
        }
      }
      else if constexpr (std::is_same_v<T, GstEvent>)
      {
        if(obj)
        {
          gst_event_unref(obj);
        }
      }
      else if constexpr(std::is_same_v<T, GstMessage>)
      {
        if(obj)
        {
          gst_message_unref(obj);
        }
      }
      else if constexpr(std::is_same_v<T, GstSample>)
      {
        if(obj)
        {
          gst_sample_unref(obj);
        }
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
makeGstSharedPtr(T* obj, TransferType transferType = TransferType::Full)
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
makeGstSharedPtr(T* obj, TransferType transferType = TransferType::Full)
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


} // namespace dh::gst

#endif // DH_GST_SHAREDPTRS_HPP
