/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */

#include "elementfactory.hpp"
#include "pipeline.hpp"
#include "element.hpp"

#include <gtest/gtest.h>

#include <gst/gst.h>

#include <thread>
#include <vector>

using namespace dh::gst;

class ObjectTest : public ::testing::Test
{
public:
  // Setup before first test case
  static void SetUpTestSuite()
  {
    // Set G_DEBUG to fatal_criticals to make critical warnings crash the program
    setenv("G_DEBUG", "fatal_criticals", 1);
    gst_init(nullptr, nullptr);  // Initialize GStreamer
  }

  // Cleanup after last test case
  static void TearDownTestSuite()
  {
    gst_deinit();
  }
};

TEST_F(ObjectTest, TestConnectGobjectSignal_PadAdded)
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
  ASSERT_NE(setStateResult, GST_STATE_CHANGE_FAILURE);

  // Wait for the main loop to process events
  GMainContext* context = g_main_context_default();
  for(int i = 0; i < 10 && !signalEmitted; ++i)
  {
    g_main_context_iteration(context, FALSE);
    g_usleep(100000); // Sleep for 100ms
  }

  // Check that the signal was emitted and the pad was captured
  EXPECT_TRUE(signalEmitted);
  ASSERT_NE(capturedPad, nullptr);
  pipeline.setState(GST_STATE_NULL);
}

TEST_F(ObjectTest, TestConnectGobjectSignal_ElementAdded)
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
  EXPECT_TRUE(signalEmitted);
  ASSERT_NE(capturedElement, nullptr);
  EXPECT_EQ(capturedElement, element->getGstElement());
}

//TODO:

TEST_F(ObjectTest, TestConnectGobjectSignal_deleteWhileCallbackActive)
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

  ASSERT_EQ(bin.use_count(), 1);
  // does not increase gstBinSPtr.use_count because a new one is created from the raw ptr
  GstBin* gstBinPtr = bin->getGstBin().get();

  ASSERT_EQ(GST_OBJECT_REFCOUNT(gstBinPtr), 1);

  std::thread thread(
    // Keep a copy to the objects when using threads. "&" is dangerous!.
    [bin, element]()
    {
      bin->addElement(element);
    }
  );

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // the callback should have increased in the thread capture
  ASSERT_EQ(bin.use_count(), 2);

  // one in Bin, one in  callback
  const auto currentGObjectRefCnt = GST_OBJECT_REFCOUNT(gstBinPtr);
  ASSERT_GE(currentGObjectRefCnt, 2);

  std::weak_ptr<Bin> weakBin = bin;
  bin.reset();

  // the Bin element must still exist. after deleting the local pointer. Copy in thread capture
  ASSERT_NE(weakBin.lock(), nullptr);
  // and of course the internal object
  ASSERT_EQ(GST_OBJECT_REFCOUNT(gstBinPtr), currentGObjectRefCnt);

  // Wait for the main loop to process events
  GMainContext* context = g_main_context_default();
  g_main_context_iteration(context, FALSE);

  // Check that the signal was emitted and the element was captured
  EXPECT_TRUE(signalEmitted);
  ASSERT_NE(capturedElement, nullptr);
  EXPECT_EQ(capturedElement, element->getGstElement());

  // the Bin element must still exist.
  ASSERT_NE(weakBin.lock(), nullptr);
  ASSERT_GE(GST_OBJECT_REFCOUNT(gstBinPtr), 1);

  thread.join();

  // Now everything should be deleted
  ASSERT_EQ(weakBin.lock(), nullptr);
}
