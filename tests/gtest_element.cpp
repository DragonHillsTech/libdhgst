#include <gtest/gtest.h>
#include <gst/gst.h>
#include "element.hpp"

using namespace dh::gst;

/**
 * @brief Test fixture for setting up and tearing down the test environment for Element class.
 */
class ElementTest : public ::testing::Test
{
public:
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

/**
 * @brief Test creation and destruction of Element.
 */
TEST_F(ElementTest, CreationAndDestruction) {
  GstElement* gstElement = gst_element_factory_make("fakesrc", "source");
  Element element(gstElement, TransferType::Floating);
  ASSERT_EQ(element.get(), gstElement);
}

/**
 * @brief Test the ref method of Element.
 */
TEST_F(ElementTest, RefMethod) {
  Element element(gst_element_factory_make("fakesrc", "source"), TransferType::Floating);
  auto refElement = element.ref();
  // Check if ref increases the reference count
  EXPECT_EQ(GST_OBJECT_REFCOUNT(refElement.get()), 2);
  EXPECT_EQ(GST_OBJECT_REFCOUNT(element.get()), 2);
}

/**
 * @brief Test the setState method.
 */
TEST_F(ElementTest, SetState) {
  Element element(gst_element_factory_make("fakesrc", "source"), TransferType::Floating);
  ASSERT_EQ(element.setState(GST_STATE_PLAYING), GST_STATE_CHANGE_SUCCESS);
  ASSERT_EQ(element.getState(), GST_STATE_PLAYING);
}

/**
 * @brief Test getting all pads.
 */
TEST_F(ElementTest, GetPads) {
  Element element(gst_element_factory_make("fakesrc", "source"), TransferType::Floating);
  std::vector<GstPad*> pads = element.getPads();
  ASSERT_EQ(pads.size(), 1);  // fakesrc has one "src" pad
}

/**
 * @brief Test getting sink pads.
 */
TEST_F(ElementTest, GetSinkPads) {
  Element element(gst_element_factory_make("fakesrc", "source"), TransferType::Floating);
  std::vector<GstPad*> sinkPads = element.getSinkPads();
  ASSERT_EQ(sinkPads.size(), 0);  // fakesrc has no sink pads
}

/**
 * @brief Test getting source pads.
 */
TEST_F(ElementTest, GetSrcPads) {
  Element element(gst_element_factory_make("fakesrc", "source"), TransferType::Floating);
  std::vector<GstPad*> srcPads = element.getSrcPads();
  ASSERT_EQ(srcPads.size(), 1);  // fakesrc has one "src" pad
  ASSERT_EQ(gst_pad_get_name(srcPads[0]), std::string("src"));  // Verify the pad name
}

/**
 * @brief Test getting static pad.
 */
TEST_F(ElementTest, GetStaticPad) {
  Element element(gst_element_factory_make("fakesrc", "source"), TransferType::Floating);
  GstPad* staticPad = element.getStaticPad("src");
  ASSERT_NE(staticPad, nullptr);  // fakesrc has a "src" pad
  ASSERT_EQ(gst_pad_get_name(staticPad), std::string("src"));  // Verify the pad name
}

/**
 * @brief Test getting compatible pad.
 */
TEST_F(ElementTest, GetCompatiblePad)
{
  Element element(gst_element_factory_make("fakesrc", "source"), TransferType::Floating);

  GstPad* sinkPad = gst_pad_new("sink", GST_PAD_SINK);
  // Create capabilities for the sink pad
  GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                      "format", G_TYPE_STRING, "I420",
                                      "width", G_TYPE_INT, 640,
                                      "height", G_TYPE_INT, 480,
                                      "framerate", GST_TYPE_FRACTION, 30, 1,
                                      nullptr);
  ASSERT_NE(caps, nullptr) << "Failed to create caps";

  // Call getCompatiblePad with the created pad and caps
  GstPad* resultPad = element.getCompatiblePad(sinkPad, caps);

  ASSERT_EQ(resultPad, element.getSrcPads().at(0));

  gst_caps_unref(caps);  // Unref the caps
}


/**
 * @brief Test linking and unlinking elements.
 */
TEST_F(ElementTest, LinkAndUnlink) {
  Element element(gst_element_factory_make("fakesrc", "source"), TransferType::Floating);

  GstElement* sinkElement = gst_element_factory_make("fakesink", "sink");
  ASSERT_NE(sinkElement, nullptr) << "Failed to create GStreamer sink element.";

  Element sink(sinkElement, TransferType::None);

  element.link(sink);
  // Check if elements are linked
  ASSERT_EQ(gst_element_link(element.get(), sink.get()), TRUE);

  element.unlink(sink);
  // Check if elements are unlinked
  ASSERT_EQ(gst_element_link(element.get(), sink.get()), FALSE);

  gst_object_unref(sinkElement);
}
