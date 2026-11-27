/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
#include "appsrc.hpp"
#include "bus.hpp"
#include "element.hpp"
#include "elementfactory.hpp"
#include "pipeline.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include "fatalgliblog.hpp"
#include <boost/test/included/unit_test.hpp>
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibCriticals);
#include <glib/gstdio.h>
#include <gst/gst.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <future>
#include <ios>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

using namespace dh::gst;

/**
 * @brief Test fixture for setting up and tearing down the test environment for Element class.
 */
class ElementTest
{
public:
  // Setup before first test case
  ElementTest()
  {
    gst_init(nullptr, nullptr); // Initialize GStreamer
  }
};

BOOST_FIXTURE_TEST_CASE(CreationAndDestruction, ElementTest)
{
  // ctor GstElementSPtr
  auto element1 = Element::create(
    makeGstSharedPtr(gst_element_factory_make("fakesrc", "testSource1"), TransferType::floating)
  );
  BOOST_REQUIRE_EQUAL(
    element1->getGstElement().use_count(),
    1
  ); // getGstElement creates a new shared_ptr, so the count must be 1
  BOOST_REQUIRE_EQUAL(
    GST_OBJECT_REFCOUNT(element1->getGstElement().get()),
    2
  ); // getGstElement creates a new shared_ptr, so the GstObject must have increased
  BOOST_REQUIRE_EQUAL(element1->getFactoryName(), "fakesrc");

  // ctor GstElement*
  auto element2 =
    Element::create(gst_element_factory_make("fakesrc", "testSource2"), TransferType::floating);
  BOOST_REQUIRE_EQUAL(
    element2->getGstElement().use_count(),
    1
  ); // getGstElement creates a new shared_ptr, so the count must be 1
  BOOST_REQUIRE_EQUAL(
    GST_OBJECT_REFCOUNT(element2->getGstElement().get()),
    2
  ); // getGstElement creates a new shared_ptr, so the GstObject must have increased
}

BOOST_FIXTURE_TEST_CASE(GetNameReturnsCorrectName, ElementTest)
{
  const auto element =
    Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::floating);

  // Check that getName returns the correct name
  BOOST_CHECK_EQUAL(element->getName(), "test_source");
}

BOOST_FIXTURE_TEST_CASE(SetState, ElementTest)
{
  auto element =
    Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::floating);

  BOOST_REQUIRE_EQUAL(element->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_SUCCESS);
  BOOST_REQUIRE_EQUAL(element->getState(), GST_STATE_PLAYING);

  // the owner must set the element to NULL before it is destroyed
  BOOST_REQUIRE_EQUAL(element->setState(GST_STATE_NULL), GST_STATE_CHANGE_SUCCESS);
}

BOOST_FIXTURE_TEST_CASE(GetStateDoesNotBlockDuringAsyncStateChange, ElementTest)
{
  // a sink without data never prerolls, so the change to PAUSED stays pending
  auto pipeline = Pipeline::create("test_pipeline");
  pipeline->addElement(*ElementFactory::makeElement("fakesink", "test_sink"));
  BOOST_REQUIRE_EQUAL(pipeline->setState(GST_STATE_PAUSED), GST_STATE_CHANGE_ASYNC);

  std::promise<GstState> result;
  auto future = result.get_future();
  std::thread queryThread(
    [&]()
    {
      result.set_value(pipeline->getState());
    }
  );
  const bool returned = future.wait_for(std::chrono::seconds(2)) == std::future_status::ready;

  // aborts the pending state change, this also wakes up a getState() that is still waiting
  pipeline->setState(GST_STATE_NULL);
  queryThread.join();

  BOOST_REQUIRE(returned);
  BOOST_CHECK_EQUAL(future.get(), GST_STATE_READY); // the current state, PAUSED is still pending
}

