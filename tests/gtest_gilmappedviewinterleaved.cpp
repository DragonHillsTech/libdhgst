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

TEST_F(GilMappedViewInterleavedTest, CreateAndDeleteConstViewSucceeds)
{
  auto buffer = createTestBuffer();
  auto info = createTestVideoInfo();

  ASSERT_EQ(buffer.use_count(), 1);
  auto view = std::make_shared<dh::gst::GilMappedViewInterleaved<boost::gil::rgb8c_view_t>>(buffer, info);
  EXPECT_EQ(view->view.dimensions().x, 640);
  EXPECT_EQ(view->view.dimensions().y, 480);
  EXPECT_EQ(buffer.use_count(), 2);
  view.reset();
  EXPECT_EQ(buffer.use_count(), 1);
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

void setPixelColorInBuffer(GstBuffer* buffer, int x, int y, uint8_t r, uint8_t g, uint8_t b, int width = 640)
{
  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_WRITE);
  const int pixel_offset = (y * width + x) * 3;
  map.data[pixel_offset] = r;
  map.data[pixel_offset + 1] = g;
  map.data[pixel_offset + 2] = b;
  gst_buffer_unmap(buffer, &map);
}

TEST_F(GilMappedViewInterleavedTest, TestColorConsistencyWithConstView)
{
  auto buffer = createTestBuffer();
  auto info = createTestVideoInfo();

  const int test_x = 100;
  const int test_y = 100;
  const uint8_t test_r = 255;
  const uint8_t test_g = 128;
  const uint8_t test_b = 64;

  setPixelColorInBuffer(buffer.get(), test_x, test_y, test_r, test_g, test_b);

  auto view = std::make_shared<dh::gst::GilMappedViewInterleaved<boost::gil::rgb8c_view_t>>(buffer, info);
  auto pixel = *view->view.at(test_x, test_y);

  EXPECT_EQ(static_cast<uint8_t>(get_color(pixel, boost::gil::red_t())), test_r);
  EXPECT_EQ(static_cast<uint8_t>(get_color(pixel, boost::gil::green_t())), test_g);
  EXPECT_EQ(static_cast<uint8_t>(get_color(pixel, boost::gil::blue_t())), test_b);
}
