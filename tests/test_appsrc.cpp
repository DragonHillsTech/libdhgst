/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
#include "appsink.hpp"
#include "appsrc.hpp"
#include "pipeline.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>
#include "fatalgliblog.hpp"
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibCriticals);

#include <gst/gst.h>

#include <atomic>
#include <chrono>
#include <future>
#include <stdexcept>

using namespace dh::gst;

class AppSrcTest
{
public:
  AppSrcTest()
  {
    gst_init(nullptr, nullptr);
  }
};

namespace
{

constexpr auto waitTimeout = std::chrono::seconds(5);

/// a buffer of size bytes, all set to value
GstBufferSPtr createBuffer(gsize size, guint8 value)
{
  GstBuffer* buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
  gst_buffer_memset(buffer, 0, value, size);
  return makeGstSharedPtr(buffer, TransferType::full);
}

/// the first byte of the buffer of the sample
guint8 firstByte(const GstSampleSPtr& sample)
{
  guint8 value = 0;
  gst_buffer_extract(gst_sample_get_buffer(sample.get()), 0, &value, 1);
  return value;
}

GstCapsSPtr createCaps(const char* caps)
{
  return makeGstSharedPtr(gst_caps_from_string(caps), TransferType::full);
}

} // namespace

BOOST_FIXTURE_TEST_CASE(CreationAndDestruction, AppSrcTest)
{
  auto appSrc = AppSrc::create("src1");
  BOOST_CHECK_EQUAL(appSrc->getName(), "src1");
  BOOST_CHECK_EQUAL(appSrc->getFactoryName(), "appsrc");
  BOOST_CHECK_EQUAL(GST_OBJECT_REFCOUNT(appSrc->getGstAppSrc().get()), 2); // wrapper + returned shared_ptr

  auto unnamed = AppSrc::create();
  BOOST_CHECK(!unnamed->getName().empty());

  auto fromSPtr = AppSrc::create(appSrc->getGstAppSrc());
  BOOST_CHECK_EQUAL(fromSPtr->getGstAppSrc().get(), appSrc->getGstAppSrc().get());
}