BOOST_FIXTURE_TEST_CASE(QueryStateDuringAsyncStateChange, ElementTest)
{
  auto pipeline = Pipeline::create("test_pipeline");
  pipeline->addElement(*ElementFactory::makeElement("fakesink", "test_sink"));
  BOOST_REQUIRE_EQUAL(pipeline->setState(GST_STATE_PAUSED), GST_STATE_CHANGE_ASYNC);

  const auto query = pipeline->queryState();
  BOOST_CHECK_EQUAL(query.result, GST_STATE_CHANGE_ASYNC);
  BOOST_CHECK_EQUAL(query.current, GST_STATE_READY);
  BOOST_CHECK_EQUAL(query.pending, GST_STATE_PAUSED);

  // a timeout waits, but returns when the change does not complete
  const auto start = std::chrono::steady_clock::now();
  BOOST_CHECK_EQUAL(pipeline->queryState(100 * GST_MSECOND).result, GST_STATE_CHANGE_ASYNC);
  BOOST_CHECK(std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(90));

  pipeline->setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(GetStateThrowsOnFailedStateChange, ElementTest)
{
  auto filesrc = ElementFactory::makeElement("filesrc", "test_filesrc");
  filesrc->setProperty<std::string>("location", "/this/file/does/not/exist");
  BOOST_REQUIRE_EQUAL(filesrc->setState(GST_STATE_PAUSED), GST_STATE_CHANGE_FAILURE);

  BOOST_CHECK_THROW((void)filesrc->getState(), std::runtime_error);
  BOOST_CHECK_EQUAL(filesrc->queryState().result, GST_STATE_CHANGE_FAILURE);

  filesrc->setState(GST_STATE_NULL);
}

namespace
{

constexpr GstClockTime asyncDoneTimeout{5 * GST_SECOND};

void waitForAsyncDone(Pipeline& pipeline)
{
  BOOST_REQUIRE(pipeline.getBus()->timedPop(asyncDoneTimeout, GST_MESSAGE_ASYNC_DONE));
}

/**
 * @brief Creates an empty file with a unique name, so parallel test runs do not share it.
 */
std::filesystem::path createUniqueTemporaryFile()
{
  gchar* rawName{nullptr};
  GError* rawError{nullptr};
  const gint fileDescriptor{g_file_open_tmp("libdhgst_test_element-XXXXXX", &rawName, &rawError)};
  const auto error = makeGlibSharedPtr(rawError, TransferType::full);
  if (fileDescriptor == -1)
  {
    BOOST_FAIL("g_file_open_tmp failed: " << error->message);
  }
  g_close(fileDescriptor, nullptr);

  const std::unique_ptr<gchar, decltype(&g_free)> name{rawName, &g_free};
  return std::filesystem::path{name.get()};
}

/**
 * @brief A file with the given number of bytes that is removed again when the test ends.
 */
class TemporaryFile
{
public:
  explicit TemporaryFile(std::size_t size)
  {
    std::ofstream file{path, std::ios::binary};
    file << std::string(size, '\0');
    if (!file)
    {
      // the destructor does not run when the constructor fails
      std::filesystem::remove(path);
      BOOST_FAIL("cannot write " << path);
    }
  }

  ~TemporaryFile()
  {
    std::filesystem::remove(path);
  }

  TemporaryFile(const TemporaryFile&) = delete;
  TemporaryFile& operator=(const TemporaryFile&) = delete;
  TemporaryFile(TemporaryFile&&) = delete;
  TemporaryFile& operator=(TemporaryFile&&) = delete;

  const std::filesystem::path path{createUniqueTemporaryFile()};
};

} // namespace

BOOST_FIXTURE_TEST_CASE(QueriesFailBeforePreroll, ElementTest)
{
  auto pipeline = Pipeline::fromDescription("videotestsrc ! fakesink");

  BOOST_CHECK(!pipeline->queryPosition());
  BOOST_CHECK(!pipeline->queryDuration());
  BOOST_CHECK_THROW(pipeline->seek(GST_SECOND), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(QuerySeekingFailsWithoutLinkedPads, ElementTest)
{
  const auto identity = ElementFactory::makeElement("identity", "test_identity");

  BOOST_CHECK(!identity->querySeeking());
}

BOOST_FIXTURE_TEST_CASE(SeekThrowsIfStreamIsNotSeekable, ElementTest)
{
  // appsrc defaults to GST_APP_STREAM_TYPE_STREAM, which cannot be seeked
  auto pipeline = Pipeline::fromDescription("appsrc name=src format=time ! fakesink");
  auto appSrc = AppSrc::fromElement(*pipeline->getElementByName("src"));
  BOOST_REQUIRE_EQUAL(pipeline->setState(GST_STATE_PAUSED), GST_STATE_CHANGE_ASYNC);
  GstBuffer* buffer{gst_buffer_new_allocate(nullptr, 1, nullptr)};
  GST_BUFFER_PTS(buffer) = 0;
  BOOST_REQUIRE_EQUAL(
    appSrc->pushBuffer(makeGstSharedPtr(buffer, TransferType::full)),
    GST_FLOW_OK
  );
  waitForAsyncDone(*pipeline);

  const auto seekRange = pipeline->querySeeking();
  BOOST_REQUIRE(seekRange);
  BOOST_CHECK(!seekRange->seekable);
  BOOST_CHECK_THROW(pipeline->seek(GST_SECOND), std::runtime_error);

  pipeline->setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(QueryDurationAndSeekingOfFile, ElementTest)
{
  constexpr std::size_t fileSize{1000};
  const TemporaryFile file{fileSize};
  auto pipeline =
    Pipeline::fromDescription("filesrc location=" + file.path.string() + " ! fakesink");
  BOOST_REQUIRE_EQUAL(pipeline->setState(GST_STATE_PAUSED), GST_STATE_CHANGE_ASYNC);
  waitForAsyncDone(*pipeline);

  const auto duration = pipeline->queryDuration(GST_FORMAT_BYTES);
  BOOST_REQUIRE(duration);
  BOOST_CHECK_EQUAL(*duration, fileSize);

  const auto seekRange = pipeline->querySeeking(GST_FORMAT_BYTES);
  BOOST_REQUIRE(seekRange);
  BOOST_CHECK(seekRange->seekable);
  BOOST_CHECK_EQUAL(seekRange->start, 0);
  BOOST_CHECK_EQUAL(seekRange->end, fileSize);

  pipeline->setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(SeekChangesPosition, ElementTest)
{
  auto pipeline = Pipeline::fromDescription("videotestsrc ! fakesink");
  BOOST_REQUIRE_EQUAL(pipeline->setState(GST_STATE_PAUSED), GST_STATE_CHANGE_ASYNC);
  waitForAsyncDone(*pipeline);

  // an endless stream answers the duration query with -1
  BOOST_CHECK(!pipeline->queryDuration());

  const auto seekRange = pipeline->querySeeking();
  BOOST_REQUIRE(seekRange);
  BOOST_CHECK(seekRange->seekable);

  BOOST_CHECK(pipeline->queryPosition() == std::optional<std::int64_t>{0});
  pipeline->seek(GST_SECOND);
  waitForAsyncDone(*pipeline);
  BOOST_CHECK(pipeline->queryPosition() == std::optional<std::int64_t>{GST_SECOND});

  pipeline->setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(SendEndOfStreamPostsEosMessage, ElementTest)
{
  auto pipeline = Pipeline::fromDescription("videotestsrc ! fakesink");
  BOOST_REQUIRE_EQUAL(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_ASYNC);
  // ASYNC_DONE already comes after preroll, an EOS sent before PLAYING deadlocks
  BOOST_REQUIRE_EQUAL(pipeline->getState(asyncDoneTimeout), GST_STATE_PLAYING);

  pipeline->sendEndOfStream();
  BOOST_CHECK(pipeline->getBus()->timedPop(asyncDoneTimeout, GST_MESSAGE_EOS));

  pipeline->setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(SendEventKeepsCallerReference, ElementTest)
{
  const auto event = makeGstSharedPtr(gst_event_new_eos(), TransferType::full);
  {
    // in PLAYING: GStreamer never releases an EOS that a source receives in NULL
    auto pipeline = Pipeline::fromDescription("videotestsrc ! fakesink");
    BOOST_REQUIRE_EQUAL(pipeline->setState(GST_STATE_PLAYING), GST_STATE_CHANGE_ASYNC);
    BOOST_REQUIRE_EQUAL(pipeline->getState(asyncDoneTimeout), GST_STATE_PLAYING);
    pipeline->sendEvent(event);
    pipeline->setState(GST_STATE_NULL);
  }
  BOOST_CHECK_EQUAL(GST_MINI_OBJECT_REFCOUNT_VALUE(event.get()), 1);
}

BOOST_FIXTURE_TEST_CASE(SendEventThrowsIfNotHandled, ElementTest)
{
  // in NULL state, the sink pad of identity is inactive and rejects the event
  const auto identity = ElementFactory::makeElement("identity", "test_identity");

  BOOST_CHECK_THROW(identity->sendEndOfStream(), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(SendEventThrowsOnNullptr, ElementTest)
{
  const auto identity = ElementFactory::makeElement("identity", "test_identity");

  BOOST_CHECK_THROW(identity->sendEvent(nullptr), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(GetPads, ElementTest)
{
  auto element =
    Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::floating);

  std::vector<GstPadSPtr> pads = element->getPads();
  BOOST_REQUIRE_EQUAL(pads.size(), 1); // fakesrc has one "src" pad
}

BOOST_FIXTURE_TEST_CASE(GetSinkPads, ElementTest)
{
  auto element =
    Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::floating);

  std::vector<GstPadSPtr> sinkPads = element->getSinkPads();
  BOOST_REQUIRE_EQUAL(sinkPads.size(), 0); // fakesrc has no sink pads
}

BOOST_FIXTURE_TEST_CASE(GetSrcPads, ElementTest)
{
  auto element =
    Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::floating);

  std::vector<GstPadSPtr> srcPads = element->getSrcPads();
  BOOST_REQUIRE_EQUAL(srcPads.size(), 1); // fakesrc has one "src" pad
  gchar* padName = gst_pad_get_name(srcPads[0].get()); // transfer full
  BOOST_CHECK_EQUAL(padName, std::string("src")); // Verify the pad name
  g_free(padName);
}

BOOST_FIXTURE_TEST_CASE(GetPadsReferencesPads, ElementTest)
{
  auto element =
    Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::floating);
  GstPad* pad =
    gst_element_get_static_pad(element->getGstElement().get(), "src"); // our own reference
  BOOST_REQUIRE_NE(pad, nullptr);
  const auto refCount = [pad]() -> int
  {
    return GST_OBJECT_REFCOUNT_VALUE(pad);
  };
  const int baseline = refCount();

  {
    const auto pads = element->getPads();
    const auto srcPads = element->getSrcPads();
    BOOST_CHECK_EQUAL(refCount(), baseline + 2); // one reference per vector
  }
  BOOST_CHECK_EQUAL(refCount(), baseline); // released again, no leak

  gst_object_unref(pad);
}

BOOST_FIXTURE_TEST_CASE(GetStaticPad, ElementTest)
{
  auto element =
    Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::floating);

  const auto staticPad = element->getStaticPad("src");
  BOOST_REQUIRE(staticPad); // fakesrc has a "src" pad
  BOOST_CHECK_EQUAL(GST_OBJECT_NAME(staticPad.get()), std::string("src")); // Verify the pad name

  const auto refCount = [&staticPad]() -> int
  {
    return GST_OBJECT_REFCOUNT_VALUE(staticPad.get());
  };
  const int baseline = refCount();
  {
    const auto samePad = element->getStaticPad("src");
    BOOST_CHECK_EQUAL(samePad, staticPad);
    BOOST_CHECK_EQUAL(refCount(), baseline + 1);
  }
  BOOST_CHECK_EQUAL(refCount(), baseline); // released again, no leak

  BOOST_CHECK(!element->getStaticPad("does_not_exist"));
}

BOOST_FIXTURE_TEST_CASE(GetCompatiblePad, ElementTest)
{
  auto element =
    Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::floating);

  const auto sinkPad = makeGstSharedPtr(gst_pad_new("sink", GST_PAD_SINK), TransferType::floating);
  // Create capabilities for the sink pad
  const auto caps = makeGstSharedPtr(
    gst_caps_new_simple(
      "video/x-raw",
      "format",
      G_TYPE_STRING,
      "I420",
      "width",
      G_TYPE_INT,
      640,
      "height",
      G_TYPE_INT,
      480,
      "framerate",
      GST_TYPE_FRACTION,
      30,
      1,
      nullptr
    ),
    TransferType::full
  );
  BOOST_REQUIRE(caps);

  const auto srcPad = element->getStaticPad("src");
  const auto refCount = [&srcPad]() -> int
  {
    return GST_OBJECT_REFCOUNT_VALUE(srcPad.get());
  };
  const int baseline = refCount();
  {
    const auto resultPad = element->getCompatiblePad(sinkPad, caps);
    BOOST_REQUIRE_EQUAL(resultPad, srcPad);
    BOOST_CHECK_EQUAL(refCount(), baseline + 1);
  }
  BOOST_CHECK_EQUAL(refCount(), baseline); // released again, no leak

  // caps are optional
  BOOST_CHECK_EQUAL(element->getCompatiblePad(sinkPad), srcPad);
  BOOST_CHECK_THROW((void)element->getCompatiblePad(nullptr), std::invalid_argument);
}

BOOST_FIXTURE_TEST_CASE(LinkAndUnlink, ElementTest)
{
  auto element =
    Element::create(gst_element_factory_make("fakesrc", "test_source"), TransferType::floating);

  auto sink =
    Element::create(gst_element_factory_make("fakesink", "test_sink"), TransferType::floating);

  const auto sourcePad = element->getStaticPad("src");
  BOOST_REQUIRE(sourcePad);

  const auto sinkPad = sink->getStaticPad("sink");
  BOOST_REQUIRE(sinkPad);

  element->link(*sink);
  // Check if elements are linked
  BOOST_REQUIRE(gst_pad_is_linked(sourcePad.get()));
  BOOST_REQUIRE(gst_pad_is_linked(sinkPad.get()));

  element->unlink(*sink);
  // Check if elements are unlinked
  BOOST_REQUIRE(!gst_pad_is_linked(sourcePad.get()));
  BOOST_REQUIRE(!gst_pad_is_linked(sinkPad.get()));
}

BOOST_FIXTURE_TEST_CASE(LinkReturnsOtherForChaining, ElementTest)
{
  auto pipeline = Pipeline::create("test_pipeline");
  auto source = ElementFactory::makeElement("fakesrc", "test_source");
  auto identity = ElementFactory::makeElement("identity", "test_identity");
  auto sink = ElementFactory::makeElement("fakesink", "test_sink");
  pipeline->addElement(*source);
  pipeline->addElement(*identity);
  pipeline->addElement(*sink);

  Element& last = source->link(*identity).link(*sink);
  BOOST_CHECK_EQUAL(&last, sink.get());

  const auto identitySinkPad = identity->getStaticPad("sink");
  const auto sinkPad = sink->getStaticPad("sink");
  BOOST_CHECK(gst_pad_is_linked(identitySinkPad.get()));
  BOOST_CHECK(gst_pad_is_linked(sinkPad.get()));

  // linking twice fails and is reported
  BOOST_CHECK_THROW(source->link(*identity), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(SetName, ElementTest)
{
  auto element =
    Element::create(gst_element_factory_make("fakesrc", "firstName"), TransferType::floating);
  BOOST_REQUIRE_EQUAL(element->getName(), "firstName");

  BOOST_REQUIRE_NO_THROW(element->setName("secondName"));
  BOOST_CHECK_EQUAL(element->getName(), "secondName");
}
