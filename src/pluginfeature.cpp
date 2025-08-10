/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
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

std::shared_ptr<PluginFeature> PluginFeature::create(GstPluginFeature* gstPluginFeature, TransferType transferType)
{
  return std::shared_ptr<PluginFeature>(new PluginFeature(gstPluginFeature, transferType));
}

std::shared_ptr<PluginFeature> PluginFeature::create(GstPluginFeatureSPtr gstPluginFeature)
{
  return std::shared_ptr<PluginFeature>(new PluginFeature(gstPluginFeature));
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
