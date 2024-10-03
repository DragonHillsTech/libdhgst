/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
#include "elementfactory.hpp"

#include <gtest/gtest.h>
#include <gst/gst.h>

#include <stdexcept>

#include <cstdlib>

using namespace dh::gst;

/**
 * @brief Test fixture for setting up and tearing down the test environment for Element class.
 */
class ElementFactoryTest : public ::testing::Test
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

TEST_F(ElementFactoryTest, FromFactoryNameTest)
{
  const std::string factoryName="fakesrc";
  auto factory = ElementFactory::fromFactoryName(factoryName);
  ASSERT_NE(factory.getGstElementFactory(), nullptr) << "Factory '" << factoryName << "' not found.";
}

TEST_F(ElementFactoryTest, FromFactoryNameNotFoundTest)
{
  EXPECT_THROW(
    {
      auto factory = ElementFactory::fromFactoryName("doesNotExist");
    },
    std::runtime_error
  );
}

// Test getMetaData method
TEST_F(ElementFactoryTest, GetMetaDataTest)
{
  const auto factory = ElementFactory::fromFactoryName("fakesrc");

  const std::string longName = factory.getMetaData("long-name");
  ASSERT_FALSE(longName.empty()) << "Long name metadata should not be empty.";
}

// Test getMetadataKeys method
TEST_F(ElementFactoryTest, GetMetadataKeysTest)
{
  const auto factory = ElementFactory::fromFactoryName("fakesrc");

  const auto keys = factory.getMetadataKeys();
  ASSERT_FALSE(keys.empty()) << "Metadata keys should not be empty.";
  ASSERT_NE(std::find(keys.begin(), keys.end(), "long-name"), keys.end()) << "Metadata key 'long-name' not found.";
}

// Test element creation
TEST_F(ElementFactoryTest, CreateElementTest)
{
  const auto factory = ElementFactory::fromFactoryName("fakesrc");

  const std::string elementName("myElement");
  const auto element = factory.createElement(elementName);

  // Check the type of the created element
  EXPECT_EQ(element->getName(), elementName);
}

TEST_F(ElementFactoryTest, makeElementTest)
{
  const std::string elementName("myElement");
  const auto element = ElementFactory::makeElement("fakesrc", elementName);

  // Ensure the element was created
  EXPECT_EQ(element->getName(), elementName);
}

// Test the getElementType method
TEST_F(ElementFactoryTest, GetElementTypeTest)
{
  const auto factory = ElementFactory::fromFactoryName("fakesrc");

  const GType elementType = factory.getElementType();
  ASSERT_NE(elementType, 0) << "Element type should not be 0.";
}
