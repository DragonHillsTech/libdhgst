/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
/**
* @file gilmappedviewinterleaved.hpp
 * @author Sandro Stiller
 * @date 2025-07-06
 */

#ifndef DH_GST_GILVIEW_HPP
#define DH_GST_GILVIEW_HPP

#include <boost/gil.hpp>
#include <gst/gst.h>
#include <gst/video/video.h>  // for GstVideoFrame, GstVideoInfo

namespace dh::gst
{
/* Remove? First test if the
template <typename ViewType>
using is_const_pixel = std::is_const<typename std::remove_reference_t<
  decltype(*std::declval<typename ViewType::x_iterator>())
>>;
*/


/**
 * @brief Create a Boost.GIL view directly from a mapped GstVideoFrame.
 *
 * @tparam ViewType   Must be an interleaved view whose pixel_t is boost::gil::rgb8_pixel_t or gray8_pixel_t.
 * @param  frame      A GstVideoFrame that has already been mapped.
 * @return The Boost.GIL view over frame's plane 0.
 * @throws std::runtime_error if the GStreamer format isn't compatible.
 */
template <typename ViewType>
ViewType createGilView(GstVideoFrame& frame)
{
  using pixel_t         = typename ViewType::value_type;
  using non_const_pix_t = std::remove_const_t<pixel_t>;

  // 1) Ensure only RGB8 or GRAY8
  if constexpr (std::is_same_v<non_const_pix_t, boost::gil::rgb8_pixel_t>)
  {
    if(GST_VIDEO_INFO_FORMAT(&frame.info) != GST_VIDEO_FORMAT_RGB)
    {
      throw std::runtime_error("createGilView: frame is not RGB8");
    }
  }
  else if constexpr (std::is_same_v<non_const_pix_t, boost::gil::gray8_pixel_t>)
  {
    if (GST_VIDEO_INFO_FORMAT(&frame.info) != GST_VIDEO_FORMAT_GRAY8)
    {
      throw std::runtime_error("createGilView: frame is not GRAY8");
    }
  }
  else
  {
    static_assert(!std::is_same_v<non_const_pix_t, non_const_pix_t>,
                  "createGilView only supports RGB8 or GRAY8 pixels");
  }

  // 2) Dimensions, data pointer, and byte-stride
  const int            width        = frame.info.width;
  const int            height       = frame.info.height;
  auto*                base         = reinterpret_cast<pixel_t*>(GST_VIDEO_FRAME_PLANE_DATA(&frame, 0));
  const std::ptrdiff_t stride_bytes = GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 0);

  // 3) Return interleaved view (rowsize in bytes)
  return boost::gil::interleaved_view(width, height, base, stride_bytes);
}
} // dh::gst

#endif //DH_GST_GILVIEW_HPP
