/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */

#include "sharedptrs.hpp"
#define BOOST_TEST_MODULE libdhgst_tests
#include "fatalgliblog.hpp"
#include <boost/test/included/unit_test.hpp>
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibCriticals);

#include <stdexcept>
#include <string>

using namespace dh::gst;
// Test fixture for GStreamer tests
class GStreamerSharedPtrTest
{
protected:
  // Setup before first test case
  GStreamerSharedPtrTest()
  {
    gst_init(nullptr, nullptr); // Initialize GStreamer
  }
};

// Helper function to check reference count
int getRefCount(GstObject* obj)
{
  return GST_OBJECT_REFCOUNT_VALUE(obj);
}

// Test makeGstSharedPtr with GstElement and TransferType::full
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstElement_Full, GStreamerSharedPtrTest)
{
  GstElement* rawSource = gst_element_factory_make("fakesrc", "source");
  BOOST_REQUIRE_NE(rawSource, nullptr);

  int initialRefCount = getRefCount(GST_OBJECT(rawSource));

  // Create shared_ptr with TransferType::full
  auto sourceSPtr = dh::gst::makeGstSharedPtr(rawSource, TransferType::full);

  BOOST_REQUIRE_NE(sourceSPtr, nullptr);

  // Ref count should remain the same because ownership was transferred fully
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(rawSource)), initialRefCount);

  // Shared pointer goes out of scope and unrefs the object. It held the only reference,
  // so the object is destroyed: observe that with a weak pointer instead of reading freed memory.
  gpointer weakSource = rawSource;
  g_object_add_weak_pointer(G_OBJECT(rawSource), &weakSource);
  sourceSPtr.reset();
  BOOST_CHECK(weakSource == nullptr);
}

// Test makeGstSharedPtr with GstPad and TransferType::none
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstPad_None, GStreamerSharedPtrTest)
{
  GstPad* rawPad = gst_pad_new("sink", GST_PAD_SINK);
  BOOST_REQUIRE_NE(rawPad, nullptr);

  int initialRefCount = getRefCount(GST_OBJECT(rawPad));

  // Create shared_ptr with TransferType::none
  auto padSPtr = dh::gst::makeGstSharedPtr(rawPad, TransferType::none);

  BOOST_REQUIRE_NE(padSPtr, nullptr);

  // Ref count should increase by 1 since ownership was not transferred
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(rawPad)), initialRefCount + 1);

  // Shared pointer goes out of scope and unrefs the object
  padSPtr.reset();
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(rawPad)), initialRefCount);
  gst_object_unref(rawPad); // Clean up the initial ref
}

// Test makeGstSharedPtr with GstCaps and TransferType::none
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstCaps_None, GStreamerSharedPtrTest)
{
  GstCaps* rawCaps = gst_caps_new_empty_simple("video/x-raw");
  BOOST_REQUIRE_NE(rawCaps, nullptr);

  int initialRefCount = GST_CAPS_REFCOUNT_VALUE(rawCaps);

  // Create shared_ptr with TransferType::none
  auto capsSPtr = dh::gst::makeGstSharedPtr(rawCaps, TransferType::none);

  BOOST_REQUIRE_NE(capsSPtr, nullptr);

  // Ref count should increase by 1 since ownership was not transferred
  BOOST_CHECK_EQUAL(GST_CAPS_REFCOUNT_VALUE(rawCaps), initialRefCount + 1);

  // Shared pointer goes out of scope and unrefs the object
  capsSPtr.reset();
  BOOST_CHECK_EQUAL(GST_CAPS_REFCOUNT_VALUE(rawCaps), initialRefCount);
  gst_caps_unref(rawCaps); // Clean up the initial ref
}

// Test makeGstSharedPtr with GstAppSink and TransferType::none
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstAppSink_None, GStreamerSharedPtrTest)
{
  GstElement* rawSink = gst_element_factory_make("appsink", "sink");
  BOOST_REQUIRE_NE(rawSink, nullptr);

  GstAppSink* appSink = GST_APP_SINK(rawSink);
  int initialRefCount = getRefCount(GST_OBJECT(appSink));

  // Create shared_ptr with TransferType::none
  auto sinkSPtr = dh::gst::makeGstSharedPtr(appSink, TransferType::none);

  BOOST_REQUIRE_NE(sinkSPtr, nullptr);

  // Ref count should increase by 1 since ownership was not transferred
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(appSink)), initialRefCount + 1);

  // Shared pointer goes out of scope and unrefs the object
  sinkSPtr.reset();
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(appSink)), initialRefCount);
  gst_object_unref(rawSink); // Clean up the initial ref
}

// Test makeGstSharedPtr with GstBus and TransferType::full
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstBus_Full, GStreamerSharedPtrTest)
{
  GstElement* rawPipeline = gst_element_factory_make("pipeline", "pipeline");
  BOOST_REQUIRE_NE(rawPipeline, nullptr);

  GstBus* rawBus = gst_pipeline_get_bus(GST_PIPELINE(rawPipeline));
  BOOST_REQUIRE_NE(rawBus, nullptr);

  int initialRefCount = getRefCount(GST_OBJECT(rawBus));

  // Create shared_ptr with TransferType::full
  auto busSPtr = dh::gst::makeGstSharedPtr(rawBus, TransferType::full);

  BOOST_REQUIRE_NE(busSPtr, nullptr);

  // Ref count should remain the same because ownership was transferred fully
  BOOST_CHECK_EQUAL(getRefCount(GST_OBJECT(rawBus)), initialRefCount);

  // Shared pointer goes out of scope and unrefs the object
  busSPtr.reset();

  gst_object_unref(rawPipeline); // Clean up the initial ref
}

