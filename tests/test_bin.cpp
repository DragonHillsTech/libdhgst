/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
#include "bin.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>

#include <gst/gst.h>

#include <stdexcept>

#include <cstdlib>

using namespace dh::gst;

/**
 * @brief Test fixture for setting up and tearing down the test environment for Element class.
 */
class BinTest
{
public:
  // Setup before first test case
  BinTest()
  {
    // Set G_DEBUG to fatal_criticals to make critical warnings crash the program
    setenv("G_DEBUG", "fatal_criticals", 1);
    gst_init(nullptr, nullptr);  // Initialize GStreamer
  }
};

BOOST_FIXTURE_TEST_CASE(CreationAndDestruction, BinTest)
{
  // ctor GstBinSPtr
  auto bin1 = Bin::create(makeGstSharedPtr(GST_BIN_CAST(gst_bin_new("bin1")), TransferType::Floating));
  BOOST_REQUIRE_EQUAL(bin1->getGstBin().use_count(), 1); // getGstBin creates a new shared_ptr, so the count must be 1
  BOOST_REQUIRE_EQUAL(GST_OBJECT_REFCOUNT(bin1->getGstBin().get()), 2); // getGstBin creates a new shared_ptr, so the GstObject must have increased

  //ctor GstBin*
  auto bin2 = Bin::create(makeGstSharedPtr(GST_BIN_CAST(gst_bin_new("bin2")), TransferType::Floating));
  BOOST_REQUIRE_EQUAL(bin2->getGstBin().use_count(), 1); // getGstBin creates a new shared_ptr, so the count must be 1
  BOOST_REQUIRE_EQUAL(GST_OBJECT_REFCOUNT(bin2->getGstBin().get()), 2); // getGstBin creates a new shared_ptr, so the GstObject must have increased

  //ctor name
  const std::string bin3Name("bin3");
  auto bin3 = Bin::create(bin3Name);
  BOOST_REQUIRE_EQUAL(bin3->getName(), bin3Name);
}

BOOST_FIXTURE_TEST_CASE(GetNameReturnsCorrectName, BinTest)
{
  auto bin1 = Bin::create("bin1");

  // Check that getName returns the correct name
  BOOST_CHECK_EQUAL(bin1->getName(), "bin1");
}

BOOST_FIXTURE_TEST_CASE(SetState, BinTest)
{
  auto bin1 = Bin::create("bin1");

  BOOST_REQUIRE_EQUAL(bin1->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_SUCCESS);
  BOOST_REQUIRE_EQUAL(bin1->getState(), GST_STATE_PLAYING);

  // here we would get a gstreamer error if the Element is destroyed when state != null.
}

BOOST_FIXTURE_TEST_CASE(AddAndRemoveElementSharedPtr, BinTest)
{
  auto bin1 = Bin::create("bin1");
  auto element1SPtr = makeGstSharedPtr(gst_element_factory_make("fakesrc", "element1"), TransferType::Floating);

  // add
  BOOST_CHECK_NO_THROW(bin1->addElement(element1SPtr));
  BOOST_CHECK(gst_bin_get_by_name(bin1->getGstBin().get(), GST_ELEMENT_NAME(element1SPtr.get())) != nullptr);

  // remove
  BOOST_CHECK_NO_THROW(bin1->removeElement(element1SPtr));
  BOOST_CHECK(gst_bin_get_by_name(bin1->getGstBin().get(), GST_ELEMENT_NAME(element1SPtr.get())) == nullptr);
}

BOOST_FIXTURE_TEST_CASE(AddAndRemoveElementByReference, BinTest)
{
  auto bin1 = Bin::create("bin1");
  auto element1 = Element::create(
    gst_element_factory_make("fakesrc", "element1"),
    TransferType::Floating
  );

  // add
  BOOST_CHECK_NO_THROW(bin1->addElement(element1));
  BOOST_CHECK(gst_bin_get_by_name(bin1->getGstBin().get(), element1->getName().c_str()) != nullptr);

  // remove
  BOOST_CHECK_NO_THROW(bin1->removeElement(element1));
  BOOST_CHECK(gst_bin_get_by_name(bin1->getGstBin().get(), element1->getName().c_str()) == nullptr);
}

BOOST_FIXTURE_TEST_CASE(GetElementByName, BinTest)
{
  auto bin1 = Bin::create("bin1");
  auto element1 = Element::create(gst_element_factory_make("fakesrc", "element1"), TransferType::Floating);

  // find existing
  bin1->addElement(element1);
  auto element2 = bin1->getElementByName(element1->getName());
  BOOST_CHECK_EQUAL(element2->getName(), element1->getName());
  BOOST_CHECK_EQUAL(element2->getGstElement(), element1->getGstElement());

  // find not existing
  BOOST_CHECK_THROW((void)bin1->getElementByName("DoesNotExist"), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(GetElementByNameRecurseUp, BinTest)
{
  auto bin1 = Bin::create("bin1");
  auto element1 = Element::create(gst_element_factory_make("fakesrc", "element1"), TransferType::Floating);

  // find existing
  bin1->addElement(element1);
  auto element2 = bin1->getElementByNameRecurseUp(element1->getName());
  BOOST_CHECK_EQUAL(element2->getName(), element1->getName());
  BOOST_CHECK_EQUAL(element2->getGstElement(), element1->getGstElement());

  // find not existing
  BOOST_CHECK_THROW((void)bin1->getElementByNameRecurseUp("DoesNotExist"), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(FromDescriptionValidPipeline, BinTest)
{
  const std::string description("fakesrc ! fakesink");
  BOOST_CHECK_NO_THROW(
    {
      std::shared_ptr<dh::gst::Bin> bin = dh::gst::Bin::fromDescription(description, false);
    }
  );
}

BOOST_FIXTURE_TEST_CASE(FromDescriptionGhostUnlinkedPads, BinTest)
{
  const std::string description("fakesrc name=src ! fakesink");
  BOOST_CHECK_NO_THROW(
    {
      std::shared_ptr<dh::gst::Bin> bin = dh::gst::Bin::fromDescription(description, true);
    }
  );
}

BOOST_FIXTURE_TEST_CASE(FromDescriptionInvalidPipelineThrows, BinTest)
{
  const std::string invalidDescription("invalidelement ! fakesink");
  BOOST_CHECK_THROW(
    {
     std::shared_ptr<dh::gst::Bin> bin = dh::gst::Bin::fromDescription(invalidDescription, false);
    }
    , std::runtime_error);
}

