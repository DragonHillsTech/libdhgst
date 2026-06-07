/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
/**
 * @file test_gilview.cpp
 * @author Sandro Stiller
 * @date 2025-07-17
 */

#include "gilview.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>

#include <gst/gst.h>
#include <gst/video/video.h>

class GilViewTest
{
protected:
  GilViewTest()
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

BOOST_FIXTURE_TEST_CASE(CreateRgb8View, GilViewTest) {
  GstVideoInfo info;
  GstBuffer* buffer = createTestBuffer(info, GST_VIDEO_FORMAT_RGB, 4, 2);
  BOOST_REQUIRE_NE(buffer, nullptr);

  GstVideoFrame frame;
  gboolean mapped = gst_video_frame_map(&frame, &info, buffer, GST_MAP_READWRITE);
  BOOST_REQUIRE(mapped);

  fillBuffer(frame, 255);

  auto view = dh::gst::createGilView<boost::gil::rgb8_view_t>(frame);

  BOOST_CHECK_EQUAL(view.width(), 4);
  BOOST_CHECK_EQUAL(view.height(), 2);
  BOOST_CHECK_EQUAL(view(0, 0)[0], 255);
  BOOST_CHECK_EQUAL(view(0, 0)[1], 255);
  BOOST_CHECK_EQUAL(view(0, 0)[2], 255);

  gst_video_frame_unmap(&frame);
  gst_buffer_unref(buffer);
}

BOOST_FIXTURE_TEST_CASE(CreateGray8View, GilViewTest) {
  GstVideoInfo info;
  GstBuffer* buffer = createTestBuffer(info, GST_VIDEO_FORMAT_GRAY8, 2, 3);
  BOOST_REQUIRE_NE(buffer, nullptr);

  GstVideoFrame frame;
  gboolean mapped = gst_video_frame_map(&frame, &info, buffer, GST_MAP_READWRITE);
  BOOST_REQUIRE(mapped);

  fillBuffer(frame, 42);

  auto view = dh::gst::createGilView<boost::gil::gray8_view_t>(frame);
  BOOST_CHECK_EQUAL(view.width(), 2);
  BOOST_CHECK_EQUAL(view.height(), 3);
  BOOST_CHECK_EQUAL(view(1, 2), 42);

  gst_video_frame_unmap(&frame);
  gst_buffer_unref(buffer);
}

BOOST_FIXTURE_TEST_CASE(ThrowsOnWrongFormatRgb8, GilViewTest) {
  GstVideoInfo info;
  GstBuffer* buffer = createTestBuffer(info, GST_VIDEO_FORMAT_GRAY8, 1, 1); // Not RGB
  BOOST_REQUIRE_NE(buffer, nullptr);

  GstVideoFrame frame;
  gboolean mapped = gst_video_frame_map(&frame, &info, buffer, GST_MAP_READ);
  BOOST_REQUIRE(mapped);

  BOOST_CHECK_THROW(dh::gst::createGilView<boost::gil::rgb8_view_t>(frame), std::runtime_error);

  gst_video_frame_unmap(&frame);
  gst_buffer_unref(buffer);
}

BOOST_FIXTURE_TEST_CASE(ThrowsOnWrongFormatGray8, GilViewTest) {
  GstVideoInfo info;
  GstBuffer* buffer = createTestBuffer(info, GST_VIDEO_FORMAT_RGB, 1, 1); // Not GRAY
  BOOST_REQUIRE_NE(buffer, nullptr);

  GstVideoFrame frame;
  gboolean mapped = gst_video_frame_map(&frame, &info, buffer, GST_MAP_READ);
  BOOST_REQUIRE(mapped);

  BOOST_CHECK_THROW(dh::gst::createGilView<boost::gil::gray8_view_t>(frame), std::runtime_error);

  gst_video_frame_unmap(&frame);
  gst_buffer_unref(buffer);
}
