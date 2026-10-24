/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
#include "bin.hpp"
#include "bus.hpp"
#include "element.hpp"
#include "elementfactory.hpp"
#include "pipeline.hpp"
#include "pluginfeature.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>
#include "fatalgliblog.hpp"
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibCriticals);
#include <gst/gst.h>

#include <stdexcept>

using namespace dh::gst;

/**
 * @brief Wrappers must reject GStreamer objects of the wrong GType instead of silently
 * reinterpreting them (e.g. a plain GstElement passed as GstBin*).
 * The reinterpret_casts simulate such mistakes, the checked GST_BIN() etc. macros would
 * only emit a critical warning.
 */
class ObjectTypeChecksTest
{
public:
  ObjectTypeChecksTest()
  {
    gst_init(nullptr, nullptr);
  }

  /// a new element with a regular (non-floating) reference owned by the caller
  static GstElement* makeSunkElement(const char* factoryName)
  {
    return static_cast<GstElement*>(gst_object_ref_sink(gst_element_factory_make(factoryName, nullptr)));
  }

  static int refCount(gpointer object)
  {
    return GST_OBJECT_REFCOUNT_VALUE(object);
  }
};

BOOST_FIXTURE_TEST_CASE(ElementRejectsNonElement, ObjectTypeChecksTest)
{
  GstBus* bus = gst_bus_new();
  const int baseline = refCount(bus);

  BOOST_CHECK_THROW((void)Element::create(reinterpret_cast<GstElement*>(bus), TransferType::None), std::invalid_argument);
  BOOST_CHECK_THROW(
    (void)Element::create(makeGstSharedPtr(reinterpret_cast<GstElement*>(bus), TransferType::None)),
    std::invalid_argument
  );
  BOOST_CHECK_EQUAL(refCount(bus), baseline); // the reference taken by the wrapper is released again

  gst_object_unref(bus);
}

BOOST_FIXTURE_TEST_CASE(BinRejectsNonBin, ObjectTypeChecksTest)
{
  GstElement* fakesrc = makeSunkElement("fakesrc");
  BOOST_REQUIRE(fakesrc);
  const int baseline = refCount(fakesrc);

  BOOST_CHECK_THROW((void)Bin::create(reinterpret_cast<GstBin*>(fakesrc), TransferType::None), std::invalid_argument);
  BOOST_CHECK_THROW(
    (void)Bin::create(makeGstSharedPtr(reinterpret_cast<GstBin*>(fakesrc), TransferType::None)),
    std::invalid_argument
  );
  BOOST_CHECK_EQUAL(refCount(fakesrc), baseline);

  gst_object_unref(fakesrc);
}

BOOST_FIXTURE_TEST_CASE(PipelineRejectsNonPipeline, ObjectTypeChecksTest)
{
  // a GstBin is an element and a bin, but not a pipeline
  GstElement* bin = static_cast<GstElement*>(gst_object_ref_sink(gst_bin_new("test_bin")));
  const int baseline = refCount(bin);

  BOOST_CHECK_THROW(
    (void)Pipeline::create(reinterpret_cast<GstPipeline*>(bin), TransferType::None),
    std::invalid_argument
  );
  BOOST_CHECK_THROW(
    (void)Pipeline::create(makeGstSharedPtr(reinterpret_cast<GstPipeline*>(bin), TransferType::None)),
    std::invalid_argument
  );
  BOOST_CHECK_EQUAL(refCount(bin), baseline);

  gst_object_unref(bin);
}

BOOST_FIXTURE_TEST_CASE(BusRejectsNonBus, ObjectTypeChecksTest)
{
  GstElement* fakesrc = makeSunkElement("fakesrc");
  BOOST_REQUIRE(fakesrc);
  const int baseline = refCount(fakesrc);

  BOOST_CHECK_THROW((void)Bus::create(reinterpret_cast<GstBus*>(fakesrc), TransferType::None), std::invalid_argument);
  BOOST_CHECK_THROW(
    (void)Bus::create(makeGstSharedPtr(reinterpret_cast<GstBus*>(fakesrc), TransferType::None)),
    std::invalid_argument
  );
  BOOST_CHECK_EQUAL(refCount(fakesrc), baseline);

  gst_object_unref(fakesrc);
}

BOOST_FIXTURE_TEST_CASE(ElementFactoryRejectsNonElementFactory, ObjectTypeChecksTest)
{
  GstElement* fakesrc = makeSunkElement("fakesrc");
  BOOST_REQUIRE(fakesrc);
  const int baseline = refCount(fakesrc);

  BOOST_CHECK_THROW(
    (void)ElementFactory::create(reinterpret_cast<GstElementFactory*>(fakesrc), TransferType::None),
    std::invalid_argument
  );
  BOOST_CHECK_THROW(
    (void)ElementFactory::create(makeGstSharedPtr(reinterpret_cast<GstElementFactory*>(fakesrc), TransferType::None)),
    std::invalid_argument
  );
  BOOST_CHECK_EQUAL(refCount(fakesrc), baseline);

  gst_object_unref(fakesrc);
}

BOOST_FIXTURE_TEST_CASE(PluginFeatureRejectsNonPluginFeature, ObjectTypeChecksTest)
{
  GstElement* fakesrc = makeSunkElement("fakesrc");
  BOOST_REQUIRE(fakesrc);
  const int baseline = refCount(fakesrc);

  BOOST_CHECK_THROW(
    (void)PluginFeature::create(reinterpret_cast<GstPluginFeature*>(fakesrc), TransferType::None),
    std::invalid_argument
  );
  BOOST_CHECK_THROW(
    (void)PluginFeature::create(makeGstSharedPtr(reinterpret_cast<GstPluginFeature*>(fakesrc), TransferType::None)),
    std::invalid_argument
  );
  BOOST_CHECK_EQUAL(refCount(fakesrc), baseline);

  gst_object_unref(fakesrc);
}

BOOST_FIXTURE_TEST_CASE(MatchingTypesAreAccepted, ObjectTypeChecksTest)
{
  // subclasses are fine: a pipeline is a bin and an element
  auto pipeline = Pipeline::create("test_pipeline");
  GstElement* rawPipeline = pipeline->getGstElement().get();

  BOOST_CHECK_NO_THROW((void)Element::create(rawPipeline, TransferType::None));
  BOOST_CHECK_NO_THROW((void)Bin::create(GST_BIN_CAST(rawPipeline), TransferType::None));
  BOOST_CHECK_NO_THROW((void)Bus::create(pipeline->getBus()->getGstBus()));

  GstElementFactory* factory = gst_element_factory_find("fakesrc");
  BOOST_REQUIRE(factory);
  BOOST_CHECK_NO_THROW((void)ElementFactory::create(factory, TransferType::None));
  BOOST_CHECK_NO_THROW((void)PluginFeature::create(GST_PLUGIN_FEATURE_CAST(factory), TransferType::None));
  gst_object_unref(factory);
}
