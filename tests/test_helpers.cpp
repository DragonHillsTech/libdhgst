/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
/**
 * @file test_helpers.cpp
 * @author Sandro Stiller
 * @date 2025-06-09
 */

#include "helpers.hpp"
#include "sharedptrs.hpp"

#include <gst/gst.h>
#include <gst/video/video.h>

#define BOOST_TEST_MODULE libdhgst_tests
#include "fatalgliblog.hpp"
#include <boost/test/included/unit_test.hpp>
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibCriticals);

class VideoInfoTest
{
protected:
  VideoInfoTest()
  {
    // Initialize GStreamer
    gst_init(nullptr, nullptr);
  }
};

BOOST_FIXTURE_TEST_CASE(CreateVideoInfoFromValidCaps, VideoInfoTest)
{
  // Create valid video caps
  dh::gst::GstCapsSPtr caps = dh::gst::makeGstSharedPtr(
    gst_caps_new_simple(
      "video/x-raw",
      "format",
      G_TYPE_STRING,
      "RGB",
      "width",
      G_TYPE_INT,
      1920,
      "height",
      G_TYPE_INT,
      1080,
      "framerate",
      GST_TYPE_FRACTION,
      30,
      1,
      nullptr
    ),
    dh::gst::TransferType::full
  );

  // Test successful creation
  BOOST_REQUIRE_NO_THROW({
    GstVideoInfo vinfo = dh::gst::helpers::createVideoInfo(*caps);

    // Verify the video info properties
    BOOST_CHECK_EQUAL(GST_VIDEO_INFO_WIDTH(&vinfo), 1920);
    BOOST_CHECK_EQUAL(GST_VIDEO_INFO_HEIGHT(&vinfo), 1080);
    BOOST_CHECK_EQUAL(GST_VIDEO_INFO_FPS_N(&vinfo), 30);
    BOOST_CHECK_EQUAL(GST_VIDEO_INFO_FPS_D(&vinfo), 1);
    BOOST_CHECK_EQUAL(
      std::string(gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&vinfo))),
      "RGB"
    );
  });
}

