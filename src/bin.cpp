/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
/**
 * @file bin.cpp
 * @author Sandro Stiller
 * @date 2024-09-11
 */

#include "bin.hpp"

#include <stdexcept>

namespace dh::gst
{

Bin::Bin(GstBinSPtr gstBin)
: Element(GST_ELEMENT_CAST(gstBin.get()), TransferType::None)
{
}

Bin::Bin(GstBin* gstBin, TransferType transferType)
: Element(GST_ELEMENT_CAST(gstBin), transferType)
{
}

Bin::Bin(const std::string& name)
: Element(
    GST_ELEMENT_CAST(gst_bin_new(name.c_str())),
    TransferType::Floating
  )
{
}

Bin Bin::fromDescription(const std::string& description, bool ghostUnlinkedPads)
{
  GError* error{nullptr};

  // Parse the pipeline description and store error if any occurs
  GstElement* gstElement = gst_parse_bin_from_description(
    description.c_str(),
    ghostUnlinkedPads,
    &error
  );

  // Create a smart pointer for the GError if it exists
  GErrorSPtr errorSPtr(error, GlibDeleter());

  // If parsing fails, throw an exception with the error message
  if(!gstElement)
  {
    std::string errMsg = "Failed to create Bin from description: ";
    errMsg += errorSPtr ? errorSPtr->message : "Unknown error.";
    throw std::runtime_error(errMsg);
  }

  // Return a Bin object created from the parsed pipeline
  return Bin(GST_BIN_CAST(gstElement), TransferType::Floating);
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

const GstBinSPtr Bin::getGstBin() const
{
  return makeGstSharedPtr(GST_BIN_CAST(getGstObject().get()), TransferType::None);
}

void Bin::addElement(GstElementSPtr element)
{
  // gst_bin_add: transfer: full
  auto* refIncreasedRawPtr = GST_ELEMENT(gst_object_ref(GST_OBJECT(element.get())));
  if(gst_bin_add(getRawGstBin(), refIncreasedRawPtr) == FALSE)
  {
    gst_object_unref(refIncreasedRawPtr);
    throw std::runtime_error("Failed to add element to GstBin.");
  }
}

void Bin::addElement(Element& element)
{
  addElement(element.getGstElement());
}

Element Bin::getElementByName(const std::string& name)
{
  // gst_bin_get_by_name: transfer:full, nullable
  GstElement* gstElement = gst_bin_get_by_name(GST_BIN(getRawGstBin()), name.c_str());
  if(!gstElement)
  {
    throw std::runtime_error("Element with name '" + name + "' not found.");
  }

  return Element(makeGstSharedPtr(gstElement, TransferType::Full));
}

Element Bin::getElementByNameRecurseUp(const std::string& name)
{
  // gst_bin_get_by_name_recurse_up: transfer:full, nullable

  GstElement* gstElement = gst_bin_get_by_name_recurse_up(GST_BIN(getRawGstBin()), name.c_str());
  if (!gstElement)
  {
    throw std::runtime_error("Element with name '" + name + "' not found.");
  }

  return Element(makeGstSharedPtr(gstElement, TransferType::Full));
}

void Bin::removeElement(GstElementSPtr element)
{
  if (gst_bin_remove(getRawGstBin(), element.get()) == FALSE)
  {
    throw std::runtime_error("Failed to remove element from GstBin.");
  }
}

void Bin::removeElement(Element& element)
{
  if (gst_bin_remove(getRawGstBin(), element.getGstElement().get()) == FALSE)
  {
    throw std::runtime_error("Failed to remove element from GstBin.");
  }
}

bs2::signal<void(GstElementSPtr)>& Bin::elementAddedSignal() const
{
  return connectGobjectSignal<GstElementSPtr>("element-added");
}

const GstBin* Bin::getRawGstBin() const
{
  return GST_BIN_CAST(getRawGstObject());
}

GstBin* Bin::getRawGstBin()
{
  return GST_BIN_CAST(getRawGstObject());
}
} // dh::gst
