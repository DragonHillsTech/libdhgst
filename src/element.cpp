/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */

// local includes
#include "element.hpp"

// opther libs
#include "spdlog/spdlog.h"

// std
#include <string>

// C
#include <cassert>

namespace dh::gst
{
class Element::Private
{
public:
  Private(GstElementSPtr gstElement)
  : gstElement{gstElement}
  {
  }

  GstElementSPtr gstElement{nullptr};
};

Element::Element(GstElement* gstElement, TransferType transferType)
: prv{
    std::make_unique<Private>(
      makeGstSharedPtr(gstElement, transferType)
    )
  }
{
  assert(prv->gstElement != nullptr);
}

Element::Element(GstElementSPtr gstElement)
: prv{std::make_unique<Private>(std::move(gstElement))}
{
  assert(prv->gstElement != nullptr);
}

Element::~Element()
{
  // if last reference, stop it

  // first increase ref cnt, so we can stop in case our shared_ptr was the last one
  // the delete shared_ptr. If ref cnt is now 1, we know that nobody else has a reference
  // and we stop the element before really finalising it.


  GstElement* rawGstElement = prv->gstElement.get();
  gst_object_ref(GST_OBJECT(rawGstElement));
  prv.reset();

  // There may be a multithreading problem:
  // if ref cnt == 2, another piece of software could see the same ref cnt, both decrease and nobody stops the element
  // leading to a gstreamer error.
  // We currently solve it with the ostrich algorithm
  const auto refCnt = GST_OBJECT_REFCOUNT(rawGstElement);
  //spdlog::info("refCount={}", refCnt);
  if(refCnt == 1)
  {
    gst_element_set_state(rawGstElement, GST_STATE_NULL);
  }

  // Disconnect all GStreamer signals with 'this' as user data
  //g_signal_handlers_disconnect_by_data(prv->gstElement, this);

  gst_object_unref(rawGstElement);
}

Element Element::ref()
{
  return Element(prv->gstElement);
}

GstElementSPtr Element::getGstElement()
{
  return prv->gstElement;
}

std::string Element::getName() const
{
  // Get the name from the GstElement
  const gchar* name = gst_element_get_name(prv->gstElement.get());

  // Return as std::string; handle null case gracefully
  return name ? std::string(name) : std::string("unknown");
}

GstStateChangeReturn Element::setState(GstState newState)
{
  return gst_element_set_state(prv->gstElement.get(), newState);
}

std::vector<GstPad*> Element::getPads()
{
  std::vector<GstPad*> pads;
  gst_element_foreach_pad(
    prv->gstElement.get(),
    [](GstElement* /*element*/, GstPad* pad, gpointer user_data) ->gboolean
    {
      auto& pads = *reinterpret_cast<std::vector<GstPad*>*>(user_data);
      pads.push_back(pad);
      return true; // Continue iteration
    },
    &pads
  );
  return pads;
}

std::vector<GstPad*> Element::getSinkPads()
{
  std::vector<GstPad*> sinkPads;
  gst_element_foreach_sink_pad(
    prv->gstElement.get(), [](GstElement* /*element*/,GstPad* pad, gpointer user_data) ->gboolean
    {
      auto& sinkPads = *reinterpret_cast<std::vector<GstPad*>*>(user_data);
      sinkPads.push_back(pad);
      return true; // Continue iteration
    },
    &sinkPads
  );
  return sinkPads;
}

std::vector<GstPad*> Element::getSrcPads()
{
  std::vector<GstPad*> srcPads;
  gst_element_foreach_src_pad(
    prv->gstElement.get(),
    [](GstElement* /*element*/, GstPad* pad, gpointer user_data) ->gboolean
    {
      auto& srcPads = *reinterpret_cast<std::vector<GstPad*>*>(user_data);
      srcPads.push_back(pad);
      return true; // Continue iteration
    },
    &srcPads
  );
  return srcPads;
}

GstPad* Element::getCompatiblePad(GstPad* pad, GstCaps* caps)
{
  return gst_element_get_compatible_pad(prv->gstElement.get(), pad, caps);
}

GstPad* Element::getStaticPad(const std::string& name)
{
  return gst_element_get_static_pad(prv->gstElement.get(), name.c_str());
}

Element& Element::link(Element& other)
{
  if (!gst_element_link(prv->gstElement.get(), other.prv->gstElement.get()))
  {
    std::string errorMessage = "Failed to link GstElements: ";
    errorMessage += getName() + " -> " + other.getName();

    // Throw an exception with the detailed error message
    throw std::runtime_error(errorMessage);
  }

  return *this;
}

GstClockTime Element::getStartTime() const
{
  return gst_element_get_start_time(prv->gstElement.get());
}

GstState Element::getState() const
{
  GstState state;
  gst_element_get_state(prv->gstElement.get(), &state, nullptr, GST_CLOCK_TIME_NONE);
  return state;
}

void Element::unlink(Element& other)
{
  gst_element_unlink(prv->gstElement.get(), other.prv->gstElement.get());
}

bool Element::syncStateWithParent()
{
  return gst_element_sync_state_with_parent(prv->gstElement.get()) ? true : false;
}

bool Element::signalExists(const std::string& signalName) const
{
  return g_signal_lookup(signalName.c_str(), G_OBJECT_TYPE(prv->gstElement.get())) != 0;
}


} // dh::gst
