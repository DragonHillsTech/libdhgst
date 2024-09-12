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
TEST_F(ElementTest, CreationAndDestruction)
{
  Element element1(makeGstSharedPtr(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating));
  ASSERT_EQ(element1.getGstElement().use_count(), 1); // getGstElement creates a new shared_ptr, so the count must be 1
  ASSERT_EQ(GST_OBJECT_REFCOUNT(element1.getGstElement().get()), 2); // getGstElement creates a new shared_ptr, so the GstObject must have increased

  Element element2(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating);
  ASSERT_EQ(element2.getGstElement().use_count(), 1); // getGstElement creates a new shared_ptr, so the count must be 1
  ASSERT_EQ(GST_OBJECT_REFCOUNT(element2.getGstElement().get()), 2); // getGstElement creates a new shared_ptr, so the GstObject must have increased
}

/**
 * @brief Test the ref method of Element.
 */
TEST_F(ElementTest, RefMethod)
{
  Element element(makeGstSharedPtr(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating));
  auto refElement = element.ref();

  EXPECT_EQ(GST_OBJECT_REFCOUNT(refElement.getGstElement().get()), 3); // original, ref and getGstElement
  EXPECT_EQ(GST_OBJECT_REFCOUNT(element.getGstElement().get()), 3);

  // 1 because we get a new shared_ptr each time
  EXPECT_EQ(refElement.getGstElement().use_count(), 1);
  EXPECT_EQ(element.getGstElement().use_count(), 1);
}

TEST_F(ElementTest, GetNameReturnsCorrectName)
{
  Element element(makeGstSharedPtr(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating));


  // Check that getName returns the correct name
  EXPECT_EQ(element.getName(), "test_source");
}

/**
 * @brief Test the setState method.
 */
TEST_F(ElementTest, SetState)
{
  Element element(makeGstSharedPtr(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating));

  ASSERT_EQ(element.setState(GST_STATE_PLAYING), GST_STATE_CHANGE_SUCCESS);
  ASSERT_EQ(element.getState(), GST_STATE_PLAYING);

  // here we whould get a gstreamer error if the Element is destroyed when state != null.
}

/**
 * @brief Test getting all pads.
 */
TEST_F(ElementTest, GetPads)
{
  Element element(makeGstSharedPtr(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating));

  std::vector<GstPad*> pads = element.getPads();
  ASSERT_EQ(pads.size(), 1);  // fakesrc has one "src" pad
}

/**
 * @brief Test getting sink pads.
 */
TEST_F(ElementTest, GetSinkPads)
{
  Element element(makeGstSharedPtr(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating));

  std::vector<GstPad*> sinkPads = element.getSinkPads();
  ASSERT_EQ(sinkPads.size(), 0);  // fakesrc has no sink pads
}

/**
 * @brief Test getting source pads.
 */
TEST_F(ElementTest, GetSrcPads)
{
  Element element(makeGstSharedPtr(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating));

  std::vector<GstPad*> srcPads = element.getSrcPads();
  ASSERT_EQ(srcPads.size(), 1);  // fakesrc has one "src" pad
  ASSERT_EQ(gst_pad_get_name(srcPads[0]), std::string("src"));  // Verify the pad name
}

/**
 * @brief Test getting static pad.
 */
TEST_F(ElementTest, GetStaticPad)
{
  Element element(makeGstSharedPtr(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating));

  GstPad* staticPad = element.getStaticPad("src");
  ASSERT_NE(staticPad, nullptr);  // fakesrc has a "src" pad
  ASSERT_EQ(gst_pad_get_name(staticPad), std::string("src"));  // Verify the pad name
}

/**
 * @brief Test getting compatible pad.
 */
TEST_F(ElementTest, GetCompatiblePad)
{
  Element element(makeGstSharedPtr(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating));

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
TEST_F(ElementTest, LinkAndUnlink)
{
  Element element(makeGstSharedPtr(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating));

  Element sink(makeGstSharedPtr(gst_element_factory_make("fakesink", "test_sink"), TransferType::Floating));


  GstPad* sourcePad = element.getStaticPad("src");
  ASSERT_NE(sourcePad, nullptr) << "Failed to get sourcePad.";

  GstPad* sinkPad = sink.getStaticPad("sink");
  ASSERT_NE(sinkPad, nullptr) << "Failed to get sinkPad.";

  element.link(sink);
  // Check if elements are linked
  ASSERT_TRUE(gst_pad_is_linked(sourcePad));
  ASSERT_TRUE(gst_pad_is_linked(sinkPad));

  element.unlink(sink);
  // Check if elements are unlinked
  ASSERT_FALSE(gst_pad_is_linked(sourcePad));
  ASSERT_FALSE(gst_pad_is_linked(sinkPad));
}
