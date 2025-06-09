/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
/**
 * @file gilmappedviewinterleaved.hpp
 * @author Sandro Stiller
 * @date 2025-06-08
 */
#ifndef DH_GST_GILMAPPEDVIEWINTERLEAVED_HPP
#define DH_GST_GILMAPPEDVIEWINTERLEAVED_HPP

#include "sharedptrs.hpp"
#include  "helpers.hpp"

#include <boost/gil.hpp>

#include <stdexcept>
#include <gst/gst.h>
#include <gst/video/video.h>  // for GstVideoFrame, GstVideoInfo

namespace dh::gst
{

/**
 * @brief Trait to determine if a GIL view's pixel type is const.
 *
 * This trait helps deduce whether the GIL view implies read-only or writable access.
 */
template <typename T>
using is_const_pixel = std::is_const<typename boost::gil::channel_type<typename T::value_type>::type>;

/**
 * @brief A RAII wrapper that holds a mapped GstBuffer and a corresponding Boost.GIL view.
 *
 * This structure ensures the GstBuffer remains mapped as long as the view is used.
 *
 * @tparam ViewType The type of the Boost.GIL view.
 */
template <typename ViewType>
class GilMappedViewInterleaved
{
public:
  GstBufferSPtr buffer;     ///< The shared pointer to the GstBuffer
  GstMapInfo map;           ///< The mapping info @todo: make private
  ViewType view;            ///< The GIL view

  GilMappedViewInterleaved(GstBufferSPtr buffer_, const GstVideoInfo& vinfo)
  {
    if(!buffer_)
    {
      throw std::invalid_argument("GilMappedViewInterleaved: empty GstBuffer");
    }

    constexpr bool viewIsWritable = !is_const_pixel<ViewType>::value;

    GstBuffer* gstBuffer = buffer_.get();
    if(viewIsWritable && ! gst_buffer_is_writable(gstBuffer))
    {
      const bool incBufferUseCount = (buffer_.use_count() > 1);
      if(incBufferUseCount)
      {
        // Temporarily increment the GstBuffer refcount to force gst_buffer_make_writable()
        // to return a new copy if the GstBuffer ref count is 1. This avoids mutating a buffer that might still be shared
        // through other shared_ptr instances, which could otherwise lead to unintended memory changes.
        gst_buffer_ref(gstBuffer);
      }

      GstBuffer* writableBuf = gst_buffer_make_writable(gstBuffer);

      if(incBufferUseCount)
      {
        gst_buffer_unref(gstBuffer);
      }

      if(!writableBuf)
      {
        throw std::runtime_error("Failed to make GstBuffer writable");
      }
      gstBuffer = writableBuf;
      buffer = makeGstSharedPtr(gstBuffer, TransferType::Full);
    }
    else
    {
      buffer = std::move(buffer_);
    }

    const GstMapFlags mapFlags = viewIsWritable ? GST_MAP_WRITE : GST_MAP_READ;
    if(!gst_buffer_map(gstBuffer, &map, mapFlags))
    {
      throw std::runtime_error("Failed to map GstBuffer");
    }

    // using pixel_t = typename ViewType::value_type;
    auto width = vinfo.width;
    auto height = vinfo.height;
    auto stride = static_cast<std::ptrdiff_t>(GST_VIDEO_INFO_PLANE_STRIDE(&vinfo, 0));

    auto* data_ptr = map.data;
    view = boost::gil::interleaved_view(
      width,
      height,
      reinterpret_cast<typename ViewType::x_iterator>(data_ptr),
      stride
    );
  }

  GilMappedViewInterleaved(GstBufferSPtr buffer_)
  : GilMappedViewInterleaved(buffer_, helpers::createVideoInfo(*buffer_))
  {
  }


  ~GilMappedViewInterleaved()
  {
    if(map.data)
    {
      gst_buffer_unmap(buffer.get(), &map);
    }
  }
};

} // dh::gst


#endif // DH_GST_GILMAPPEDVIEWINTERLEAVED_HPP