BOOST_FIXTURE_TEST_CASE(CreateVideoInfoFromInvalidCaps, VideoInfoTest)
{
  // Create invalid video caps (missing required fields)
  dh::gst::GstCapsSPtr invalidCaps = dh::gst::makeGstSharedPtr(
    gst_caps_new_simple(
      "video/x-raw",
      "format",
      G_TYPE_STRING,
      "RGB",
      // Missing width and height
      nullptr
    ),
    dh::gst::TransferType::full
  );

  BOOST_CHECK_THROW(dh::gst::helpers::createVideoInfo(*invalidCaps), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(CreateVideoInfoFromNonVideoCaps, VideoInfoTest)
{
  // Create non-video caps
  dh::gst::GstCapsSPtr audioCaps = dh::gst::makeGstSharedPtr(
    gst_caps_new_simple(
      "audio/x-raw",
      "format",
      G_TYPE_STRING,
      "S16LE",
      "rate",
      G_TYPE_INT,
      44100,
      "channels",
      G_TYPE_INT,
      2,
      nullptr
    ),
    dh::gst::TransferType::full
  );

  BOOST_CHECK_THROW(dh::gst::helpers::createVideoInfo(*audioCaps), std::runtime_error);
}

// --- createVideoInfo(const GstBuffer&) ---

BOOST_FIXTURE_TEST_CASE(CreateVideoInfoFromBufferWithPaddedLayout, VideoInfoTest)
{
  // I420 320x240 with padded rows: the layout in the meta differs from the default layout of the
  // format
  const guint width = 320;
  const guint height = 240;
  gsize offsets[GST_VIDEO_MAX_PLANES] = {0, 384 * 240, 384 * 240 + 192 * 120};
  gint strides[GST_VIDEO_MAX_PLANES] = {384, 192, 192};
  const gsize size = 384 * 240 + 2 * 192 * 120;

  auto buffer = dh::gst::makeGstSharedPtr(
    gst_buffer_new_allocate(nullptr, size, nullptr),
    dh::gst::TransferType::full
  );
  BOOST_REQUIRE(gst_buffer_add_video_meta_full(
    buffer.get(),
    GST_VIDEO_FRAME_FLAG_NONE,
    GST_VIDEO_FORMAT_I420,
    width,
    height,
    3,
    offsets,
    strides
  ));

  const GstVideoInfo vinfo = dh::gst::helpers::createVideoInfo(*buffer);

  BOOST_CHECK_EQUAL(GST_VIDEO_INFO_FORMAT(&vinfo), GST_VIDEO_FORMAT_I420);
  BOOST_CHECK_EQUAL(GST_VIDEO_INFO_WIDTH(&vinfo), static_cast<gint>(width));
  BOOST_CHECK_EQUAL(GST_VIDEO_INFO_HEIGHT(&vinfo), static_cast<gint>(height));
  BOOST_REQUIRE_EQUAL(GST_VIDEO_INFO_N_PLANES(&vinfo), 3U);
  for (guint plane = 0; plane < 3; ++plane)
  {
    BOOST_CHECK_EQUAL(GST_VIDEO_INFO_PLANE_STRIDE(&vinfo, plane), strides[plane]);
    BOOST_CHECK_EQUAL(GST_VIDEO_INFO_PLANE_OFFSET(&vinfo, plane), offsets[plane]);
  }
  BOOST_CHECK_EQUAL(GST_VIDEO_INFO_SIZE(&vinfo), size);

  // the info must be usable to map the buffer, also without relying on the meta
  GstVideoFrame frame;
  BOOST_REQUIRE(gst_video_frame_map(&frame, &vinfo, buffer.get(), GST_MAP_READ));
  BOOST_CHECK_EQUAL(GST_VIDEO_FRAME_PLANE_STRIDE(&frame, 1), 192);
  gst_video_frame_unmap(&frame);
}

BOOST_FIXTURE_TEST_CASE(CreateVideoInfoFromBufferWithDefaultLayout, VideoInfoTest)
{
  // a meta with the default layout gives the same info as the format itself; odd sizes give padded
  // rows, YV12 stores its planes in a different order than their index (V before U)
  for (const GstVideoFormat format :
       {GST_VIDEO_FORMAT_RGB, GST_VIDEO_FORMAT_I420, GST_VIDEO_FORMAT_YV12, GST_VIDEO_FORMAT_NV12})
  {
    BOOST_TEST_CONTEXT("format " << gst_video_format_to_string(format))
    {
      GstVideoInfo expected;
      BOOST_REQUIRE(gst_video_info_set_format(&expected, format, 33, 17));

      auto buffer = dh::gst::makeGstSharedPtr(
        gst_buffer_new_allocate(nullptr, GST_VIDEO_INFO_SIZE(&expected), nullptr),
        dh::gst::TransferType::full
      );
      BOOST_REQUIRE(
        gst_buffer_add_video_meta(buffer.get(), GST_VIDEO_FRAME_FLAG_NONE, format, 33, 17)
      );

      const GstVideoInfo vinfo = dh::gst::helpers::createVideoInfo(*buffer);
      BOOST_REQUIRE_EQUAL(GST_VIDEO_INFO_N_PLANES(&vinfo), GST_VIDEO_INFO_N_PLANES(&expected));
      for (guint plane = 0; plane < GST_VIDEO_INFO_N_PLANES(&vinfo); ++plane)
      {
        BOOST_CHECK_EQUAL(
          GST_VIDEO_INFO_PLANE_STRIDE(&vinfo, plane),
          GST_VIDEO_INFO_PLANE_STRIDE(&expected, plane)
        );
        BOOST_CHECK_EQUAL(
          GST_VIDEO_INFO_PLANE_OFFSET(&vinfo, plane),
          GST_VIDEO_INFO_PLANE_OFFSET(&expected, plane)
        );
      }
      BOOST_CHECK_EQUAL(GST_VIDEO_INFO_SIZE(&vinfo), GST_VIDEO_INFO_SIZE(&expected));
    }
  }
}

BOOST_FIXTURE_TEST_CASE(CreateVideoInfoFromBufferWithoutMetaThrows, VideoInfoTest)
{
  auto buffer = dh::gst::makeGstSharedPtr(
    gst_buffer_new_allocate(nullptr, 16, nullptr),
    dh::gst::TransferType::full
  );
  BOOST_CHECK_THROW(dh::gst::helpers::createVideoInfo(*buffer), std::runtime_error);
}
