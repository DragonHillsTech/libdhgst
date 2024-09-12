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

Element::Element(GstElement* gstElement, TransferType transferType)
: Object(GST_OBJECT_CAST(gstElement), transferType)
{
  assert(getGstElement() != nullptr);
}

Element::Element(GstElementSPtr gstElement)
: Object(makeGstSharedPtr(GST_OBJECT_CAST(gstElement.get()), TransferType::None)) // no pointer_cast because C inheritance
{
  assert(getGstElement() != nullptr);
}

Element Element::ref()
{
  return Element(getGstElement());
}

GstElementSPtr Element::getGstElement()
{
  // we can not use *pointer_cast because these are C types
  return makeGstSharedPtr(GST_ELEMENT_CAST(getGstObject().get()), TransferType::None);
}

const GstElementSPtr Element::getGstElement() const
{
  // we can not use *pointer_cast because these are C types
  return makeGstSharedPtr(GST_ELEMENT_CAST(getGstObject().get()), TransferType::None);
}

GstStateChangeReturn Element::setState(GstState newState)
{
  return gst_element_set_state(getGstElement().get(), newState);
}

std::vector<GstPad*> Element::getPads()
{
  std::vector<GstPad*> pads;
  gst_element_foreach_pad(
    getGstElement().get(),
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
    getGstElement().get(), [](GstElement* /*element*/,GstPad* pad, gpointer user_data) ->gboolean
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
    getGstElement().get(),
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
  return gst_element_get_compatible_pad(getGstElement().get(), pad, caps);
}

GstPad* Element::getStaticPad(const std::string& name)
{
  return gst_element_get_static_pad(getGstElement().get(), name.c_str());
}

Element& Element::link(Element& other)
{
  if (!gst_element_link(getGstElement().get(), other.getGstElement().get()))
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
  return gst_element_get_start_time(getGstElement().get());
}

GstState Element::getState() const
{
  GstState state;
  gst_element_get_state(getGstElement().get(), &state, nullptr, GST_CLOCK_TIME_NONE);
  return state;
}

void Element::unlink(Element& other)
{
  gst_element_unlink(getGstElement().get(), other.getGstElement().get());
}

bool Element::syncStateWithParent()
{
  return gst_element_sync_state_with_parent(getGstElement().get()) ? true : false;
}

bool Element::signalExists(const std::string& signalName) const
{
  return g_signal_lookup(signalName.c_str(), G_OBJECT_TYPE(getGstElement().get())) != 0;
}


} // dh::gst
