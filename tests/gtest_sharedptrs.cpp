
#include "sharedptrs.hpp"
#include <gtest/gtest.h>

using namespace dh::gst;
// Test fixture for GStreamer tests
class GStreamerSharedPtrTest : public ::testing::Test {
protected:
  // Setup before first test case
  static void SetUpTestSuite()
  {
    gst_init(nullptr, nullptr);  // Initialize GStreamer
  }

  // Cleanup after last test case
  static void TearDownTestSuite()
  {
    gst_deinit();
  }
};

// Helper function to check reference count
int getRefCount(GstObject* obj) {
  return GST_OBJECT_REFCOUNT_VALUE(obj);
}

// Test makeGstSharedPtr with GstElement and TransferType::Full
TEST_F(GStreamerSharedPtrTest, MakeGstSharedPtr_GstElement_Full) {
  GstElement* rawSource = gst_element_factory_make("fakesrc", "source");
  ASSERT_NE(rawSource, nullptr) << "Failed to create element 'fakesrc'";

  int initialRefCount = getRefCount(GST_OBJECT(rawSource));

  // Create shared_ptr with TransferType::Full
  auto sourceSPtr = dh::gst::makeGstSharedPtr(rawSource, TransferType::Full);

  ASSERT_NE(sourceSPtr, nullptr) << "Shared pointer is null";

  // Ref count should remain the same because ownership was transferred fully
  EXPECT_EQ(getRefCount(GST_OBJECT(rawSource)), initialRefCount);

  // Shared pointer goes out of scope and unrefs the object
  sourceSPtr.reset();
  EXPECT_EQ(getRefCount(GST_OBJECT(rawSource)), 0);
}

// Test makeGstSharedPtr with GstPad and TransferType::None
TEST_F(GStreamerSharedPtrTest, MakeGstSharedPtr_GstPad_None) {
  GstPad* rawPad = gst_pad_new("sink", GST_PAD_SINK);
  ASSERT_NE(rawPad, nullptr) << "Failed to create pad 'sink'";

  int initialRefCount = getRefCount(GST_OBJECT(rawPad));

  // Create shared_ptr with TransferType::None
  auto padSPtr = dh::gst::makeGstSharedPtr(rawPad, TransferType::None);

  ASSERT_NE(padSPtr, nullptr) << "Shared pointer is null";

  // Ref count should increase by 1 since ownership was not transferred
  EXPECT_EQ(getRefCount(GST_OBJECT(rawPad)), initialRefCount + 1);

  // Shared pointer goes out of scope and unrefs the object
  padSPtr.reset();
  EXPECT_EQ(getRefCount(GST_OBJECT(rawPad)), initialRefCount);
  gst_object_unref(rawPad);  // Clean up the initial ref
}

// Test makeGstSharedPtr with GstCaps and TransferType::None
TEST_F(GStreamerSharedPtrTest, MakeGstSharedPtr_GstCaps_None) {
  GstCaps* rawCaps = gst_caps_new_empty_simple("video/x-raw");
  ASSERT_NE(rawCaps, nullptr) << "Failed to create caps";

  int initialRefCount = GST_CAPS_REFCOUNT_VALUE(rawCaps);

  // Create shared_ptr with TransferType::None
  auto capsSPtr = dh::gst::makeGstSharedPtr(rawCaps, TransferType::None);

  ASSERT_NE(capsSPtr, nullptr) << "Shared pointer is null";

  // Ref count should increase by 1 since ownership was not transferred
  EXPECT_EQ(GST_CAPS_REFCOUNT_VALUE(rawCaps), initialRefCount + 1);

  // Shared pointer goes out of scope and unrefs the object
  capsSPtr.reset();
  EXPECT_EQ(GST_CAPS_REFCOUNT_VALUE(rawCaps), initialRefCount);
  gst_caps_unref(rawCaps);  // Clean up the initial ref
}

// Test makeGstSharedPtr with GstAppSink and TransferType::None
TEST_F(GStreamerSharedPtrTest, MakeGstSharedPtr_GstAppSink_None) {
  GstElement* rawSink = gst_element_factory_make("appsink", "sink");
  ASSERT_NE(rawSink, nullptr) << "Failed to create element 'appsink'";

  GstAppSink* appSink = GST_APP_SINK(rawSink);
  int initialRefCount = getRefCount(GST_OBJECT(appSink));

  // Create shared_ptr with TransferType::None
  auto sinkSPtr = dh::gst::makeGstSharedPtr(appSink, TransferType::None);

  ASSERT_NE(sinkSPtr, nullptr) << "Shared pointer is null";

  // Ref count should increase by 1 since ownership was not transferred
  EXPECT_EQ(getRefCount(GST_OBJECT(appSink)), initialRefCount + 1);

  // Shared pointer goes out of scope and unrefs the object
  sinkSPtr.reset();
  EXPECT_EQ(getRefCount(GST_OBJECT(appSink)), initialRefCount);
  gst_object_unref(rawSink);  // Clean up the initial ref
}

// Test makeGstSharedPtr with GstBus and TransferType::Full
TEST_F(GStreamerSharedPtrTest, MakeGstSharedPtr_GstBus_Full) {
  GstElement* rawPipeline = gst_element_factory_make("pipeline", "pipeline");
  ASSERT_NE(rawPipeline, nullptr) << "Failed to create element 'pipeline'";

  GstBus* rawBus = gst_pipeline_get_bus(GST_PIPELINE(rawPipeline));
  ASSERT_NE(rawBus, nullptr) << "Failed to get bus from pipeline";

  int initialRefCount = getRefCount(GST_OBJECT(rawBus));

  // Create shared_ptr with TransferType::Full
  auto busSPtr = dh::gst::makeGstSharedPtr(rawBus, TransferType::Full);

  ASSERT_NE(busSPtr, nullptr) << "Shared pointer is null";

  // Ref count should remain the same because ownership was transferred fully
  EXPECT_EQ(getRefCount(GST_OBJECT(rawBus)), initialRefCount);

  // Shared pointer goes out of scope and unrefs the object
  busSPtr.reset();

  gst_object_unref(rawPipeline);  // Clean up the initial ref
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
