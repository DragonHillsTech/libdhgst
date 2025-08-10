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

#include "object.hpp"

#include <stdexcept>

namespace dh::gst
{

Object::Object(GstObjectSPtr gstObject)
: gstObject{std::move(gstObject)}
{
  if(! this->gstObject)
  {
    throw std::runtime_error("No GstObject");
  }
}

Object::Object(GstObject* gstObject, TransferType transferType)
: Object{makeGstSharedPtr(gstObject, transferType)}
{
}

Object::~Object() = default;

GstObjectSPtr Object::getGstObject()
{
  return gstObject;
}

const GstObjectSPtr Object::getGstObject() const
{
  return gstObject;
}

std::string Object::getName() const
{
  // Get the name from the GstElement
  gchar* name = gst_object_get_name(const_cast<GstObject*>(getRawGstObject()));
  std::string ret = name ? name : "";
  g_free(name);
  return ret;
}

void Object::setName(const std::string& name)
{
  const bool success = gst_object_set_name(
    getRawGstObject(),
    name.empty() ? nullptr : name.c_str()
  );
  if(! success)
  {
    throw std::logic_error("Failed to set name (Object may have parent)");
  }
}

bool Object::signalExists(const std::string& signalName) const
{
  if (signalName.empty())
  {
    throw std::invalid_argument("empty signal name");
  }
  return g_signal_lookup(signalName.c_str(), G_OBJECT_TYPE(getRawGstObject())) != 0;
}

bool Object::propertyExists(const std::string& name) const
{
  if (name.empty())
  {
    throw std::invalid_argument("empty property name");
  }

  const auto* propertySpec = g_object_class_find_property(G_OBJECT_GET_CLASS(getRawGstObject()), name.c_str());

  return propertySpec != nullptr;
}

const GstObject* Object::getRawGstObject() const
{
  return gstObject.get();
}

GstObject* Object::getRawGstObject()
{
  return gstObject.get();
}

} // dh::gst
