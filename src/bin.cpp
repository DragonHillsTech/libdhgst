/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
/* Copyright (C) 2024 Sandro Stiller <sandro.stiller@dragonhills.de>
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of Libdhgst <https://dragonhills.de/>.
 *
 * Libdhgst is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Libdhgst is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Libdhgst.  If not, see <http://www.gnu.org/licenses/>.
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

std::shared_ptr<Bin> Bin::create(GstBinSPtr gstBin)
{
  return std::shared_ptr<Bin>(new Bin(gstBin));
}

std::shared_ptr<Bin> Bin::create(GstBin* gstBin, TransferType transferType)
{
  return std::shared_ptr<Bin>(new Bin(gstBin, transferType));
}

std::shared_ptr<Bin> Bin::create(const std::string& name)
{
  return std::shared_ptr<Bin>(new Bin(name));
}

std::shared_ptr<Bin> Bin::fromDescription(const std::string& description, bool ghostUnlinkedPads)
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
  return create(GST_BIN_CAST(gstElement), TransferType::Floating);
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

void Bin::addElement(std::shared_ptr<Element> element)
{
  addElement(element->getGstElement());
}

std::shared_ptr<Element> Bin::getElementByName(const std::string& name)
{
  // gst_bin_get_by_name: transfer:full, nullable
  GstElement* gstElement = gst_bin_get_by_name(GST_BIN(getRawGstBin()), name.c_str());
  if(!gstElement)
  {
    throw std::runtime_error("Element with name '" + name + "' not found.");
  }

  return Element::create(gstElement, TransferType::Full);
}

std::shared_ptr<Element> Bin::getElementByNameRecurseUp(const std::string& name)
{
  // gst_bin_get_by_name_recurse_up: transfer:full, nullable

  GstElement* gstElement = gst_bin_get_by_name_recurse_up(GST_BIN(getRawGstBin()), name.c_str());
  if (!gstElement)
  {
    throw std::runtime_error("Element with name '" + name + "' not found.");
  }

  return Element::create(gstElement, TransferType::Full);
}

void Bin::removeElement(GstElementSPtr element)
{
  if (gst_bin_remove(getRawGstBin(), element.get()) == FALSE)
  {
    throw std::runtime_error("Failed to remove element from GstBin.");
  }
}

void Bin::removeElement(const std::shared_ptr<Element>& element)
{
  if(gst_bin_remove(getRawGstBin(), element->getGstElement().get()) == FALSE)
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
