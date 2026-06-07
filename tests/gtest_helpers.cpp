/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
/**
 * @file gtest_helpers.cpp
 * @author Sandro Stiller
 * @date 2025-06-09
 */

#include "sharedptrs.hpp"
#include "helpers.hpp"

#include <gst/gst.h>
#include <gst/video/video.h>

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>

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
      "format", G_TYPE_STRING, "RGB",
      "width", G_TYPE_INT, 1920,
      "height", G_TYPE_INT, 1080,
      "framerate", GST_TYPE_FRACTION, 30, 1,
      nullptr),
    dh::gst::TransferType::Full
  );

  // Test successful creation
  BOOST_REQUIRE_NO_THROW(
    {
      GstVideoInfo vinfo = dh::gst::helpers::createVideoInfo(*caps);

      // Verify the video info properties
      BOOST_CHECK_EQUAL(GST_VIDEO_INFO_WIDTH(&vinfo), 1920);
      BOOST_CHECK_EQUAL(GST_VIDEO_INFO_HEIGHT(&vinfo), 1080);
      BOOST_CHECK_EQUAL(GST_VIDEO_INFO_FPS_N(&vinfo), 30);
      BOOST_CHECK_EQUAL(GST_VIDEO_INFO_FPS_D(&vinfo), 1);
      BOOST_CHECK_EQUAL(std::string(gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&vinfo))), "RGB");
    }
  );
}

BOOST_FIXTURE_TEST_CASE(CreateVideoInfoFromInvalidCaps, VideoInfoTest)
{
  // Create invalid video caps (missing required fields)
  dh::gst::GstCapsSPtr invalidCaps = dh::gst::makeGstSharedPtr(
    gst_caps_new_simple("video/x-raw",
      "format", G_TYPE_STRING, "RGB",
      // Missing width and height
      nullptr),
    dh::gst::TransferType::Full
  );

  BOOST_CHECK_THROW(dh::gst::helpers::createVideoInfo(*invalidCaps), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(CreateVideoInfoFromNonVideoCaps, VideoInfoTest)
{
  // Create non-video caps
  dh::gst::GstCapsSPtr audioCaps = dh::gst::makeGstSharedPtr(
    gst_caps_new_simple(
      "audio/x-raw",
      "format", G_TYPE_STRING, "S16LE",
      "rate", G_TYPE_INT, 44100,
      "channels", G_TYPE_INT, 2,
      nullptr),
    dh::gst::TransferType::Full
  );

  BOOST_CHECK_THROW(dh::gst::helpers::createVideoInfo(*audioCaps), std::runtime_error);
}

