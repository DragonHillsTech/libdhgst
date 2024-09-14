/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
/**
 * @file bin.hpp
 * @author Sandro Stiller
 */

#include "bin.hpp"

#include <stdexcept>

namespace dh::gst
{

Bin::Bin(GstBinSPtr gstBin)
: Element(makeGstSharedPtr(GST_ELEMENT_CAST(gstBin.get()), TransferType::None)) // no pointer_cast because C inheritance
{
}

Bin::Bin(GstBin* gstBin, TransferType transferType)
: Element(GST_ELEMENT_CAST(gstBin), transferType)
{
}

Bin Bin::ref()
{
  return Bin(getGstBin());
}

GstBinSPtr Bin::getGstBin()
{
  // we can not use *pointer_cast because these are C types
  return makeGstSharedPtr(GST_BIN_CAST(getGstObject().get()), TransferType::None);
}

void Bin::addElement(GstElementSPtr element)
{
  if (gst_bin_add(getGstBin().get(), element.get()) == FALSE)
  {
    throw std::runtime_error("Failed to add element to GstBin.");
  }
}

void Bin::addElement(Element& element)
{
  if (gst_bin_add(getGstBin().get(), element.getGstElement().get()) == FALSE)
  {
    throw std::runtime_error("Failed to add element to GstBin.");
  }
}

Element Bin::getElementByName(const std::string& name)
{
  GstElement* gstElement = gst_bin_get_by_name(GST_BIN(getGstBin().get()), name.c_str());
  if (!gstElement) {
    throw std::runtime_error("Element with name '" + name + "' not found.");
  }

  return Element(makeGstSharedPtr(gstElement, TransferType::Full));
}

Element Bin::getElementByNameRecurseUp(const std::string& name)
{
  GstElement* gstElement = gst_bin_get_by_name_recurse_up(GST_BIN(getGstBin().get()), name.c_str());
  if (!gstElement) {
    throw std::runtime_error("Element with name '" + name + "' not found.");
  }

  return Element(makeGstSharedPtr(gstElement, TransferType::Full));
}

void Bin::removeElement(GstElementSPtr element)
{
  if (gst_bin_remove(getGstBin().get(), element.get()) == FALSE)
  {
    throw std::runtime_error("Failed to remove element from GstBin.");
  }
}

void Bin::removeElement(Element& element)
{
  if (gst_bin_remove(getGstBin().get(), element.getGstElement().get()) == FALSE)
  {
    throw std::runtime_error("Failed to remove element from GstBin.");
  }
}

} // dh::gst