// Test makeGstSharedPtr with GstBuffer and TransferType::none
BOOST_FIXTURE_TEST_CASE(MakeGstSharedPtr_GstBuffer_None, GStreamerSharedPtrTest)
{
  GstBuffer* rawBuffer = gst_buffer_new();
  BOOST_REQUIRE_NE(rawBuffer, nullptr);

  int initialRefCount = GST_MINI_OBJECT_REFCOUNT_VALUE(rawBuffer);

  // Create shared_ptr with TransferType::none
  auto bufferSPtr = dh::gst::makeGstSharedPtr(rawBuffer, TransferType::none);

  BOOST_REQUIRE_NE(bufferSPtr, nullptr);

  // Ref count should increase by 1 since ownership was not transferred
  BOOST_CHECK_EQUAL(GST_MINI_OBJECT_REFCOUNT_VALUE(rawBuffer), initialRefCount + 1);

  // Shared pointer goes out of scope and unrefs the object
  bufferSPtr.reset();
  BOOST_CHECK_EQUAL(GST_MINI_OBJECT_REFCOUNT_VALUE(rawBuffer), initialRefCount);
  gst_buffer_unref(rawBuffer); // Clean up the initial ref
}

// --- makeGlibSharedPtr ---

BOOST_AUTO_TEST_CASE(MakeGlibSharedPtr_Null)
{
  BOOST_CHECK_EQUAL(makeGlibSharedPtr(static_cast<GError*>(nullptr), TransferType::full), nullptr);
  BOOST_CHECK_EQUAL(
    makeGlibSharedPtr(static_cast<GHashTable*>(nullptr), TransferType::none),
    nullptr
  );
}

BOOST_AUTO_TEST_CASE(MakeGlibSharedPtr_GHashTable_None)
{
  GHashTable* table = g_hash_table_new(g_str_hash, g_str_equal);
  g_hash_table_insert(table, const_cast<char*>("key"), const_cast<char*>("value"));

  auto tableSPtr = makeGlibSharedPtr(table, TransferType::none);
  BOOST_REQUIRE_EQUAL(tableSPtr.get(), table);
  tableSPtr.reset();

  // only our reference was released; the caller's table is still alive and unchanged
  BOOST_CHECK_EQUAL(g_hash_table_size(table), 1U);
  g_hash_table_unref(table);
}

BOOST_AUTO_TEST_CASE(MakeGlibSharedPtr_GHashTable_Full)
{
  GHashTable* table = g_hash_table_new(g_str_hash, g_str_equal);
  g_hash_table_ref(
    table
  ); // keep the table alive to check that the shared_ptr releases exactly one reference
  g_hash_table_insert(table, const_cast<char*>("key"), const_cast<char*>("value"));

  auto tableSPtr = makeGlibSharedPtr(table, TransferType::full);
  tableSPtr.reset();

  BOOST_CHECK_EQUAL(g_hash_table_size(table), 1U);
  g_hash_table_unref(table);
}

BOOST_AUTO_TEST_CASE(MakeGlibSharedPtr_NotReferenceCountedNoneThrows)
{
  GError* error = g_error_new_literal(g_quark_from_static_string("test"), 1, "test");
  BOOST_CHECK_THROW((void)makeGlibSharedPtr(error, TransferType::none), std::invalid_argument);
  g_error_free(error);

  GList* list = g_list_append(nullptr, nullptr);
  BOOST_CHECK_THROW((void)makeGlibSharedPtr(list, TransferType::none), std::invalid_argument);
  g_list_free(list);
}

BOOST_AUTO_TEST_CASE(MakeGlibSharedPtr_FloatingThrows)
{
  GHashTable* table = g_hash_table_new(g_str_hash, g_str_equal);
  BOOST_CHECK_THROW((void)makeGlibSharedPtr(table, TransferType::floating), std::invalid_argument);
  g_hash_table_unref(table);

  GError* error = g_error_new_literal(g_quark_from_static_string("test"), 1, "test");
  BOOST_CHECK_THROW((void)makeGlibSharedPtr(error, TransferType::floating), std::invalid_argument);
  g_error_free(error);
}

BOOST_AUTO_TEST_CASE(MakeGlibSharedPtr_GError_Full)
{
  GError* error = g_error_new_literal(g_quark_from_static_string("test"), 1, "test message");
  const auto errorSPtr = makeGlibSharedPtr(error, TransferType::full);
  BOOST_REQUIRE_EQUAL(errorSPtr.get(), error);
  BOOST_CHECK_EQUAL(std::string(errorSPtr->message), "test message");
  // freed by the deleter; LeakSanitizer reports it otherwise
}
