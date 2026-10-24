/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
#include "element.hpp"
#include "elementfactory.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>
#include "fatalgliblog.hpp"
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibCriticals);
#include <gst/gst.h>

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

BOOST_FIXTURE_TEST_CASE(SetStringProperty, ObjectPropertiesTest)
{
  auto element = Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::Floating);

  element->setProperty<std::string>("name", "renamed_source");
  BOOST_CHECK_EQUAL(element->getProperty<std::string>("name"), "renamed_source");
}

/*
 * Strict type checking for getProperty / setProperty (see todo.md #3).
 * ValueType must match the GType of the property exactly, otherwise std::invalid_argument is thrown.
 *
 * Properties used (all from GStreamer core elements):
 *   queue      max-size-time         guint64, read/write, default 1000000000
 *   queue      current-level-buffers guint,   read-only
 *   fakesrc    num-buffers           gint,    read/write, default -1
 *   fakesrc    is-live               gboolean, read/write, default false
 *   identity   drop-probability      gfloat,  read/write, default 0
 *   capsfilter caps                  GstCaps, read/write
 */

// --- matching types must keep working ---

BOOST_FIXTURE_TEST_CASE(GetPropertyMatchingTypes, ObjectPropertiesTest)
{
  auto queue = ElementFactory::makeElement("queue", "test_queue");
  BOOST_CHECK_EQUAL(queue->getProperty<guint64>("max-size-time"), 1000000000u);

  auto fakesrc = ElementFactory::makeElement("fakesrc", "test_source");
  BOOST_CHECK_EQUAL(fakesrc->getProperty<gint>("num-buffers"), -1);
  BOOST_CHECK_EQUAL(fakesrc->getProperty<bool>("is-live"), false);

  auto identity = ElementFactory::makeElement("identity", "test_identity");
  BOOST_CHECK_EQUAL(identity->getProperty<float>("drop-probability"), 0.0f);
}

BOOST_FIXTURE_TEST_CASE(SetPropertyMatchingTypes, ObjectPropertiesTest)
{
  auto queue = ElementFactory::makeElement("queue", "test_queue");
  queue->setProperty<guint64>("max-size-time", 5000000000u); // > 32 bit
  BOOST_CHECK_EQUAL(queue->getProperty<guint64>("max-size-time"), 5000000000u);

  auto fakesrc = ElementFactory::makeElement("fakesrc", "test_source");
  fakesrc->setProperty<gint>("num-buffers", 42);
  BOOST_CHECK_EQUAL(fakesrc->getProperty<gint>("num-buffers"), 42);
  fakesrc->setProperty<bool>("is-live", true);
  BOOST_CHECK_EQUAL(fakesrc->getProperty<bool>("is-live"), true);

  auto identity = ElementFactory::makeElement("identity", "test_identity");
  identity->setProperty<float>("drop-probability", 0.5f);
  BOOST_CHECK_EQUAL(identity->getProperty<float>("drop-probability"), 0.5f);
}

BOOST_FIXTURE_TEST_CASE(GetCapsPropertyReturnsOwningSharedPtr, ObjectPropertiesTest)
{
  auto capsfilter = ElementFactory::makeElement("capsfilter", "test_capsfilter");
  GstCaps* caps = gst_caps_from_string("video/x-raw,width=320");
  g_object_set(capsfilter->getGstElement().get(), "caps", caps, nullptr);

  const auto result = capsfilter->getProperty<GstCapsSPtr>("caps");
  BOOST_REQUIRE(result);
  BOOST_CHECK(gst_caps_is_equal(result.get(), caps));
  // must own its reference (released by the deleter), not be a raw pointer stuffed into a shared_ptr
  BOOST_CHECK_EQUAL(result.use_count(), 1);

  gst_caps_unref(caps);
}

// --- mismatching types must throw ---

