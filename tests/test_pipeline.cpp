/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
#include "pipeline.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>

#include <gst/gst.h>

#include <cstdlib>

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
    // Set G_DEBUG to fatal_criticals to make critical warnings crash the program
    setenv("G_DEBUG", "fatal_criticals", 1);
    gst_init(nullptr, nullptr);  // Initialize GStreamer
  }
};

// Test the empty pipeline constructor
BOOST_FIXTURE_TEST_CASE(EmptyPipelineConstructorTest, PipelineTest)
{
  const std::string pipelineName = "test_pipeline";
  const auto pipeline = Pipeline::create(pipelineName);
  BOOST_REQUIRE_EQUAL(pipeline->getName(), pipelineName);
}

// Test the fromDescription method with a valid pipeline description
BOOST_FIXTURE_TEST_CASE(FromDescriptionTest, PipelineTest)
{
  BOOST_REQUIRE_NO_THROW(
    {
      auto descPipeline = Pipeline::fromDescription("fakesrc ! fakesink");
    }
  );
}

// Test fromDescription with an invalid description (should throw)
BOOST_FIXTURE_TEST_CASE(FromDescriptionInvalidTest, PipelineTest)
{
  BOOST_REQUIRE_THROW(
    {
      auto pipeline = Pipeline::fromDescription("invalid_description");
    }, std::runtime_error
  );
}

// Does not exist for older gstreamer. Enable when needed
// BOOST_FIXTURE_TEST_CASE(IsLiveTest, PipelineTest) {
//   Pipeline pipeline1("pipeline1");
//
//   // Initially, the pipeline should not be live
//   BOOST_REQUIRE(!pipeline1.isLive());
//
//   // Set the pipeline to the PLAYING state and check if it's live
//   pipeline1.setState(GST_STATE_PLAYING);
//   BOOST_REQUIRE(!pipeline1.isLive());
// }

