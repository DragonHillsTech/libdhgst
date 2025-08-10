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

#ifndef DH_GST_PLUGIN_FEATURE_HPP
#define DH_GST_PLUGIN_FEATURE_HPP

// local includes
#include "object.hpp"
#include "sharedptrs.hpp"
#include "transfertype.hpp"

// GStreamer includes
#include <gst/gst.h>

// std
#include <memory>
#include <string>
#include <vector>

namespace dh::gst
{

/**
 * @todo add tests
 */
class PluginFeature : public Object
{
protected:
  /**
   * @brief Constructs a PluginFeature from a GstPluginFeature pointer
   * @param gstPluginFeature The GStreamer plugin feature pointer
   * @param transferType The ownership transfer type for the GStreamer object
   */
  PluginFeature(GstPluginFeature* gstPluginFeature, TransferType transferType);

  /**
   * @brief Constructs a PluginFeature from a shared pointer to GstPluginFeature
   * @param gstPluginFeature The GStreamer plugin feature shared pointer
   */
  explicit PluginFeature(GstPluginFeatureSPtr gstPluginFeature);

 public:
  [[nodiscard]] static std::shared_ptr<PluginFeature> create(GstPluginFeature* gstPluginFeature, TransferType transferType);
  [[nodiscard]] static std::shared_ptr<PluginFeature> create(GstPluginFeatureSPtr gstPluginFeature);

  /**
   * @brief Gets the underlying GstPluginFeature object
   * @return A shared pointer to the GstPluginFeature
   */
  [[nodiscard]] GstPluginFeatureSPtr getGstPluginFeature();

  /**
   * @brief Gets the underlying GstPluginFeature object (const)
   * @return A shared pointer to the GstPluginFeature (const)
   */
  [[nodiscard]] const GstPluginFeatureSPtr getGstPluginFeature() const;

  /**
   * @brief Gets the rank of this plugin feature
   * @return The rank of the plugin feature
   */
  [[nodiscard]] int getRank() const;

  /**
   * @brief Sets the rank of this plugin feature
   * @param rank The new rank to set
   */
  void setRank(int rank);

private:
  [[nodiscard]] const GstPluginFeature* getRawGstPluginFeature() const;
  [[nodiscard]] GstPluginFeature* getRawGstPluginFeature();
};

} // namespace dh::gst

#endif // DH_GST_PLUGIN_FEATURE_HPP
