/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */

// local includes
#include "elementfactory.hpp"

// std
#include <cassert>

namespace dh::gst
{

ElementFactory::ElementFactory(GstElementFactory* gstElementFactory, TransferType transferType)
: PluginFeature(GST_PLUGIN_FEATURE_CAST(gstElementFactory), transferType)
{
  assert(getGstElementFactory() != nullptr);
}

ElementFactory::ElementFactory(GstElementFactorySPtr gstElementFactory)
: PluginFeature(makeGstSharedPtr(GST_PLUGIN_FEATURE_CAST(gstElementFactory.get()), TransferType::None)) // No pointer_cast due to C inheritance
{
  assert(getRawGstElementFactory() != nullptr);
}

ElementFactory ElementFactory::fromFactoryName(const std::string& factoryName)
{
  GstElementFactory* factory = gst_element_factory_find(factoryName.c_str());
  if (!factory)
  {
    throw std::runtime_error("ElementFactory with name '" + factoryName + "' not found");
  }

  return ElementFactory(factory, TransferType::Full);
}

ElementFactory ElementFactory::ref()
{
  return ElementFactory(getGstElementFactory());
}

Element ElementFactory::createElement(const std::string& elementName) const
{
  auto* gstElementRaw = gst_element_factory_create(
    const_cast<GstElementFactory*>(getRawGstElementFactory()),
    elementName.empty() ? nullptr : elementName.c_str()
  );

  if(!gstElementRaw)
  {
    throw std::runtime_error("Failed to create element: " + elementName);
  }
  return Element(gstElementRaw, TransferType::Floating);
}

Element ElementFactory::makeElement(const std::string& factoryName, const std::string& elementName)
{
  // static version
  assert(! factoryName.empty());

  GstElement* gstElement = gst_element_factory_make(
    factoryName.c_str(),
    elementName.empty() ? nullptr : elementName.c_str()
  );

  if(! gstElement)
  {
    throw std::runtime_error("Failed to create element '" + elementName + "' from factory '" + factoryName + "'");
  }

  return Element(gstElement, TransferType::Floating);
}

std::string ElementFactory::getMetaData(const std::string& key) const
{
  const gchar* metadata = gst_element_factory_get_metadata(
    const_cast<GstElementFactory*>(getRawGstElementFactory()),
    key.c_str()
  );
  return metadata ? std::string(metadata) : "";
}

std::vector<std::string> ElementFactory::getMetadataKeys() const
{
  std::vector<std::string> keys;
  const gchar* const* rawKeys = gst_element_factory_get_metadata_keys(
    const_cast<GstElementFactory*>(getRawGstElementFactory())
  );

  if (rawKeys)
  {
    for (int i = 0; rawKeys[i] != nullptr; ++i)
    {
      keys.emplace_back(rawKeys[i]);
    }
  }

  return keys;
}

GType ElementFactory::getElementType() const
{
  return gst_element_factory_get_element_type(const_cast<GstElementFactory*>(getRawGstElementFactory()));
}

GstElementFactorySPtr ElementFactory::getGstElementFactory()
{
  return makeGstSharedPtr(
    GST_ELEMENT_FACTORY_CAST(getGstObject().get()),
    TransferType::None
  );
}

const GstElementFactorySPtr ElementFactory::getGstElementFactory() const
{
  return makeGstSharedPtr(
    GST_ELEMENT_FACTORY_CAST(getGstObject().get()),
    TransferType::None
  );
}

GstElementFactory* ElementFactory::getRawGstElementFactory()
{
  return (getGstElementFactory().get());
}

const GstElementFactory* ElementFactory::getRawGstElementFactory() const
{
  return (getGstElementFactory().get());
}

} // namespace dh::gst
