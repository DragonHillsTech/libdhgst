/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */

#include "elementfactory.hpp"
#include "pipeline.hpp"
#include "element.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>

#include <gst/gst.h>

#include <thread>
#include <vector>

using namespace dh::gst;

class ObjectTest
{
public:
  // Setup before first test case
  ObjectTest()
  {
    // Set G_DEBUG to fatal_criticals to make critical warnings crash the program
    setenv("G_DEBUG", "fatal_criticals", 1);
    gst_init(nullptr, nullptr);  // Initialize GStreamer
  }
};

BOOST_FIXTURE_TEST_CASE(TestConnectGobjectSignal_PadAdded, ObjectTest)
{
  auto pipeline = Pipeline::fromDescription("videotestsrc ! decodebin name=test-decodebin");

  auto decodeBinElement = pipeline.getElementByName("test-decodebin");

  // Connect to the "pad-added" signal
  bool signalEmitted = false;
  std::shared_ptr<GstPad> capturedPad;
  decodeBinElement->padAddedSignal().connect(
  [&](std::shared_ptr<GstPad> pad)
    {
      signalEmitted = true;
      capturedPad = pad;
    }
  );

  // Set pipeline to PLAYING to trigger the "pad-added" signal
  const auto setStateResult = pipeline.setState(GST_STATE_PLAYING);
  BOOST_REQUIRE_NE(setStateResult, GST_STATE_CHANGE_FAILURE);

  // Wait for the main loop to process events
  GMainContext* context = g_main_context_default();
  for(int i = 0; i < 10 && !signalEmitted; ++i)
  {
    g_main_context_iteration(context, FALSE);
    g_usleep(100000); // Sleep for 100ms
  }

  // Check that the signal was emitted and the pad was captured
  BOOST_CHECK(signalEmitted);
  BOOST_REQUIRE_NE(capturedPad, nullptr);
  pipeline.setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(TestConnectGobjectSignal_ElementAdded, ObjectTest)
{
  // Create a GstBin
  auto bin = Bin::create("test-bin");

  bool signalEmitted = false;
  GstElementSPtr capturedElement;
  bin->elementAddedSignal().connect(
    [&](std::shared_ptr<GstElement> element)
    {
      signalEmitted = true;
      capturedElement = element;
    }
  );

  auto element = ElementFactory::makeElement("fakesrc", "test-source");

  bin->addElement(element);

  // Wait for the main loop to process events
  GMainContext* context = g_main_context_default();
  g_main_context_iteration(context, FALSE);

  // Check that the signal was emitted and the element was captured
  BOOST_CHECK(signalEmitted);
  BOOST_REQUIRE_NE(capturedElement, nullptr);
  BOOST_CHECK_EQUAL(capturedElement, element->getGstElement());
}

//TODO:

BOOST_FIXTURE_TEST_CASE(TestConnectGobjectSignal_deleteWhileCallbackActive, ObjectTest)
{
  /* TODO:
   * This works find, BUT
   * *
   */

  // Create a GstBin
  auto bin = Bin::create("test-bin");

  bool signalEmitted = false;
  GstElementSPtr capturedElement;
  bin->elementAddedSignal().connect(
    [&](std::shared_ptr<GstElement> element)
    {
      signalEmitted = true;
      capturedElement = element;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  );

  auto element = ElementFactory::makeElement("fakesrc", "test-source");

  BOOST_REQUIRE_EQUAL(bin.use_count(), 1);
  // does not increase gstBinSPtr.use_count because a new one is created from the raw ptr
  GstBin* gstBinPtr = bin->getGstBin().get();

  BOOST_REQUIRE_EQUAL(GST_OBJECT_REFCOUNT(gstBinPtr), 1);

  std::thread thread(
    // Keep a copy to the objects when using threads. "&" is dangerous!.
    [bin, element]()
    {
      bin->addElement(element);
    }
  );

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // the callback should have increased in the thread capture
  BOOST_REQUIRE_EQUAL(bin.use_count(), 2);

  // one in Bin, one in  callback
  const auto currentGObjectRefCnt = GST_OBJECT_REFCOUNT(gstBinPtr);
  BOOST_REQUIRE_GE(currentGObjectRefCnt, 2);

  std::weak_ptr<Bin> weakBin = bin;
  bin.reset();

  // the Bin element must still exist. after deleting the local pointer. Copy in thread capture
  BOOST_REQUIRE_NE(weakBin.lock(), nullptr);
  // and of course the internal object
  BOOST_REQUIRE_EQUAL(GST_OBJECT_REFCOUNT(gstBinPtr), currentGObjectRefCnt);

  // Wait for the main loop to process events
  GMainContext* context = g_main_context_default();
  g_main_context_iteration(context, FALSE);

  // Check that the signal was emitted and the element was captured
  BOOST_CHECK(signalEmitted);
  BOOST_REQUIRE_NE(capturedElement, nullptr);
  BOOST_CHECK_EQUAL(capturedElement, element->getGstElement());

  // the Bin element must still exist.
  BOOST_REQUIRE_NE(weakBin.lock(), nullptr);
  BOOST_REQUIRE_GE(GST_OBJECT_REFCOUNT(gstBinPtr), 1);

  thread.join();

  // Now everything should be deleted
  BOOST_REQUIRE_EQUAL(weakBin.lock(), nullptr);
}

