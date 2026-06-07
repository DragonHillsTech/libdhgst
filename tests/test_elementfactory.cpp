/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
#include "elementfactory.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>
#include <gst/gst.h>

#include <stdexcept>

#include <cstdlib>

using namespace dh::gst;

/**
 * @brief Test fixture for setting up and tearing down the test environment for Element class.
 */
class ElementFactoryTest
{
public:
  // Setup before first test case
  ElementFactoryTest()
  {
    // Set G_DEBUG to fatal_criticals to make critical warnings crash the program
    setenv("G_DEBUG", "fatal_criticals", 1);
    gst_init(nullptr, nullptr);  // Initialize GStreamer
  }
};

BOOST_FIXTURE_TEST_CASE(FromFactoryNameTest, ElementFactoryTest)
{
  const std::string factoryName="fakesrc";
  auto factory = ElementFactory::fromFactoryName(factoryName);
  BOOST_REQUIRE_NE(factory.getGstElementFactory(), nullptr);
}

BOOST_FIXTURE_TEST_CASE(FromFactoryNameNotFoundTest, ElementFactoryTest)
{
  BOOST_CHECK_THROW(
    {
      auto factory = ElementFactory::fromFactoryName("doesNotExist");
    },
    std::runtime_error
  );
}

// Test getMetaData method
BOOST_FIXTURE_TEST_CASE(GetMetaDataTest, ElementFactoryTest)
{
  const auto factory = ElementFactory::fromFactoryName("fakesrc");

  const std::string longName = factory.getMetaData("long-name");
  BOOST_REQUIRE(!longName.empty());
}

// Test getMetadataKeys method
BOOST_FIXTURE_TEST_CASE(GetMetadataKeysTest, ElementFactoryTest)
{
  const auto factory = ElementFactory::fromFactoryName("fakesrc");

  const auto keys = factory.getMetadataKeys();
  BOOST_REQUIRE(!keys.empty());
  BOOST_REQUIRE(std::find(keys.begin(), keys.end(), "long-name") != keys.end());
}

// Test element creation
BOOST_FIXTURE_TEST_CASE(CreateElementTest, ElementFactoryTest)
{
  const auto factory = ElementFactory::fromFactoryName("fakesrc");

  const std::string elementName("myElement");
  const auto element = factory.createElement(elementName);

  // Check the type of the created element
  BOOST_CHECK_EQUAL(element->getName(), elementName);
}

BOOST_FIXTURE_TEST_CASE(makeElementTest, ElementFactoryTest)
{
  const std::string elementName("myElement");
  const auto element = ElementFactory::makeElement("fakesrc", elementName);

  // Ensure the element was created
  BOOST_CHECK_EQUAL(element->getName(), elementName);
}

// Test the getElementType method
BOOST_FIXTURE_TEST_CASE(GetElementTypeTest, ElementFactoryTest)
{
  const auto factory = ElementFactory::fromFactoryName("fakesrc");

  const GType elementType = factory.getElementType();
  BOOST_REQUIRE_NE(elementType, 0);
}

