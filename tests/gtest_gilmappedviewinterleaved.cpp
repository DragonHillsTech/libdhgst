/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
/**
 * @file gtest_gilmappedviewinterleaved.cpp
 * @author Sandro Stiller
 * @date 2025-06-08
 */
#include "gilmappedviewinterleaved.hpp"

#include <gtest/gtest.h>
#include <gst/gst.h>
#include <gst/video/video.h>

class GilMappedViewInterleavedTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    gst_init(nullptr, nullptr);
  }

  dh::gst::GstBufferSPtr createTestBuffer(std::size_t size = 640 * 480 * 3)
  {
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
    return dh::gst::makeGstSharedPtr(buffer, dh::gst::TransferType::Full);
  }

  GstVideoInfo createTestVideoInfo(int width = 640, int height = 480)
  {
    GstVideoInfo info;
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_RGB, width, height);
    return info;
  }
};

TEST_F(GilMappedViewInterleavedTest, CreateConstViewSucceeds)
{
  auto buffer = createTestBuffer();
  auto info = createTestVideoInfo();

  ASSERT_EQ(buffer.use_count(), 1);
  dh::gst::GilMappedViewInterleaved<boost::gil::rgb8c_view_t> view{buffer, info};
  EXPECT_EQ(view.view.dimensions().x, 640);
  EXPECT_EQ(view.view.dimensions().y, 480);
  EXPECT_EQ(buffer.use_count(), 2);
}

TEST_F(GilMappedViewInterleavedTest, CreateWritableViewSucceeds)
{
  auto buffer = createTestBuffer();
  auto info = createTestVideoInfo();

  dh::gst::GilMappedViewInterleaved<boost::gil::rgb8_view_t> view{buffer, info};
  EXPECT_EQ(view.view.dimensions().x, 640);
  EXPECT_EQ(view.view.dimensions().y, 480);
}

TEST_F(GilMappedViewInterleavedTest, ThrowsOnNullBuffer)
{
  auto info = createTestVideoInfo();
  EXPECT_THROW(dh::gst::GilMappedViewInterleaved<boost::gil::rgb8_view_t>(nullptr, info), std::invalid_argument);
}
