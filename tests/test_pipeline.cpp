/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
#include "pipeline.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include "fatalgliblog.hpp"
#include <boost/test/included/unit_test.hpp>
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibCriticals);

#include <gst/gst.h>

using namespace dh::gst;

/**
 * @brief Test fixture for setting up and tearing down the test environment for Element class.
 */
class PipelineTest
{
public:
  // Setup before first test case
  PipelineTest()
  {
    gst_init(nullptr, nullptr); // Initialize GStreamer
  }
};

// Test the empty pipeline constructor
BOOST_FIXTURE_TEST_CASE(EmptyPipelineConstructorTest, PipelineTest)
{
  const std::string pipelineName = "test_pipeline";
  const auto pipeline = Pipeline::create(pipelineName);
  BOOST_REQUIRE_EQUAL(pipeline->getName(), pipelineName);
}

// --- create() checks the type of the C object, because GST_*_CAST does not ---

BOOST_FIXTURE_TEST_CASE(CreateWithWrongTypeThrowsAndReleasesTheObject, PipelineTest)
{
  // a GstBin is no GstPipeline
  GstElement* bin = gst_bin_new("test_bin");
  gst_object_ref_sink(bin);
  gst_object_ref(
    bin
  ); // the reference handed over below; the first one keeps it alive for the check
  BOOST_CHECK_THROW(
    (void)Pipeline::create(GST_PIPELINE_CAST(bin), TransferType::Full),
    std::invalid_argument
  );
  // the handed over reference was released again
  BOOST_CHECK_EQUAL(GST_OBJECT_REFCOUNT_VALUE(bin), 1);
  gst_object_unref(bin);
}

BOOST_FIXTURE_TEST_CASE(CreateBaseClassWithWrongTypeThrows, PipelineTest)
{
  // a fakesrc is no GstBin
  GstElement* element = gst_element_factory_make("fakesrc", nullptr);
  BOOST_CHECK_THROW(
    (void)Bin::create(GST_BIN_CAST(element), TransferType::Floating),
    std::invalid_argument
  );
}

BOOST_FIXTURE_TEST_CASE(CreateWithSubtypeIsAccepted, PipelineTest)
{
  // a GstPipeline is a GstBin
  GstElement* pipeline = gst_pipeline_new("test_pipeline");
  const auto bin = Bin::create(GST_BIN_CAST(pipeline), TransferType::Floating);
  BOOST_CHECK_EQUAL(bin->getName(), "test_pipeline");
}

// Test the fromDescription method with a valid pipeline description
BOOST_FIXTURE_TEST_CASE(FromDescriptionTest, PipelineTest)
{
  BOOST_REQUIRE_NO_THROW({ auto descPipeline = Pipeline::fromDescription("fakesrc ! fakesink"); });
}

// Test fromDescription with an invalid description (should throw)
BOOST_FIXTURE_TEST_CASE(FromDescriptionInvalidTest, PipelineTest)
{
  BOOST_REQUIRE_THROW(
    { auto pipeline = Pipeline::fromDescription("invalid_description"); },
    std::runtime_error
  );
}

// A partly parsed description must not return a broken pipeline
BOOST_FIXTURE_TEST_CASE(FromDescriptionUnknownElementThrows, PipelineTest)
{
  BOOST_CHECK_THROW(
    (void)Pipeline::fromDescription("fakesrc ! doesnotexist ! fakesink"),
    std::runtime_error
  );
}

BOOST_FIXTURE_TEST_CASE(FromDescriptionFailedLinkThrows, PipelineTest)
{
  // a sink has no source pad, so linking it to the next element fails
  BOOST_CHECK_THROW(
    (void)Pipeline::fromDescription("fakesrc ! fakesink name=a ! fakesink"),
    std::runtime_error
  );
}

// A single element is returned as that element, not as a pipeline; it must be released properly
BOOST_FIXTURE_TEST_CASE(FromDescriptionSingleElementThrows, PipelineTest)
{
  BOOST_CHECK_THROW((void)Pipeline::fromDescription("fakesrc"), std::runtime_error);
}

#if DH_GST_HAS_PIPELINE_IS_LIVE
BOOST_FIXTURE_TEST_CASE(IsLiveTest, PipelineTest)
{
  auto pipeline = Pipeline::create("pipeline1");

  // Initially, the pipeline should not be live.
  BOOST_REQUIRE(!pipeline->isLive());

  // Empty pipelines should not report as live after starting.
  pipeline->setState(GST_STATE_PLAYING);
  BOOST_REQUIRE(!pipeline->isLive());
  pipeline->setState(GST_STATE_NULL);

  auto livePipeline = Pipeline::fromDescription("videotestsrc is-live=true ! fakesink");

  BOOST_REQUIRE_EQUAL(livePipeline->setState(GST_STATE_PAUSED), GST_STATE_CHANGE_NO_PREROLL);
  BOOST_REQUIRE(livePipeline->isLive());
  livePipeline->setState(GST_STATE_NULL);
}
#endif
