/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
/**
 * @file gtest_gilmappedviewinterleaved.cpp
 * @author Sandro Stiller
 * @date 2025-07-17
 */

#include "gilview.hpp"

#include <gtest/gtest.h>

#include <gst/gst.h>
#include <gst/video/video.h>

class GilViewTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    gst_init(nullptr, nullptr);
  }
  static GstBuffer* createTestBuffer(GstVideoInfo& info, GstVideoFormat format, int width, int height)
  {
    gst_video_info_set_format(&info, format, width, height);
    return gst_buffer_new_allocate(nullptr, info.size, nullptr);
  }

  static void fillBuffer(GstVideoFrame& frame, uint8_t value)
  {
    uint8_t* data = static_cast<uint8_t*>(GST_VIDEO_FRAME_PLANE_DATA(&frame, 0));
    std::fill_n(data, GST_VIDEO_FRAME_SIZE(&frame), value);
  }
};

TEST_F(GilViewTest, CreateRgb8View) {
  GstVideoInfo info;
  GstBuffer* buffer = createTestBuffer(info, GST_VIDEO_FORMAT_RGB, 4, 2);
  ASSERT_NE(buffer, nullptr);

  GstVideoFrame frame;
  gboolean mapped = gst_video_frame_map(&frame, &info, buffer, GST_MAP_READWRITE);
  ASSERT_TRUE(mapped);

  fillBuffer(frame, 255);

  auto view = dh::gst::createGilView<boost::gil::rgb8_view_t>(frame);

  EXPECT_EQ(view.width(), 4);
  EXPECT_EQ(view.height(), 2);
  EXPECT_EQ(view(0, 0)[0], 255);
  EXPECT_EQ(view(0, 0)[1], 255);
  EXPECT_EQ(view(0, 0)[2], 255);

  gst_video_frame_unmap(&frame);
  gst_buffer_unref(buffer);
}

TEST_F(GilViewTest, CreateGray8View) {
  GstVideoInfo info;
  GstBuffer* buffer = createTestBuffer(info, GST_VIDEO_FORMAT_GRAY8, 2, 3);
  ASSERT_NE(buffer, nullptr);

  GstVideoFrame frame;
  gboolean mapped = gst_video_frame_map(&frame, &info, buffer, GST_MAP_READWRITE);
  ASSERT_TRUE(mapped);

  fillBuffer(frame, 42);

  auto view = dh::gst::createGilView<boost::gil::gray8_view_t>(frame);
  EXPECT_EQ(view.width(), 2);
  EXPECT_EQ(view.height(), 3);
  EXPECT_EQ(view(1, 2), 42);

  gst_video_frame_unmap(&frame);
  gst_buffer_unref(buffer);
}

TEST_F(GilViewTest, ThrowsOnWrongFormatRgb8) {
  GstVideoInfo info;
  GstBuffer* buffer = createTestBuffer(info, GST_VIDEO_FORMAT_GRAY8, 1, 1); // Not RGB
  ASSERT_NE(buffer, nullptr);

  GstVideoFrame frame;
  gboolean mapped = gst_video_frame_map(&frame, &info, buffer, GST_MAP_READ);
  ASSERT_TRUE(mapped);

  EXPECT_THROW(dh::gst::createGilView<boost::gil::rgb8_view_t>(frame), std::runtime_error);

  gst_video_frame_unmap(&frame);
  gst_buffer_unref(buffer);
}

TEST_F(GilViewTest, ThrowsOnWrongFormatGray8) {
  GstVideoInfo info;
  GstBuffer* buffer = createTestBuffer(info, GST_VIDEO_FORMAT_RGB, 1, 1); // Not GRAY
  ASSERT_NE(buffer, nullptr);

  GstVideoFrame frame;
  gboolean mapped = gst_video_frame_map(&frame, &info, buffer, GST_MAP_READ);
  ASSERT_TRUE(mapped);

  EXPECT_THROW(dh::gst::createGilView<boost::gil::gray8_view_t>(frame), std::runtime_error);

  gst_video_frame_unmap(&frame);
  gst_buffer_unref(buffer);
}
