/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
#include "pipeline.hpp"

#include <gtest/gtest.h>

#include <gst/gst.h>

#include <cstdlib>

using namespace dh::gst;

/**
 * @brief Test fixture for setting up and tearing down the test environment for Element class.
 */
class PipelineTest : public ::testing::Test
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

// Test the empty pipeline constructor
TEST_F(PipelineTest, EmptyPipelineConstructorTest)
{
  const std::string pipelineName = "test_pipeline";
  const auto pipeline = Pipeline::create(pipelineName);
  ASSERT_EQ(pipeline->getName(), pipelineName) << "Pipeline name mismatch.";
}

// Test the fromDescription method with a valid pipeline description
TEST_F(PipelineTest, FromDescriptionTest)
{
  ASSERT_NO_THROW(
    {
      auto descPipeline = Pipeline::fromDescription("fakesrc ! fakesink");
    }
  );
}

// Test fromDescription with an invalid description (should throw)
TEST_F(PipelineTest, FromDescriptionInvalidTest)
{
  ASSERT_THROW(
    {
      auto pipeline = Pipeline::fromDescription("invalid_description");
    }, std::runtime_error
  ) << "Expected std::runtime_error for invalid pipeline description.";
}

// Does not exist for older gstreamer. Enable when needed
// TEST_F(PipelineTest, IsLiveTest) {
//   Pipeline pipeline1("pipeline1");
//
//   // Initially, the pipeline should not be live
//   ASSERT_FALSE(pipeline1.isLive()) << "Pipeline should not be live initially.";
//
//   // Set the pipeline to the PLAYING state and check if it's live
//   pipeline1.setState(GST_STATE_PLAYING);
//   ASSERT_FALSE(pipeline1.isLive()) << "Pipeline should not be live after starting unless configured to be live.";
// }
