/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */

#include "elementfactory.hpp"
#include "pipeline.hpp"
#include "element.hpp"

#define BOOST_TEST_MODULE libdhgst_tests
#include <boost/test/included/unit_test.hpp>
#include "fatalgliblog.hpp"
BOOST_TEST_GLOBAL_FIXTURE(FatalGlibCriticals);

#include <gst/gst.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace dh::gst;

class ObjectTest
{
public:
  // Setup before first test case
  ObjectTest()
  {
    gst_init(nullptr, nullptr);  // Initialize GStreamer
  }
};

/**
 * @brief Element that exposes the protected connectGobjectSignal for testing.
 */
class SignalTestElement : public Element
{
public:
  explicit SignalTestElement(const char* factoryName)
  : Element(gst_element_factory_make(factoryName, nullptr), TransferType::Floating)
  {
  }

  using Object::connectGobjectSignal;
};

BOOST_FIXTURE_TEST_CASE(TestConnectGobjectSignal_PadAdded, ObjectTest)
{
  auto pipeline = Pipeline::fromDescription("videotestsrc ! decodebin name=test-decodebin");

  auto decodeBinElement = pipeline.getElementByName("test-decodebin");

  // Connect to the "pad-added" signal
  bool signalEmitted = false;
  std::shared_ptr<GstPad> capturedPad;
  auto padAddedConnection = decodeBinElement->connectSignalPadAdded(
    [&](std::shared_ptr<GstPad> pad)
    {
      signalEmitted = true;
      capturedPad = pad;
    }
  );
  BOOST_REQUIRE(padAddedConnection.connected());

  // Set pipeline to PLAYING to trigger the "pad-added" signal
  const auto setStateResult = pipeline.setState(GST_STATE_PLAYING);
  BOOST_REQUIRE_NE(setStateResult, GST_STATE_CHANGE_FAILURE);

  // Wait for the main loop to process events
  GMainContext* context = g_main_context_default();
  for(int i = 0; i < 10 && !signalEmitted; ++i)
  {
    g_main_context_iteration(context, FALSE);
    g_usleep(100000); // Sleep for 100ms
  }

  // Check that the signal was emitted and the pad was captured
  BOOST_CHECK(signalEmitted);
  BOOST_REQUIRE_NE(capturedPad, nullptr);
  pipeline.setState(GST_STATE_NULL);
}

BOOST_FIXTURE_TEST_CASE(TestConnectGobjectSignal_ElementAdded, ObjectTest)
{
  // Create a GstBin
  auto bin = Bin::create("test-bin");

  bool signalEmitted = false;
  GstElementSPtr capturedElement;
  auto elementAddedConnection = bin->connectSignalElementAdded(
    [&](std::shared_ptr<GstElement> element)
    {
      signalEmitted = true;
      capturedElement = element;
    }
  );
  BOOST_REQUIRE(elementAddedConnection.connected());

  auto element = ElementFactory::makeElement("fakesrc", "test-source");

  bin->addElement(*element);

  // Wait for the main loop to process events
  GMainContext* context = g_main_context_default();
  g_main_context_iteration(context, FALSE);

  // Check that the signal was emitted and the element was captured
  BOOST_CHECK(signalEmitted);
  BOOST_REQUIRE_NE(capturedElement, nullptr);
  BOOST_CHECK_EQUAL(capturedElement, element->getGstElement());
}

//TODO:

