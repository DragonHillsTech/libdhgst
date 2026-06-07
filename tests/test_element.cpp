/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
#include "element.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>
#include <gst/gst.h>

#include <stdexcept>

#include <cstdlib>

using namespace dh::gst;

/**
 * @brief Test fixture for setting up and tearing down the test environment for Element class.
 */
class ElementTest
{
public:
  // Setup before first test case
  ElementTest()
  {
    // Set G_DEBUG to fatal_criticals to make critical warnings crash the program
    setenv("G_DEBUG", "fatal_criticals", 1);
    gst_init(nullptr, nullptr);  // Initialize GStreamer
  }
};

BOOST_FIXTURE_TEST_CASE(CreationAndDestruction, ElementTest)
{
  // ctor GstElementSPtr
  auto element1 = Element::create(makeGstSharedPtr(gst_element_factory_make("fakesrc", "testSource1"), TransferType::Floating));
  BOOST_REQUIRE_EQUAL(element1->getGstElement().use_count(), 1); // getGstElement creates a new shared_ptr, so the count must be 1
  BOOST_REQUIRE_EQUAL(GST_OBJECT_REFCOUNT(element1->getGstElement().get()), 2); // getGstElement creates a new shared_ptr, so the GstObject must have increased
  BOOST_REQUIRE_EQUAL(element1->getFactoryName(), "fakesrc");

  //ctor GstElement*
  auto element2 = Element::create(gst_element_factory_make("fakesrc", "testSource2"), TransferType::Floating);
  BOOST_REQUIRE_EQUAL(element2->getGstElement().use_count(), 1); // getGstElement creates a new shared_ptr, so the count must be 1
  BOOST_REQUIRE_EQUAL(GST_OBJECT_REFCOUNT(element2->getGstElement().get()), 2); // getGstElement creates a new shared_ptr, so the GstObject must have increased
}

BOOST_FIXTURE_TEST_CASE(GetNameReturnsCorrectName, ElementTest)
{
  const auto element = Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating);

  // Check that getName returns the correct name
  BOOST_CHECK_EQUAL(element->getName(), "test_source");
}

BOOST_FIXTURE_TEST_CASE(SetState, ElementTest)
{
  auto element = Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating);

  BOOST_REQUIRE_EQUAL(element->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_SUCCESS);
  BOOST_REQUIRE_EQUAL(element->getState(), GST_STATE_PLAYING);

  // here we would get a gstreamer error if the Element is destroyed when state != null.
}

BOOST_FIXTURE_TEST_CASE(GetPads, ElementTest)
{
  auto element = Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating);

  std::vector<GstPad*> pads = element->getPads();
  BOOST_REQUIRE_EQUAL(pads.size(), 1);  // fakesrc has one "src" pad
}

BOOST_FIXTURE_TEST_CASE(GetSinkPads, ElementTest)
{
  auto element = Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating);

  std::vector<GstPad*> sinkPads = element->getSinkPads();
  BOOST_REQUIRE_EQUAL(sinkPads.size(), 0);  // fakesrc has no sink pads
}

BOOST_FIXTURE_TEST_CASE(GetSrcPads, ElementTest)
{
  auto element = Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating);

  std::vector<GstPad*> srcPads = element->getSrcPads();
  BOOST_REQUIRE_EQUAL(srcPads.size(), 1);  // fakesrc has one "src" pad
  BOOST_REQUIRE_EQUAL(gst_pad_get_name(srcPads[0]), std::string("src"));  // Verify the pad name
}

BOOST_FIXTURE_TEST_CASE(GetStaticPad, ElementTest)
{
  auto element = Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating);

  GstPad* staticPad = element->getStaticPad("src");
  BOOST_REQUIRE_NE(staticPad, nullptr);  // fakesrc has a "src" pad
  BOOST_REQUIRE_EQUAL(gst_pad_get_name(staticPad), std::string("src"));  // Verify the pad name
}

BOOST_FIXTURE_TEST_CASE(GetCompatiblePad, ElementTest)
{
  auto element = Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating);

  GstPad* sinkPad = gst_pad_new("sink", GST_PAD_SINK);
  // Create capabilities for the sink pad
  GstCaps* caps = gst_caps_new_simple("video/x-raw",
                                      "format", G_TYPE_STRING, "I420",
                                      "width", G_TYPE_INT, 640,
                                      "height", G_TYPE_INT, 480,
                                      "framerate", GST_TYPE_FRACTION, 30, 1,
                                      nullptr);
  BOOST_REQUIRE_NE(caps, nullptr);

  // Call getCompatiblePad with the created pad and caps
  GstPad* resultPad = element->getCompatiblePad(sinkPad, caps);

  BOOST_REQUIRE_EQUAL(resultPad, element->getSrcPads().at(0));

  gst_caps_unref(caps);  // Unref the caps
}

BOOST_FIXTURE_TEST_CASE(LinkAndUnlink, ElementTest)
{
  auto element = Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating);

  auto sink = Element::create(gst_element_factory_make("fakesink", "test_sink"), TransferType::Floating);

  GstPad* sourcePad = element->getStaticPad("src");
  BOOST_REQUIRE_NE(sourcePad, nullptr);

  GstPad* sinkPad = sink->getStaticPad("sink");
  BOOST_REQUIRE_NE(sinkPad, nullptr);

  element->link(sink);
  // Check if elements are linked
  BOOST_REQUIRE(gst_pad_is_linked(sourcePad));
  BOOST_REQUIRE(gst_pad_is_linked(sinkPad));

  element->unlink(sink);
  // Check if elements are unlinked
  BOOST_REQUIRE(!gst_pad_is_linked(sourcePad));
  BOOST_REQUIRE(!gst_pad_is_linked(sinkPad));
}

BOOST_FIXTURE_TEST_CASE(SetName, ElementTest)
{
  auto element = Element::create(gst_element_factory_make("fakesrc", "firstName"), TransferType::Floating);
  BOOST_REQUIRE_EQUAL(element->getName(), "firstName");

  BOOST_REQUIRE_NO_THROW(element->setName("secondName"));
  BOOST_CHECK_EQUAL(element->getName(), "secondName");
}

