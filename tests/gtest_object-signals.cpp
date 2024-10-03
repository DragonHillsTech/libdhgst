/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */

#include <elementfactory.hpp>
#include <gtest/gtest.h>
#include "pipeline.hpp"
#include "element.hpp"
#include <gst/gst.h>
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
  decodeBinElement.padAddedSignal().connect(
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
  {
    // Create a GstBin
    Bin bin("test-bin");

    bool signalEmitted = false;
    GstElementSPtr capturedElement;
    bin.elementAddedSignal().connect(
      [&](std::shared_ptr<GstElement> element)
      {
        signalEmitted = true;
        capturedElement = element;
      }
    );

    auto element = ElementFactory::makeElement("fakesrc", "test-source");

    bin.addElement(element);

    // Wait for the main loop to process events
    GMainContext* context = g_main_context_default();
    g_main_context_iteration(context, FALSE);

    // Check that the signal was emitted and the element was captured
    EXPECT_TRUE(signalEmitted);
    ASSERT_NE(capturedElement, nullptr);
    EXPECT_EQ(capturedElement, element.getGstElement());
  }
}