BOOST_FIXTURE_TEST_CASE(FromElement, AppSrcTest)
{
  auto pipeline = Pipeline::fromDescription("appsrc name=src ! fakesink");
  BOOST_CHECK_EQUAL(AppSrc::fromElement(*pipeline->getElementByName("src"))->getName(), "src");

  auto fakeSrc = Pipeline::fromDescription("fakesrc name=src ! fakesink")->getElementByName("src");
  BOOST_CHECK_THROW((void)AppSrc::fromElement(*fakeSrc), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(Properties, AppSrcTest)
{
  auto appSrc = AppSrc::create();

  BOOST_CHECK(!appSrc->getCaps());
  const GstCapsSPtr caps = createCaps("application/x-test");
  appSrc->setCaps(caps);
  BOOST_REQUIRE(appSrc->getCaps());
  BOOST_CHECK(gst_caps_is_equal(appSrc->getCaps().get(), caps.get()));
  appSrc->setCaps(nullptr);
  BOOST_CHECK(!appSrc->getCaps());

  BOOST_CHECK_EQUAL(appSrc->getStreamType(), GST_APP_STREAM_TYPE_STREAM);
  appSrc->setStreamType(GST_APP_STREAM_TYPE_SEEKABLE);
  BOOST_CHECK_EQUAL(appSrc->getStreamType(), GST_APP_STREAM_TYPE_SEEKABLE);

  BOOST_CHECK_EQUAL(appSrc->getSize(), -1);
  appSrc->setSize(1000);
  BOOST_CHECK_EQUAL(appSrc->getSize(), 1000);

  appSrc->setMaxBytes(4096);
  BOOST_CHECK_EQUAL(appSrc->getMaxBytes(), 4096u);
  BOOST_CHECK_EQUAL(appSrc->getCurrentLevelBytes(), 0u);
}

BOOST_FIXTURE_TEST_CASE(PushInvalid, AppSrcTest)
{
  auto appSrc = AppSrc::create();
  BOOST_CHECK_THROW(appSrc->pushBuffer(nullptr), std::invalid_argument);
  BOOST_CHECK_THROW(appSrc->pushSample(nullptr), std::invalid_argument);

  // not started yet: the buffer is queued
  BOOST_CHECK_EQUAL(appSrc->pushBuffer(createBuffer(4, 1)), GST_FLOW_OK);
  BOOST_CHECK_EQUAL(appSrc->getCurrentLevelBytes(), 4u);
}

BOOST_FIXTURE_TEST_CASE(PushAfterStop, AppSrcTest)
{
  auto pipeline = Pipeline::fromDescription("appsrc name=src ! fakesink");
  auto appSrc = AppSrc::fromElement(*pipeline->getElementByName("src"));
  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_FAILURE);
  BOOST_REQUIRE_EQUAL(pipeline->setState(GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);

  // stopped: the buffer is dropped
  BOOST_CHECK_EQUAL(appSrc->pushBuffer(createBuffer(4, 1)), GST_FLOW_FLUSHING);
  BOOST_CHECK_EQUAL(appSrc->endOfStream(), GST_FLOW_FLUSHING);
}

BOOST_FIXTURE_TEST_CASE(PushBuffersToAppSink, AppSrcTest)
{
  auto pipeline = Pipeline::fromDescription("appsrc name=src ! appsink name=sink");
  auto appSrc = AppSrc::fromElement(*pipeline->getElementByName("src"));
  auto appSink = AppSink::fromElement(*pipeline->getElementByName("sink"));
  appSrc->setCaps(createCaps("application/x-test"));
  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_FAILURE);

  for(guint8 i = 1; i <= 3; ++i)
  {
    const GstBufferSPtr buffer = createBuffer(4, i);
    BOOST_CHECK_EQUAL(appSrc->pushBuffer(buffer), GST_FLOW_OK);
    BOOST_CHECK_EQUAL(buffer.use_count(), 1); // appsrc took its own reference, not ours
  }
  BOOST_CHECK_EQUAL(appSrc->endOfStream(), GST_FLOW_OK);
  BOOST_CHECK_EQUAL(appSrc->pushBuffer(createBuffer(4, 4)), GST_FLOW_EOS);

  for(guint8 i = 1; i <= 3; ++i)
  {
    GstSampleSPtr sample = appSink->tryPullSample(5 * GST_SECOND);
    BOOST_REQUIRE(sample);
    BOOST_CHECK_EQUAL(firstByte(sample), i);
  }
  BOOST_CHECK(!appSink->pullSample());
  BOOST_CHECK(appSink->isEos());

  pipeline->setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(PushSample, AppSrcTest)
{
  auto pipeline = Pipeline::fromDescription("appsrc name=src ! appsink name=sink");
  auto appSrc = AppSrc::fromElement(*pipeline->getElementByName("src"));
  auto appSink = AppSink::fromElement(*pipeline->getElementByName("sink"));
  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_FAILURE);

  const GstCapsSPtr caps = createCaps("application/x-sample");
  const GstBufferSPtr buffer = createBuffer(4, 7);
  const GstSampleSPtr sample = makeGstSharedPtr(
    gst_sample_new(buffer.get(), caps.get(), nullptr, nullptr),
    TransferType::full
  );
  BOOST_CHECK_EQUAL(appSrc->pushSample(sample), GST_FLOW_OK);

  GstSampleSPtr received = appSink->tryPullSample(5 * GST_SECOND);
  BOOST_REQUIRE(received);
  BOOST_CHECK_EQUAL(firstByte(received), 7);
  BOOST_CHECK(gst_caps_is_equal(gst_sample_get_caps(received.get()), caps.get()));
  // the caps of the sample are taken over by the appsrc
  BOOST_REQUIRE(appSrc->getCaps());
  BOOST_CHECK(gst_caps_is_equal(appSrc->getCaps().get(), caps.get()));

  pipeline->setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(NeedData, AppSrcTest)
{
  auto pipeline = Pipeline::fromDescription("appsrc name=src ! appsink name=sink");
  auto appSrc = AppSrc::fromElement(*pipeline->getElementByName("src"));
  auto appSink = AppSink::fromElement(*pipeline->getElementByName("sink"));
  appSrc->setCaps(createCaps("application/x-test"));

  // push 3 buffers, one per need-data call, then end the stream
  std::atomic<int> pushed{0};
  auto needDataConnection = appSrc->connectNeedData([&](guint /*length*/)
  {
    if(pushed < 3)
    {
      ++pushed;
      (void)appSrc->pushBuffer(createBuffer(4, static_cast<guint8>(pushed.load())));
    }
    else
    {
      (void)appSrc->endOfStream();
    }
  });

  std::atomic<int> received{0};
  std::promise<void> eos;
  auto sampleConnection = appSink->connectNewSample([&](GstSampleSPtr sample)
  {
    BOOST_CHECK_EQUAL(firstByte(sample), ++received);
  });
  auto eosConnection = appSink->connectEndOfStream([&]()
  {
    eos.set_value();
  });

  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_FAILURE);
  BOOST_REQUIRE(eos.get_future().wait_for(waitTimeout) == std::future_status::ready);
  pipeline->setState(GST_STATE_NULL);

  BOOST_CHECK_EQUAL(received, 3);
}

BOOST_FIXTURE_TEST_CASE(EnoughData, AppSrcTest)
{
  // the appsink blocks after one sample, so the queue of the appsrc fills up
  auto pipeline = Pipeline::fromDescription("appsrc name=src max-bytes=16 ! appsink name=sink max-buffers=1");
  auto appSrc = AppSrc::fromElement(*pipeline->getElementByName("src"));

  std::atomic<bool> enoughData{false};
  auto connection = appSrc->connectEnoughData([&]()
  {
    enoughData = true;
  });
  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_FAILURE);

  for(int i = 0; i < 100 && !enoughData; ++i)
  {
    BOOST_REQUIRE_EQUAL(appSrc->pushBuffer(createBuffer(8, 0)), GST_FLOW_OK);
  }
  BOOST_CHECK(enoughData);
  BOOST_CHECK_GE(appSrc->getCurrentLevelBytes(), 16u);

  pipeline->setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(SeekData, AppSrcTest)
{
  auto pipeline = Pipeline::fromDescription("appsrc name=src ! fakesink");
  auto appSrc = AppSrc::fromElement(*pipeline->getElementByName("src"));
  appSrc->setStreamType(GST_APP_STREAM_TYPE_SEEKABLE);
  appSrc->setSize(1000);

  BOOST_CHECK_THROW((void)appSrc->connectSeekData({}), std::invalid_argument);

  auto needDataConnection = appSrc->connectNeedData([&](guint /*length*/)
  {
    (void)appSrc->pushBuffer(createBuffer(10, 0));
  });
  std::promise<guint64> seekOffset;
  std::atomic<bool> seekReported{false};
  auto seekConnection = appSrc->connectSeekData([&](guint64 offset)
  {
    // the first seek-data call is for the start of the stream
    if(offset != 0 && !seekReported.exchange(true))
    {
      seekOffset.set_value(offset);
    }
    return true;
  });

  BOOST_REQUIRE_NE(pipeline->setState(GST_STATE_PAUSED), GST_STATE_CHANGE_FAILURE);
  BOOST_REQUIRE_EQUAL(pipeline->getState(5 * GST_SECOND), GST_STATE_PAUSED);
  BOOST_REQUIRE(gst_element_seek_simple(
    pipeline->getGstElement().get(), GST_FORMAT_BYTES, GST_SEEK_FLAG_FLUSH, 100
  ));

  auto future = seekOffset.get_future();
  BOOST_REQUIRE(future.wait_for(waitTimeout) == std::future_status::ready);
  BOOST_CHECK_EQUAL(future.get(), 100u);

  pipeline->setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(CallbacksSharedByWrappers, AppSrcTest)
{
  auto appSrc = AppSrc::create();
  auto otherWrapper = AppSrc::create(appSrc->getGstAppSrc());

  auto connection = appSrc->connectNeedData([](guint) {});
  BOOST_CHECK_THROW((void)otherWrapper->connectNeedData([](guint) {}), std::logic_error);
  // the other kinds have their own slots
  auto enoughConnection = otherWrapper->connectEnoughData([]() {});
  BOOST_CHECK(enoughConnection.connected());

  connection.disconnect();
  auto otherConnection = otherWrapper->connectNeedData([](guint) {});
  BOOST_CHECK(otherConnection.connected());
}
