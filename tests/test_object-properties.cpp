/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
#include "element.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>
#include <gst/gst.h>

#include <cstdlib>
#include <string>

using namespace dh::gst;

/**
 * @brief Test fixture for setting up the test environment for Object property access.
 */
class ObjectPropertiesTest
{
public:
  ObjectPropertiesTest()
  {
    // Set G_DEBUG to fatal_criticals to make critical warnings crash the program
    setenv("G_DEBUG", "fatal_criticals", 1);
    gst_init(nullptr, nullptr);
  }
};

BOOST_FIXTURE_TEST_CASE(GetStringProperty, ObjectPropertiesTest)
{
  auto element = Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating);

  // "name" is a gchar* property of every GstObject
  const auto name = element->getProperty<std::string>("name");
  BOOST_CHECK_EQUAL(name, "test_source");
}
