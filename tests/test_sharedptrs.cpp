/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */

#include "sharedptrs.hpp"
#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>

using namespace dh::gst;
// Test fixture for GStreamer tests
class GStreamerSharedPtrTest {
protected:
  // Setup before first test case
  GStreamerSharedPtrTest()
  {
    gst_init(nullptr, nullptr);  // Initialize GStreamer
  }
};

// Helper function to check reference count
int getRefCount(GstObject* obj) {
  return GST_OBJECT_REFCOUNT_VALUE(obj);
}

// Test makeGstSharedPtr with GstElement and TransferType::Full
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstElement_Full, GStreamerSharedPtrTest) {
  GstElement* rawSource = gst_element_factory_make("fakesrc", "source");
  BOOST_REQUIRE_NE(rawSource, nullptr);

  int initialRefCount = getRefCount(GST_OBJECT(rawSource));

  // Create shared_ptr with TransferType::Full
  auto sourceSPtr = dh::gst::makeGstSharedPtr(rawSource, TransferType::Full);

  BOOST_REQUIRE_NE(sourceSPtr, nullptr);

  // Ref count should remain the same because ownership was transferred fully
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(rawSource)), initialRefCount);

  // Shared pointer goes out of scope and unrefs the object
  sourceSPtr.reset();
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(rawSource)), 0);
}

// Test makeGstSharedPtr with GstPad and TransferType::None
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstPad_None, GStreamerSharedPtrTest) {
  GstPad* rawPad = gst_pad_new("sink", GST_PAD_SINK);
  BOOST_REQUIRE_NE(rawPad, nullptr);

  int initialRefCount = getRefCount(GST_OBJECT(rawPad));

  // Create shared_ptr with TransferType::None
  auto padSPtr = dh::gst::makeGstSharedPtr(rawPad, TransferType::None);

  BOOST_REQUIRE_NE(padSPtr, nullptr);

  // Ref count should increase by 1 since ownership was not transferred
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(rawPad)), initialRefCount + 1);

  // Shared pointer goes out of scope and unrefs the object
  padSPtr.reset();
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(rawPad)), initialRefCount);
  gst_object_unref(rawPad);  // Clean up the initial ref
}

// Test makeGstSharedPtr with GstCaps and TransferType::None
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstCaps_None, GStreamerSharedPtrTest) {
  GstCaps* rawCaps = gst_caps_new_empty_simple("video/x-raw");
  BOOST_REQUIRE_NE(rawCaps, nullptr);

  int initialRefCount = GST_CAPS_REFCOUNT_VALUE(rawCaps);

  // Create shared_ptr with TransferType::None
  auto capsSPtr = dh::gst::makeGstSharedPtr(rawCaps, TransferType::None);

  BOOST_REQUIRE_NE(capsSPtr, nullptr);

  // Ref count should increase by 1 since ownership was not transferred
  BOOST_CHECK_EQUAL(GST_CAPS_REFCOUNT_VALUE(rawCaps), initialRefCount + 1);

  // Shared pointer goes out of scope and unrefs the object
  capsSPtr.reset();
  BOOST_CHECK_EQUAL(GST_CAPS_REFCOUNT_VALUE(rawCaps), initialRefCount);
  gst_caps_unref(rawCaps);  // Clean up the initial ref
}

// Test makeGstSharedPtr with GstAppSink and TransferType::None
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstAppSink_None, GStreamerSharedPtrTest) {
  GstElement* rawSink = gst_element_factory_make("appsink", "sink");
  BOOST_REQUIRE_NE(rawSink, nullptr);

  GstAppSink* appSink = GST_APP_SINK(rawSink);
  int initialRefCount = getRefCount(GST_OBJECT(appSink));

  // Create shared_ptr with TransferType::None
  auto sinkSPtr = dh::gst::makeGstSharedPtr(appSink, TransferType::None);

  BOOST_REQUIRE_NE(sinkSPtr, nullptr);

  // Ref count should increase by 1 since ownership was not transferred
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(appSink)), initialRefCount + 1);

  // Shared pointer goes out of scope and unrefs the object
  sinkSPtr.reset();
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(appSink)), initialRefCount);
  gst_object_unref(rawSink);  // Clean up the initial ref
}

// Test makeGstSharedPtr with GstBus and TransferType::Full
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstBus_Full, GStreamerSharedPtrTest) {
  GstElement* rawPipeline = gst_element_factory_make("pipeline", "pipeline");
  BOOST_REQUIRE_NE(rawPipeline, nullptr);

  GstBus* rawBus = gst_pipeline_get_bus(GST_PIPELINE(rawPipeline));
  BOOST_REQUIRE_NE(rawBus, nullptr);

  int initialRefCount = getRefCount(GST_OBJECT(rawBus));

  // Create shared_ptr with TransferType::Full
  auto busSPtr = dh::gst::makeGstSharedPtr(rawBus, TransferType::Full);

  BOOST_REQUIRE_NE(busSPtr, nullptr);

  // Ref count should remain the same because ownership was transferred fully
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(rawBus)), initialRefCount);

  // Shared pointer goes out of scope and unrefs the object
  busSPtr.reset();

  gst_object_unref(rawPipeline);  // Clean up the initial ref
}

// Test makeGstSharedPtr with GstBuffer and TransferType::None
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstBuffer_None, GStreamerSharedPtrTest)
{
  GstBuffer* rawBuffer = gst_buffer_new();
  BOOST_REQUIRE_NE(rawBuffer, nullptr);

  int initialRefCount = GST_MINI_OBJECT_REFCOUNT_VALUE(rawBuffer);

  // Create shared_ptr with TransferType::None
  auto bufferSPtr = dh::gst::makeGstSharedPtr(rawBuffer, TransferType::None);

  BOOST_REQUIRE_NE(bufferSPtr, nullptr);

  // Ref count should increase by 1 since ownership was not transferred
  BOOST_CHECK_EQUAL(GST_MINI_OBJECT_REFCOUNT_VALUE(rawBuffer), initialRefCount + 1);

  // Shared pointer goes out of scope and unrefs the object
  bufferSPtr.reset();
  BOOST_CHECK_EQUAL(GST_MINI_OBJECT_REFCOUNT_VALUE(rawBuffer), initialRefCount);
  gst_buffer_unref(rawBuffer); // Clean up the initial ref
}

