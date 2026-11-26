/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
#include "appsink.hpp"
#include "pipeline.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include "fatalgliblog.hpp"
#include <boost/test/included/unit_test.hpp>
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibCriticals);

#include <gst/gst.h>

#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>

using namespace dh::gst;

class AppSinkTest
{
public:
  AppSinkTest()
  {
    gst_init(nullptr, nullptr);
  }
};

namespace
{

constexpr auto waitTimeout = std::chrono::seconds(5);

std::shared_ptr<Pipeline> createPipeline(int numBuffers)
{
  return Pipeline::fromDescription(
    "videotestsrc num-buffers=" + std::to_string(numBuffers)
    + " ! video/x-raw,width=16,height=8 ! appsink name=sink"
  );
}

std::shared_ptr<AppSink> getAppSink(Pipeline& pipeline)
{
  return AppSink::fromElement(*pipeline.getElementByName("sink"));
}

} // namespace

BOOST_FIXTURE_TEST_CASE(CreationAndDestruction, AppSinkTest)
{
  auto appSink = AppSink::create("sink1");
  BOOST_CHECK_EQUAL(appSink->getName(), "sink1");
  BOOST_CHECK_EQUAL(appSink->getFactoryName(), "appsink");
  // wrapper + returned shared_ptr
  BOOST_CHECK_EQUAL(GST_OBJECT_REFCOUNT(appSink->getGstAppSink().get()), 2);

  auto unnamed = AppSink::create();
  BOOST_CHECK(!unnamed->getName().empty());

  auto fromSPtr = AppSink::create(appSink->getGstAppSink());
  BOOST_CHECK_EQUAL(fromSPtr->getGstAppSink().get(), appSink->getGstAppSink().get());
}

BOOST_FIXTURE_TEST_CASE(FromElement, AppSinkTest)
{
  auto pipeline = createPipeline(1);
  auto appSink = getAppSink(*pipeline);
  BOOST_CHECK_EQUAL(appSink->getName(), "sink");

  auto fakeSink =
    Pipeline::fromDescription("fakesrc ! fakesink name=sink")->getElementByName("sink");
  BOOST_CHECK_THROW((void)AppSink::fromElement(*fakeSink), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(Properties, AppSinkTest)
{
  auto appSink = AppSink::create();

  BOOST_CHECK(!appSink->getCaps());
  const GstCapsSPtr caps =
    makeGstSharedPtr(gst_caps_from_string("video/x-raw"), TransferType::full);
  appSink->setCaps(caps);
  BOOST_REQUIRE(appSink->getCaps());
  BOOST_CHECK(gst_caps_is_equal(appSink->getCaps().get(), caps.get()));
  appSink->setCaps(nullptr);
  BOOST_CHECK(!appSink->getCaps());

  appSink->setMaxBuffers(3);
  BOOST_CHECK_EQUAL(appSink->getMaxBuffers(), 3u);
}

BOOST_FIXTURE_TEST_CASE(PullSamples, AppSinkTest)
{
  auto pipeline = createPipeline(3);
  auto appSink = getAppSink(*pipeline);
  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_FAILURE);

  for (int i = 0; i < 3; ++i)
  {
    GstSampleSPtr sample = appSink->tryPullSample(5 * GST_SECOND);
    BOOST_REQUIRE(sample);
    BOOST_CHECK(gst_sample_get_buffer(sample.get()));
    BOOST_CHECK(gst_sample_get_caps(sample.get()));
  }
  // end-of-stream: pullSample does not block any more
  BOOST_CHECK(!appSink->pullSample());
  BOOST_CHECK(appSink->isEos());

  pipeline->setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(PullPreroll, AppSinkTest)
{
  auto pipeline = createPipeline(3);
  auto appSink = getAppSink(*pipeline);
  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PAUSED), GST_STATE_CHANGE_FAILURE);

  BOOST_CHECK(appSink->pullPreroll());
  // the preroll sample is consumed, the next one comes after a flush or state change
  BOOST_CHECK(!appSink->tryPullPreroll(0));
  // but it is delivered as the first sample when playing
  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_FAILURE);
  BOOST_CHECK(appSink->tryPullSample(5 * GST_SECOND));

  pipeline->setState(GST_STATE_NULL);
  BOOST_CHECK(!appSink->tryPullSample(0)); // not PAUSED or PLAYING
}