BOOST_FIXTURE_TEST_CASE(TestConnectGobjectSignal_deleteWhileCallbackActive, ObjectTest)
{
  /* TODO:
   * This works find, BUT
   * *
   */

  // Create a GstBin
  auto bin = Bin::create("test-bin");

  bool signalEmitted = false;
  GstElementSPtr capturedElement;
  auto elementAddedConnection = bin->connectSignalElementAdded(
    [&](std::shared_ptr<GstElement> element)
    {
      signalEmitted = true;
      capturedElement = element;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  );
  BOOST_REQUIRE(elementAddedConnection.connected());

  auto element = ElementFactory::makeElement("fakesrc", "test-source");

  BOOST_REQUIRE_EQUAL(bin.use_count(), 1);
  // does not increase gstBinSPtr.use_count because a new one is created from the raw ptr
  GstBin* gstBinPtr = bin->getGstBin().get();

  BOOST_REQUIRE_EQUAL(GST_OBJECT_REFCOUNT(gstBinPtr), 1);

  std::thread thread(
    // Keep a copy to the objects when using threads. "&" is dangerous!.
    [bin, element]()
    {
      bin->addElement(*element);
    }
  );

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // the callback should have increased in the thread capture
  BOOST_REQUIRE_EQUAL(bin.use_count(), 2);

  // one in Bin, one in  callback
  const auto currentGObjectRefCnt = GST_OBJECT_REFCOUNT(gstBinPtr);
  BOOST_REQUIRE_GE(currentGObjectRefCnt, 2);

  std::weak_ptr<Bin> weakBin = bin;
  bin.reset();

  // the Bin element must still exist. after deleting the local pointer. Copy in thread capture
  BOOST_REQUIRE_NE(weakBin.lock(), nullptr);
  // and of course the internal object
  BOOST_REQUIRE_EQUAL(GST_OBJECT_REFCOUNT(gstBinPtr), currentGObjectRefCnt);

  // Wait for the main loop to process events
  GMainContext* context = g_main_context_default();
  g_main_context_iteration(context, FALSE);

  // Check that the signal was emitted and the element was captured
  BOOST_CHECK(signalEmitted);
  BOOST_REQUIRE_NE(capturedElement, nullptr);
  BOOST_CHECK_EQUAL(capturedElement, element->getGstElement());

  // the Bin element must still exist.
  BOOST_REQUIRE_NE(weakBin.lock(), nullptr);
  BOOST_REQUIRE_GE(GST_OBJECT_REFCOUNT(gstBinPtr), 1);

  thread.join();

  // Now everything should be deleted
  BOOST_REQUIRE_EQUAL(weakBin.lock(), nullptr);
}


BOOST_FIXTURE_TEST_CASE(TestConnectGobjectSignal_ThrowingCallbackDoesNotPropagate, ObjectTest)
{
  auto bin = Bin::create("test-bin");

  bool signalEmitted = false;
  auto elementAddedConnection = bin->connectSignalElementAdded(
    [&](GstElementSPtr /*element*/)
    {
      signalEmitted = true;
      throw std::runtime_error("exception from signal callback");
    }
  );
  BOOST_REQUIRE(elementAddedConnection.connected());

  // the exception must not unwind through the GLib signal emission into the caller
  auto element = ElementFactory::makeElement("fakesrc", "test-source");
  BOOST_CHECK_NO_THROW(bin->addElement(*element));
  BOOST_CHECK(signalEmitted);
}

// --- connectGobjectSignal checks the signal signature (see todo.md) ---

BOOST_FIXTURE_TEST_CASE(TestConnectGobjectSignal_MatchingSignature, ObjectTest)
{
  auto element = std::make_shared<SignalTestElement>("identity");
  BOOST_CHECK_NO_THROW((void)element->connectGobjectSignal<>("no-more-pads", std::function<void()>([]() {})));
  BOOST_CHECK_NO_THROW(
    (void)element->connectGobjectSignal<GstPadSPtr>("pad-added", std::function<void(GstPadSPtr)>([](GstPadSPtr) {}))
  );
}

BOOST_FIXTURE_TEST_CASE(TestConnectGobjectSignal_WrongNumberOfParametersThrows, ObjectTest)
{
  auto element = std::make_shared<SignalTestElement>("identity");
  // pad-added has one parameter
  BOOST_CHECK_THROW(
    (void)element->connectGobjectSignal<>("pad-added", std::function<void()>([]() {})),
    std::invalid_argument
  );
  // no-more-pads has none
  BOOST_CHECK_THROW(
    (void)element->connectGobjectSignal<GstPadSPtr>("no-more-pads", std::function<void(GstPadSPtr)>([](GstPadSPtr) {})),
    std::invalid_argument
  );
}

BOOST_FIXTURE_TEST_CASE(TestConnectGobjectSignal_WrongParameterTypeThrows, ObjectTest)
{
  auto element = std::make_shared<SignalTestElement>("identity");
  // pad-added passes a GstPad, not a GstElement: the callback would reinterpret the pad
  BOOST_CHECK_THROW(
    (void)element->connectGobjectSignal<GstElementSPtr>(
      "pad-added",
      std::function<void(GstElementSPtr)>([](GstElementSPtr) {})
    ),
    std::invalid_argument
  );
  BOOST_CHECK_THROW(
    (void)element->connectGobjectSignal<int>("pad-added", std::function<void(int)>([](int) {})),
    std::invalid_argument
  );
}

BOOST_FIXTURE_TEST_CASE(TestConnectGobjectSignal_SignalWithReturnValueThrows, ObjectTest)
{
  // appsink "new-sample" returns a GstFlowReturn, but the callbacks return void
  auto appsink = std::make_shared<SignalTestElement>("appsink");
  BOOST_CHECK_THROW(
    (void)appsink->connectGobjectSignal<>("new-sample", std::function<void()>([]() {})),
    std::invalid_argument
  );
}
