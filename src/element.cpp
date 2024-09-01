
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
  Private(GstElement* gstElement)
  : gstElement{gstElement}
  {
  }

  GstElement* gstElement{nullptr};
};

Element::Element(GstElement* gstElement, TransferType transferType)
: prv{std::make_unique<Private>(gstElement)}
{
  assert(gstElement != nullptr);
  spdlog::info("Create element from {}, refCnt = {}", static_cast<void*>(gstElement), GST_OBJECT_REFCOUNT(prv->gstElement));
  if(transferType == TransferType::None)
  {
    gst_object_ref(GST_OBJECT(gstElement));
  }
}

Element::~Element()
{
  // if last reference, stop it
  const auto refCnt = GST_OBJECT_REFCOUNT(prv->gstElement);
  spdlog::info("refCount={}", refCnt);
  if(refCnt == 1)
  {
    setState(GST_STATE_NULL);
  }

  // Disconnect all GStreamer signals with 'this' as user data
  //g_signal_handlers_disconnect_by_data(prv->gstElement, this);

  gst_object_unref(prv->gstElement);
}

Element Element::ref()
{
  return Element(prv->gstElement, TransferType::None);
}

GstElement* Element::get()
{
  return prv->gstElement;
}

std::string Element::getName() const
{
  // Get the name from the GstElement
  const gchar* name = gst_element_get_name(prv->gstElement);

  // Return as std::string; handle null case gracefully
  return name ? std::string(name) : std::string("unknown");
}

GstStateChangeReturn Element::setState(GstState newState)
{
  return gst_element_set_state(prv->gstElement, newState);
}

std::vector<GstPad*> Element::getPads()
{
  std::vector<GstPad*> pads;
  gst_element_foreach_pad(
    prv->gstElement,
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
    prv->gstElement, [](GstElement* /*element*/,GstPad* pad, gpointer user_data) ->gboolean
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
    prv->gstElement,
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
  return gst_element_get_compatible_pad(prv->gstElement, pad, caps);
}

GstPad* Element::getStaticPad(const std::string& name)
{
  return gst_element_get_static_pad(prv->gstElement, name.c_str());
}

Element& Element::link(Element& other)
{
  if (!gst_element_link(prv->gstElement, other.prv->gstElement))
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
  return gst_element_get_start_time(prv->gstElement);
}

GstState Element::getState() const
{
  GstState state;
  gst_element_get_state(prv->gstElement, &state, nullptr, GST_CLOCK_TIME_NONE);
  return state;
}

void Element::unlink(Element& other)
{
  gst_element_unlink(prv->gstElement, other.prv->gstElement);
}

bool Element::syncStateWithParent()
{
  return gst_element_sync_state_with_parent(prv->gstElement) ? true : false;
}

bool Element::signalExists(const std::string& signalName) const
{
  return g_signal_lookup(signalName.c_str(), G_OBJECT_TYPE(prv->gstElement)) != 0;
}


} // dh::gst
