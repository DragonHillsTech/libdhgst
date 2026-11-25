/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
/* Copyright (C) 2024 Sandro Stiller <sandro.stiller@dragonhills.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This file is part of Libdhgst <https://dragonhills.de/>.
 *
 * Libdhgst is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Libdhgst is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Libdhgst. If not, see <http://www.gnu.org/licenses/>.
 */

#include "bin.hpp"
#include "element.hpp"
#include "sharedptrs.hpp"
#include "signalconnection.hpp"
#include "transfertype.hpp"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace dh::gst
{

Bin::Bin(const GstBinSPtr& gstBin)
  : Element(GST_ELEMENT_CAST(gstBin.get()), TransferType::none)
{
}

Bin::Bin(GstBin* gstBin, TransferType transferType)
  : Element(GST_ELEMENT_CAST(gstBin), transferType)
{
}

Bin::Bin(const std::string& name)
  : Element(GST_ELEMENT_CAST(gst_bin_new(name.c_str())), TransferType::floating)
{
}

std::shared_ptr<Bin> Bin::create(const GstBinSPtr& gstBin)
{
  std::shared_ptr<Bin> bin(new Bin(gstBin));
  bin->requireType<GstBin>();
  return bin;
}

std::shared_ptr<Bin> Bin::create(GstBin* gstBin, TransferType transferType)
{
  std::shared_ptr<Bin> bin(new Bin(gstBin, transferType));
  bin->requireType<GstBin>();
  return bin;
}

std::shared_ptr<Bin> Bin::create(const std::string& name)
{
  return std::shared_ptr<Bin>(new Bin(name));
}

std::shared_ptr<Bin> Bin::fromDescription(const std::string& description, bool ghostUnlinkedPads)
{
  GError* error{nullptr};

  // Parse the pipeline description and store error if any occurs
  GstElement* gstElement =
    gst_parse_bin_from_description(description.c_str(), ghostUnlinkedPads, &error);

  // Create a smart pointer for the GError if it exists
  const GErrorSPtr errorSPtr(error, GlibDeleter());

  // If parsing fails, throw an exception with the error message
  if (!gstElement)
  {
    std::string errMsg = "Failed to create Bin from description: ";
    errMsg += errorSPtr ? errorSPtr->message : "Unknown error.";
    throw std::runtime_error(errMsg);
  }

  // Return a Bin object created from the parsed pipeline
  return create(GST_BIN_CAST(gstElement), TransferType::floating);
}

GstBinSPtr Bin::getGstBin() const
{
  return makeGstSharedPtr(GST_BIN_CAST(getGstObject().get()), TransferType::none);
}

void Bin::addElement(GstElement& element)
{
  // gst_bin_add: transfer: floating
  // The bin sinks a floating reference, otherwise it takes its own reference.
  if (gst_bin_add(getRawGstBin(), &element) == FALSE)
  {
    throw std::runtime_error("Failed to add element to GstBin.");
  }
}

void Bin::addElement(Element& element)
{
  addElement(*element.getGstElement());
}

std::shared_ptr<Element> Bin::getElementByName(const std::string& name)
{
  // gst_bin_get_by_name: transfer:full, nullable
  GstElement* gstElement = gst_bin_get_by_name(GST_BIN(getRawGstBin()), name.c_str());
  if (!gstElement)
  {
    throw std::runtime_error("Element with name '" + name + "' not found.");
  }

  return Element::create(gstElement, TransferType::full);
}

std::shared_ptr<Element> Bin::getElementByNameRecurseUp(const std::string& name)
{
  // gst_bin_get_by_name_recurse_up: transfer:full, nullable

  GstElement* gstElement = gst_bin_get_by_name_recurse_up(GST_BIN(getRawGstBin()), name.c_str());
  if (!gstElement)
  {
    throw std::runtime_error("Element with name '" + name + "' not found.");
  }

  return Element::create(gstElement, TransferType::full);
}

void Bin::removeElement(GstElement& element)
{
  if (gst_bin_remove(getRawGstBin(), &element) == FALSE)
  {
    throw std::runtime_error("Failed to remove element from GstBin.");
  }
}

void Bin::removeElement(Element& element)
{
  removeElement(*element.getGstElement());
}

SignalConnection Bin::connectSignalElementAdded(std::function<void(GstElementSPtr)> callback) const
{
  return connectGobjectSignal<GstElementSPtr>("element-added", std::move(callback));
}

const GstBin* Bin::getRawGstBin() const
{
  return GST_BIN_CAST(getRawGstObject());
}

GstBin* Bin::getRawGstBin()
{
  return GST_BIN_CAST(getRawGstObject());
}

} // namespace dh::gst