BOOST_FIXTURE_TEST_CASE(GetPropertyNarrowerIntegerThrows, ObjectPropertiesTest)
{
  // int (4 bytes) for a guint64 property (8 bytes): currently overwrites the stack
  auto queue = ElementFactory::makeElement("queue", "test_queue");
  BOOST_CHECK_THROW((void)queue->getProperty<int>("max-size-time"), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(GetPropertySignednessMismatchThrows, ObjectPropertiesTest)
{
  auto fakesrc = ElementFactory::makeElement("fakesrc", "test_source");
  BOOST_CHECK_THROW((void)fakesrc->getProperty<guint>("num-buffers"), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(GetPropertyFloatAsDoubleThrows, ObjectPropertiesTest)
{
  auto identity = ElementFactory::makeElement("identity", "test_identity");
  BOOST_CHECK_THROW((void)identity->getProperty<double>("drop-probability"), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(GetPropertyIntAsStringThrows, ObjectPropertiesTest)
{
  // currently the int is written into a gchar* and dereferenced
  auto fakesrc = ElementFactory::makeElement("fakesrc", "test_source");
  BOOST_CHECK_THROW((void)fakesrc->getProperty<std::string>("num-buffers"), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(GetPropertyBooleanAsIntThrows, ObjectPropertiesTest)
{
  auto fakesrc = ElementFactory::makeElement("fakesrc", "test_source");
  BOOST_CHECK_THROW((void)fakesrc->getProperty<int>("is-live"), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(SetPropertyNarrowerIntegerThrows, ObjectPropertiesTest)
{
  auto queue = ElementFactory::makeElement("queue", "test_queue");
  BOOST_CHECK_THROW(queue->setProperty<int>("max-size-time", 10), std::invalid_argument);
  BOOST_CHECK_EQUAL(queue->getProperty<guint64>("max-size-time"), 1000000000u); // unchanged
}

BOOST_FIXTURE_TEST_CASE(SetPropertyDoubleForIntThrows, ObjectPropertiesTest)
{
  auto fakesrc = ElementFactory::makeElement("fakesrc", "test_source");
  BOOST_CHECK_THROW(fakesrc->setProperty<double>("num-buffers", 3.0), std::invalid_argument);
  BOOST_CHECK_EQUAL(fakesrc->getProperty<gint>("num-buffers"), -1); // unchanged
}

BOOST_FIXTURE_TEST_CASE(SetPropertyStringForIntThrows, ObjectPropertiesTest)
{
  auto fakesrc = ElementFactory::makeElement("fakesrc", "test_source");
  BOOST_CHECK_THROW(fakesrc->setProperty<std::string>("num-buffers", "3"), std::invalid_argument);
  BOOST_CHECK_EQUAL(fakesrc->getProperty<gint>("num-buffers"), -1); // unchanged
}

BOOST_FIXTURE_TEST_CASE(SetPropertyIntForBooleanThrows, ObjectPropertiesTest)
{
  auto fakesrc = ElementFactory::makeElement("fakesrc", "test_source");
  BOOST_CHECK_THROW(fakesrc->setProperty<int>("is-live", 1), std::invalid_argument);
}

// --- access flags ---

BOOST_FIXTURE_TEST_CASE(SetReadOnlyPropertyThrows, ObjectPropertiesTest)
{
  auto queue = ElementFactory::makeElement("queue", "test_queue");
  BOOST_CHECK_THROW(queue->setProperty<guint>("current-level-buffers", 5u), std::invalid_argument);
}

// --- unknown properties (already implemented, regression) ---

BOOST_FIXTURE_TEST_CASE(UnknownPropertyThrows, ObjectPropertiesTest)
{
  auto fakesrc = ElementFactory::makeElement("fakesrc", "test_source");
  BOOST_CHECK_THROW((void)fakesrc->getProperty<gint>("does-not-exist"), std::invalid_argument);
  BOOST_CHECK_THROW(fakesrc->setProperty<gint>("does-not-exist", 1), std::invalid_argument);
  BOOST_CHECK_THROW((void)fakesrc->getProperty<std::string>("does-not-exist"), std::invalid_argument);
  BOOST_CHECK_THROW(fakesrc->setProperty<std::string>("does-not-exist", "x"), std::invalid_argument);
}
