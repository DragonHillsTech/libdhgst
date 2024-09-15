/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
#include "bin.hpp"

#include <gtest/gtest.h>
#include <gst/gst.h>

#include <stdexcept>

#include <cstdlib>

using namespace dh::gst;

/**
 * @brief Test fixture for setting up and tearing down the test environment for Element class.
 */
class BinTest : public ::testing::Test
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

TEST_F(BinTest, CreationAndDestruction)
{
  // ctor GstBinSPtr
  Bin bin1(makeGstSharedPtr(GST_BIN_CAST(gst_bin_new("bin1")), TransferType::Floating));
  ASSERT_EQ(bin1.getGstBin().use_count(), 1); // getGstBin creates a new shared_ptr, so the count must be 1
  ASSERT_EQ(GST_OBJECT_REFCOUNT(bin1.getGstBin().get()), 2); // getGstBin creates a new shared_ptr, so the GstObject must have increased

  //ctor GstBin*
  Bin bin2(makeGstSharedPtr(GST_BIN_CAST(gst_bin_new("bin2")), TransferType::Floating));
  ASSERT_EQ(bin2.getGstBin().use_count(), 1); // getGstBin creates a new shared_ptr, so the count must be 1
  ASSERT_EQ(GST_OBJECT_REFCOUNT(bin2.getGstBin().get()), 2); // getGstBin creates a new shared_ptr, so the GstObject must have increased

  //ctor name
  const std::string bin3Name("bin3");
  Bin bin3(bin3Name);
  ASSERT_EQ(bin3.getName(), bin3Name);
}

TEST_F(BinTest, Move)
{
  Bin bin1("bin1");

  //move ctor bin1 -> bin2
  Bin bin2(std::move(bin1));
  ASSERT_EQ(bin2.getGstBin().use_count(), 1); // getGstBin creates a new shared_ptr, so the count must be 1
  ASSERT_EQ(GST_OBJECT_REFCOUNT(bin2.getGstBin().get()), 2); // getGstBin creates a new shared_ptr, so the GstObject must have increased
  ASSERT_EQ(bin2.getName(), "bin1");
  // check if bin1 is moved and throws on access
  EXPECT_THROW(bin1.getName(), std::logic_error);

  // move assignment back bin2 -> bin1
  bin1 = std::move(bin2);
  ASSERT_EQ(bin1.getGstBin().use_count(), 1); // getGstBin creates a new shared_ptr, so the count must be 1
  ASSERT_EQ(GST_OBJECT_REFCOUNT(bin1.getGstBin().get()), 2); // getGstBin creates a new shared_ptr, so the GstObject must have increased
  ASSERT_EQ(bin1.getName(), "bin1");
  // check if bin1 is moved and throws on access
  EXPECT_THROW(bin2.getName(), std::logic_error);
}

TEST_F(BinTest, RefMethod)
{
  Bin bin1("bin1");
  auto bin1Ref = bin1.ref();

  EXPECT_EQ(GST_OBJECT_REFCOUNT(bin1Ref.getGstElement().get()), 3); // original, ref and getGstElement
  EXPECT_EQ(GST_OBJECT_REFCOUNT(bin1.getGstElement().get()), 3);

  // 1 because we get a new shared_ptr each time
  EXPECT_EQ(bin1Ref.getGstElement().use_count(), 1);
  EXPECT_EQ(bin1.getGstElement().use_count(), 1);
}

TEST_F(BinTest, GetNameReturnsCorrectName)
{
  Bin bin1("bin1");

  // Check that getName returns the correct name
  EXPECT_EQ(bin1.getName(), "bin1");
}

TEST_F(BinTest, SetState)
{
  Bin bin1("bin1");

  ASSERT_EQ(bin1.setState(GST_STATE_PLAYING), GST_STATE_CHANGE_SUCCESS);
  ASSERT_EQ(bin1.getState(), GST_STATE_PLAYING);

  // here we whould get a gstreamer error if the Element is destroyed when state != null.
}

TEST_F(BinTest, AddAndRemoveElementSharedPtr)
{
  Bin bin1("bin1");
  auto element1SPtr = makeGstSharedPtr(gst_element_factory_make("fakesrc", "element1"), TransferType::Floating);

  // add
  EXPECT_NO_THROW(bin1.addElement(element1SPtr));
  EXPECT_TRUE(gst_bin_get_by_name(bin1.getGstBin().get(), GST_ELEMENT_NAME(element1SPtr.get())) != nullptr);

  // remove
  EXPECT_NO_THROW(bin1.removeElement(element1SPtr));
  EXPECT_TRUE(gst_bin_get_by_name(bin1.getGstBin().get(), GST_ELEMENT_NAME(element1SPtr.get())) == nullptr);

}

TEST_F(BinTest, AddAndRemoveElementByReference)
{
  Bin bin1("bin1");
  Element element1(
    gst_element_factory_make("fakesrc", "element1"),
    TransferType::Floating
  );

  // add
  EXPECT_NO_THROW(bin1.addElement(element1));
  EXPECT_TRUE(gst_bin_get_by_name(bin1.getGstBin().get(), element1.getName().c_str()) != nullptr);

  // remove
  EXPECT_NO_THROW(bin1.removeElement(element1));
  EXPECT_TRUE(gst_bin_get_by_name(bin1.getGstBin().get(), element1.getName().c_str()) == nullptr);
}

TEST_F(BinTest, GetElementByName)
{
  Bin bin1("bin1");
  Element element1(gst_element_factory_make("fakesrc", "element1"), TransferType::Floating);

  // find existing
  bin1.addElement(element1);
  auto element2 = bin1.getElementByName(element1.getName());
  EXPECT_EQ(element2.getName(), element1.getName());
  EXPECT_EQ(element2.getGstElement(), element1.getGstElement());

  // find not existing
  EXPECT_THROW(bin1.getElementByName("DoesNotExist"), std::runtime_error);
}

TEST_F(BinTest, GetElementByNameRecurseUp)
{
  Bin bin1("bin1");
  Element element1(gst_element_factory_make("fakesrc", "element1"), TransferType::Floating);

  // find existing
  bin1.addElement(element1);
  auto element2 = bin1.getElementByNameRecurseUp(element1.getName());
  EXPECT_EQ(element2.getName(), element1.getName());
  EXPECT_EQ(element2.getGstElement(), element1.getGstElement());

  // find not existing
  EXPECT_THROW(bin1.getElementByNameRecurseUp("DoesNotExist"), std::runtime_error);
}

TEST_F(BinTest, FromDescriptionValidPipeline)
{
  const std::string description("fakesrc ! fakesink");
  EXPECT_NO_THROW(
    {
      dh::gst::Bin bin = dh::gst::Bin::fromDescription(description, false);
    }
  );
}

TEST_F(BinTest, FromDescriptionGhostUnlinkedPads)
{
  const std::string description("fakesrc name=src ! fakesink");
  EXPECT_NO_THROW(
    {
      dh::gst::Bin bin = dh::gst::Bin::fromDescription(description, true);
    }
  );
}

TEST_F(BinTest, FromDescriptionInvalidPipelineThrows)
{
  const std::string invalidDescription("invalidelement ! fakesink");
  EXPECT_THROW(
    {
     dh::gst::Bin bin = dh::gst::Bin::fromDescription(invalidDescription, false);
    }
    , std::runtime_error);
}
