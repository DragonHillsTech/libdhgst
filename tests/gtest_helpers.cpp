/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
/**
 * @file gtest_helpers.cpp
 * @author Sandro Stiller
 * @date 2025-06-09
 */

#include "sharedptrs.hpp"

#include <gst/gst.h>
#include <gst/video/video.h>

#include <gtest/gtest.h>

class VideoInfoTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Initialize GStreamer
    gst_init(nullptr, nullptr);
  }

  void TearDown() override
  {
    // Cleanup if needed
  }
};

TEST_F(VideoInfoTest, CreateVideoInfoFromValidCaps)
{
  // Create valid video caps
  GstCapsSPtr caps = makeGstSharedPtr(
    gst_caps_new_simple(
      "video/x-raw",
      "format", G_TYPE_STRING, "RGB",
      "width", G_TYPE_INT, 1920,
      "height", G_TYPE_INT, 1080,
      "framerate", GST_TYPE_FRACTION, 30, 1,
      nullptr),
    TransferType::Full
  );

  // Test successful creation
  ASSERT_NO_THROW(
    {
      GstVideoInfo vinfo = createVideoInfo(caps);

      // Verify the video info properties
      EXPECT_EQ(GST_VIDEO_INFO_WIDTH(&vinfo), 1920);
      EXPECT_EQ(GST_VIDEO_INFO_HEIGHT(&vinfo), 1080);
      EXPECT_EQ(GST_VIDEO_INFO_FPS_N(&vinfo), 30);
      EXPECT_EQ(GST_VIDEO_INFO_FPS_D(&vinfo), 1);
      EXPECT_EQ(std::string(GST_VIDEO_INFO_FORMAT_STRING(&vinfo)), "RGB");
    }
  );
}

TEST_F(VideoInfoTest, CreateVideoInfoFromNullCaps)
{
  // Test with null caps
  GstCapsSPtr nullCaps;
  EXPECT_THROW(createVideoInfo(nullCaps), std::invalid_argument);
}

TEST_F(VideoInfoTest, CreateVideoInfoFromInvalidCaps)
{
  // Create invalid video caps (missing required fields)
  GstCapsSPtr invalidCaps = makeGstSharedPtr(
    gst_caps_new_simple("video/x-raw",
      "format", G_TYPE_STRING, "RGB",
      // Missing width and height
      nullptr),
    TransferType::Full
  );

  EXPECT_THROW(createVideoInfo(invalidCaps), std::runtime_error);
}

TEST_F(VideoInfoTest, CreateVideoInfoFromNonVideoCaps)
{
  // Create non-video caps
  GstCapsSPtr audioCaps = makeGstSharedPtr(
    gst_caps_new_simple(
      "audio/x-raw",
      "format", G_TYPE_STRING, "S16LE",
      "rate", G_TYPE_INT, 44100,
      "channels", G_TYPE_INT, 2,
      nullptr),
    TransferType::Full
  );

  EXPECT_THROW(createVideoInfo(audioCaps), std::runtime_error);
}