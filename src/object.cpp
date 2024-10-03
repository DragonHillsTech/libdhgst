/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */

/**
 * @file object.hpp
 * @author Sandro Stiller
 * @date 2024-09-12
 */

#include "object.hpp"

#include <stdexcept>

namespace dh::gst
{

class Object::Private
{
public:
  Private(GstObjectSPtr gstObject)
  : gstObject{std::move(gstObject)}
  {
  }
  GstObjectSPtr gstObject;
};

Object::Object(Object&& other) noexcept = default;

Object& Object::operator=(Object&& other) noexcept = default;

Object::Object(GstObjectSPtr gstObject)
: prv{std::make_unique<Private>(std::move(gstObject))}
{
  if (! prv->gstObject)
  {
    throw std::runtime_error("No GstObject");
  }
}

Object::Object(GstObject* gstObject, TransferType transferType)
: prv{std::make_unique<Private>(makeGstSharedPtr(gstObject, transferType))}
{
  if (! prv->gstObject)
  {
    throw std::runtime_error("No GstObject");
  }
}

Object::~Object() = default;

Object Object::ref()
{
  return Object(getGstObject());
}

GstObjectSPtr Object::getGstObject()
{
  if(! prv)
  {
    throw std::logic_error("No valid Object.prv (moved?)");
  }
  return prv->gstObject;
}

const GstObjectSPtr Object::getGstObject() const
{
  if(! prv)
  {
    throw std::logic_error("No valid GstObject (moved?)");
  }
  return prv->gstObject;
}

std::string Object::getName() const
{
  // Get the name from the GstElement
  const gchar* name = gst_object_get_name(const_cast<GstObject*>(getRawGstObject()));

  // Return as std::string; handle null case gracefully
  return name ? std::string(name) : std::string("unknown");
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
  return getGstObject().get();
}

GstObject* Object::getRawGstObject()
{
  return getGstObject().get();
}



} // dh::gst
