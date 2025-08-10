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

#ifndef DH_GST_ELEMENTFACTORY_HPP
#define DH_GST_ELEMENTFACTORY_HPP

// local includes
#include "element.hpp"
#include "pluginfeature.hpp"
#include "sharedptrs.hpp"

// GStreamer includes
#include <gst/gst.h>
#include <gst/gstelementfactory.h>

// std
#include <stdexcept>
#include <string>
#include <vector>

namespace dh::gst
{

class ElementFactory : public PluginFeature
{
protected:
  /**
   * @brief Constructs an ElementFactory from a GstElementFactory pointer
   * @param gstElementFactory The GStreamer element factory pointer
   * @param transferType The ownership transfer type for the GStreamer object
   */
  ElementFactory(GstElementFactory* gstElementFactory, TransferType transferType);

  /**
   * @brief Constructs an ElementFactory from a shared pointer to GstElementFactory
   * @param gstElementFactory The GStreamer element factory shared pointer
   */
  ElementFactory(GstElementFactorySPtr gstElementFactory);

public:
  [[nodiscard]] static std::shared_ptr<ElementFactory> create(GstElementFactory* gstElementFactory, TransferType transferType);
  [[nodiscard]] static std::shared_ptr<ElementFactory> create(GstElementFactorySPtr gstElementFactory);

  /**
  * @brief Creates an ElementFactory from the factory name
  * @param factoryName The name of the element factory
  * @return An ElementFactory instance wrapping the corresponding GstElementFactory
  */
  [[nodiscard]] static ElementFactory fromFactoryName(const std::string& factoryName);

  // bring move semantics back
  ElementFactory(ElementFactory&& other) noexcept = default;
  ElementFactory& operator=(ElementFactory&&) noexcept = default;

  /**
   * @brief Retrieves metadata of the element factory for a given key
   * @param key The metadata key to retrieve
   * @return The metadata value for the given key, or an empty string if the key doesn't exist
   */
  [[nodiscard]] std::string getMetaData(const std::string& key) const;

  /**
   * @brief Retrieves all available metadata keys for the element factory
   * @return A vector of strings containing the metadata keys
   */
  [[nodiscard]] std::vector<std::string> getMetadataKeys() const;

  /**
  * @brief Creates a new element from the factory
  * @param name The name to assign to the created element
  * @return A pointer to the created element
  */
  [[nodiscard]] std::shared_ptr<Element> createElement(const std::string& elementName) const;

  /**
   * @brief Creates an Element from a factory name and optional element name
   * @param factoryName The name of the element factory
   * @param elementName The name of the element to create (optional)
   * @return An Element instance wrapping the created GstElement
   */
  [[nodiscard]] static std::shared_ptr<Element> makeElement(const std::string& factoryName, const std::string& elementName = "");

  /**
   * @brief Gets the type of the element factory
   * @return The type of the element factory
   */
  [[nodiscard]] GType getElementType() const;

  /**
   * @brief get the GstElementSPtr of the Element
   * @return the internal GstElement.
   */
  [[nodiscard]] GstElementFactorySPtr getGstElementFactory();

  [[nodiscard]] const GstElementFactorySPtr getGstElementFactory() const;


private:
  [[nodiscard]] GstElementFactory* getRawGstElementFactory();
  [[nodiscard]] const GstElementFactory* getRawGstElementFactory() const;
};

} // namespace dh::gst

#endif // DH_GST_ELEMENTFACTORY_HPP
