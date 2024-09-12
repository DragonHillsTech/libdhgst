/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */

/**
 * @file object.hpp
 * @author Sandro Stiller
 * @date 2024-09-12
 */

#include "object.hpp"

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

Object::Object(GstObjectSPtr gstObject)
: prv{std::make_unique<Private>(std::move(gstObject))}
{
}

Object::Object(GstObject* gstObject, TransferType transferType)
: prv{std::make_unique<Private>(makeGstSharedPtr(gstObject, transferType))}
{
}

Object::~Object() = default;

Object Object::ref()
{
  return Object(getGstObject());
}

GstObjectSPtr Object::getGstObject()
{
  return prv->gstObject;
}

const GstObjectSPtr Object::getGstObject() const
{
  return prv->gstObject;
}

std::string Object::getName() const
{
  // Get the name from the GstElement
  const gchar* name = gst_object_get_name(prv->gstObject.get());

  // Return as std::string; handle null case gracefully
  return name ? std::string(name) : std::string("unknown");
}


} // dh::gst