BOOST_FIXTURE_TEST_CASE(Callbacks, AppSinkTest)
{
  auto pipeline = createPipeline(3);
  auto appSink = getAppSink(*pipeline);

  std::atomic<int> sampleCount{0};
  std::atomic<int> prerollCount{0};
  std::promise<void> eos;

  auto sampleConnection = appSink->connectNewSample(
    [&](GstSampleSPtr sample)
    {
      BOOST_CHECK(sample);
      ++sampleCount;
    }
  );
  auto prerollConnection = appSink->connectNewPreroll(
    [&](GstSampleSPtr sample)
    {
      BOOST_CHECK(sample);
      ++prerollCount;
    }
  );
  auto eosConnection = appSink->connectEndOfStream(
    [&]()
    {
      eos.set_value();
    }
  );

  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_FAILURE);
  BOOST_REQUIRE(eos.get_future().wait_for(waitTimeout) == std::future_status::ready);
  pipeline->setState(GST_STATE_NULL);

  BOOST_CHECK_EQUAL(sampleCount, 3);
  BOOST_CHECK_EQUAL(prerollCount, 1);
}

BOOST_FIXTURE_TEST_CASE(OneCallbackPerAppSink, AppSinkTest)
{
  auto pipeline = createPipeline(1);
  auto appSink = getAppSink(*pipeline);
  auto otherWrapper = getAppSink(*pipeline);

  BOOST_CHECK_THROW((void)appSink->connectNewSample({}), std::invalid_argument);

  auto connection = appSink->connectNewSample(
    [](GstSampleSPtr)
    {
    }
  );
  BOOST_CHECK(connection.connected());
  // the callbacks belong to the GstAppSink, not to the wrapper
  BOOST_CHECK_THROW(
    (void)otherWrapper->connectNewSample(
      [](GstSampleSPtr)
      {
      }
    ),
    std::logic_error
  );

  connection.disconnect();
  BOOST_CHECK(!connection.connected());
  auto otherConnection = otherWrapper->connectNewSample(
    [](GstSampleSPtr)
    {
    }
  );
  BOOST_CHECK(otherConnection.connected());
}

BOOST_FIXTURE_TEST_CASE(SamplesQueuedWithoutCallback, AppSinkTest)
{
  auto pipeline = createPipeline(2);
  auto appSink = getAppSink(*pipeline);

  // only an end-of-stream callback: the new-sample callback must not take the samples
  std::promise<void> eos;
  auto eosConnection = appSink->connectEndOfStream(
    [&]()
    {
      eos.set_value();
    }
  );

  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_FAILURE);

  BOOST_CHECK(appSink->tryPullSample(5 * GST_SECOND));
  BOOST_CHECK(appSink->tryPullSample(5 * GST_SECOND));
  // with wait-on-eos (default), end-of-stream is only reported after all samples were pulled
  BOOST_REQUIRE(eos.get_future().wait_for(waitTimeout) == std::future_status::ready);
  BOOST_CHECK(!appSink->tryPullSample(0));

  pipeline->setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(ConnectionOutlivesAppSink, AppSinkTest)
{
  SignalConnection connection;
  {
    auto appSink = AppSink::create();
    connection = appSink->connectEndOfStream(
      []()
      {
      }
    );
    BOOST_CHECK(connection.connected());
  }
  // the GstAppSink and its callback slots are gone
  BOOST_CHECK(!connection.connected());
  connection.disconnect();
}

BOOST_FIXTURE_TEST_CASE(ReinstallAfterCallbacksReplaced, AppSinkTest)
{
  auto pipeline = createPipeline(1);
  auto appSink = getAppSink(*pipeline);

  auto connection = appSink->connectNewSample(
    [](GstSampleSPtr)
    {
    }
  );
  connection.disconnect();

  // own callbacks replace ours
  GstAppSinkCallbacks callbacks{};
  gst_app_sink_set_callbacks(appSink->getGstAppSink().get(), &callbacks, nullptr, nullptr);

  // connecting again installs ours again
  std::promise<void> sampleReceived;
  connection = appSink->connectNewSample(
    [&](GstSampleSPtr)
    {
      sampleReceived.set_value();
    }
  );
  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_FAILURE);
  BOOST_CHECK(sampleReceived.get_future().wait_for(waitTimeout) == std::future_status::ready);
  pipeline->setState(GST_STATE_NULL);
}
