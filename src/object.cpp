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

Object::Object(GstObjectSPtr gstObject)
: Object{gstObject.get(), TransferType::None}
{
}

Object::Object(GstObject* gstObject, TransferType transferType)
: gstObject{gstObject}
{
  if(!gstObject)
  {
    throw std::runtime_error("No GstObject");
  }

  //FIXME: code duplication, see makeGstSharedPtr
  switch(transferType)
  {
    case TransferType::Full:
      // Full ownership is transferred; nothing extra to do
        break;
    case TransferType::None:
      // No ownership is transferred; we need to increase the ref count
        gst_object_ref(GST_OBJECT(gstObject));
    break;
    case TransferType::Floating:
      // Handle floating references properly
        if (g_object_is_floating(G_OBJECT(gstObject)))
        {
          g_object_ref_sink(G_OBJECT(gstObject)); // Sink the floating reference
        }
        else
        {
          gst_object_ref(GST_OBJECT(gstObject));  // Increase ref count if not floating
        }
    break;
  }}

Object::~Object()
{
  // Set GstElement state to NULL before unref if it's the last ref to the GstElement
  // This does not work always because setting a pipeline to "playing" increases ref counts.
  // But it is a layer of safety.
  // TODO: code duplication in @ref GstObjectDeleter
  if(GST_IS_ELEMENT(gstObject) && GST_OBJECT_REFCOUNT(gstObject) == 1)
  {
    gst_element_set_state(GST_ELEMENT(gstObject), GST_STATE_NULL);
  }
  gst_object_unref(gstObject);
}

GstObjectSPtr Object::getGstObject()
{
  return makeGstSharedPtr(gstObject, TransferType::None);
}

const GstObjectSPtr Object::getGstObject() const
{
  return makeGstSharedPtr(gstObject, TransferType::None);
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
  return gstObject;
}

GstObject* Object::getRawGstObject()
{
  return gstObject;
}

} // dh::gst
