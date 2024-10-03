/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file pluginfeature.hpp
 * @author Sandro Stiller
 * @date 2024-09-14
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
