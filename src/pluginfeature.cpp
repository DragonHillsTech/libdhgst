/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */

// local includes
#include "pluginfeature.hpp"

// C
#include <cassert>

namespace dh::gst
{

PluginFeature::PluginFeature(GstPluginFeature* gstPluginFeature, TransferType transferType)
: Object(GST_OBJECT_CAST(gstPluginFeature), transferType)
{
  assert(getRawGstPluginFeature() != nullptr);
}

PluginFeature::PluginFeature(GstPluginFeatureSPtr gstPluginFeature)
: Object(GST_OBJECT_CAST(gstPluginFeature.get()), TransferType::None) // No pointer_cast due to C inheritance
{
  assert(getGstPluginFeature() != nullptr);
}

PluginFeature PluginFeature::ref()
{
  return PluginFeature(getGstPluginFeature());
}

GstPluginFeatureSPtr PluginFeature::getGstPluginFeature()
{
  return makeGstSharedPtr(GST_PLUGIN_FEATURE_CAST(getGstObject().get()), TransferType::None);
}

const std::shared_ptr<GstPluginFeature> PluginFeature::getGstPluginFeature() const
{
  return makeGstSharedPtr(GST_PLUGIN_FEATURE_CAST(getGstObject().get()), TransferType::None);
}

int PluginFeature::getRank() const
{
  return gst_plugin_feature_get_rank(const_cast<GstPluginFeature*>(getRawGstPluginFeature()));
}

void PluginFeature::setRank(int rank)
{
  gst_plugin_feature_set_rank(getRawGstPluginFeature(), rank);
}

const GstPluginFeature* PluginFeature::getRawGstPluginFeature() const
{
  return GST_PLUGIN_FEATURE_CAST(getRawGstObject());
}

GstPluginFeature* PluginFeature::getRawGstPluginFeature()
{
  return GST_PLUGIN_FEATURE_CAST(getRawGstObject());
}


} // namespace dh::gst
