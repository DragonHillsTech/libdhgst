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

#ifndef DH_GST_ELEMENT_H
#define DH_GST_ELEMENT_H

// local includes
#include "object.hpp"
#include "sharedptrs.hpp"
#include "transfertype.hpp"

// boost
#include <boost/signals2.hpp>

// std
#include <string>
#include <vector>

// C
#include <gst/gst.h>

namespace bs2 = boost::signals2;

namespace dh::gst
{

/**
 * @class Element
 * @brief A wrapper class for GstElement, providing additional functionalities.
 * The destruction of the last reference to the GstElement* will
 * automatically set the state to null by @ref GstObjectDeleter
 */
class Element : public Object
{
protected:
  /**
   * @brief Create a new Element object that wraps a GstElementSPtr.
   * @param gstElement
   */
  explicit Element(GstElementSPtr gstElement);

  /**
   * @brief Create a new Element object that wraps a GstElement.
   * @ref makeGstSharedPtr is used to wrap in a internal shared_ptr
   * @param gstElement
   * @param transferType see if None, then increase use count
   */
  Element(GstElement* gstElement, TransferType transferType);

 public:
 [[nodiscard]] static std::shared_ptr<Element> create(GstElementSPtr gstElement);
 [[nodiscard]] static std::shared_ptr<Element> create(GstElement* gstElement, TransferType transferType);

  /**
   * @brief get the GstElementSPtr of the Element
   * @return the internal GstElement.
   */
  [[nodiscard]] GstElementSPtr getGstElement();

  [[nodiscard]] const GstElementSPtr getGstElement() const;

  GstStateChangeReturn setState(GstState newState);

 /**
  * @brief Gets all pads of the GStreamer element.
  * @return (transfer none): A vector containing pointers to all GstPad objects.
  * @note The caller does not own the returned GstPad pointers. Use gst_object_ref() if you need to keep them.
  */
  [[nodiscard]] std::vector<GstPad*> getPads();

 /**
  * @brief Gets all sink pads of the GStreamer element.
  * @return (transfer none): A vector containing pointers to sink GstPad objects.
  * @note The caller does not own the returned GstPad pointers. Use gst_object_ref() if you need to keep them.
  */
  [[nodiscard]] std::vector<GstPad*> getSinkPads();

 /**
  * @brief Gets all source pads of the GStreamer element.
  * @return (transfer none): A vector containing pointers to source GstPad objects.
  * @note The caller does not own the returned GstPad pointers. Use gst_object_ref() if you need to keep them.
  */
  [[nodiscard]] std::vector<GstPad*> getSrcPads();

 /**
  * @brief Finds a compatible pad for a given pad and caps.
  * @param pad (transfer none): The GstPad to find a compatible pad for.
  * @param caps (transfer none): The GstCaps that the compatible pad must match.
  * @return (transfer none): A pointer to the compatible GstPad, or nullptr if none found.
  * @note The caller does not own the returned GstPad pointer. Use gst_object_ref() if you need to keep it.
  */
  [[nodiscard]] GstPad* getCompatiblePad(GstPad* pad, GstCaps* caps);

 /**
  * @brief Gets a static pad by name.
  * @param name The name of the pad to retrieve.
  * @return (transfer none): A pointer to the GstPad if found, nullptr otherwise.
  * @note The caller does not own the returned GstPad pointer. Use gst_object_ref() if you need to keep it.
  */
  [[nodiscard]] GstPad* getStaticPad(const std::string& name);

  /**
   * @brief Links this element with another element.
   * @param other (transfer none): The Element to link with.
   * @return Reference to the other Element for chaining.
   * @throws std::runtime_error if the elements cannot be linked.
   * @todo: add test
   */
 std::shared_ptr<Element>& link(std::shared_ptr<Element>& other);


  /**
   * @brief Retrieves the factory that was used to create this element.
   * @return the factory name or empty string if element has not been registered (static element).
   */
  std::string getFactoryName() const;

  /**
   * @brief Gets the currently configured clock of the element.
   * This is the clock as was last set.
   * Elements in a pipeline will only have their clock set when the pipeline is in the PLAYING state.
   * @return the clock of the Element or empty ptr
   */
  GstClockSPtr getElementClock() const;

   /**
   * @brief Gets the start time of the element.
   * @return The start time in nanoseconds.
   */
  [[nodiscard]] GstClockTime getStartTime() const;

  /**
   * @brief Gets the current state of the element.
   * @return The current GstState of the element.
   */
  [[nodiscard]] GstState getState() const;

  /**
   * @brief Unlinks the element from another element.
   * @param other The Element to unlink from.
   */
  void unlink(std::shared_ptr<Element>& other);

  /**
   * @brief Synchronizes the state of the element with its parent.
   * @return true if the synchronization was successful, false otherwise.
   * @throws std::runtime_error if element could not be synced
   */
  void syncStateWithParent();

  // signals
  /**
   * @brief This signals that the element will not generate more dynamic pads.
   * Note that this signal will usually be emitted from the context of the streaming thread.
   * Also keep in mind that if you add new elements to the pipeline in the signal handler
   * you will need to set them to the desired target state with gst_element_set_state or
   * gst_element_sync_state_with_parent.
   * @todo Calling this function creates a new glib signal connection. no matter if that was already done. This could be improved by keeping track of bs2 signals (in GObject data)
   */
  [[nodiscard]] bs2::signal<void()>& noMorePadsSignal() const;

  /**
   * @brief a new GstPad has been added to the element.
   * Note that this signal will usually be emitted from the context of the streaming thread.
   * Also keep in mind that if you add new elements to the pipeline in the signal handler
   * you will need to set them to the desired target state with gst_element_set_state or
   * gst_element_sync_state_with_parent.
   * @todo Calling this function creates a new glib signal connection, no matter if that was already done.
   *       This could be improved by keeping track of bs2 signals (in GObject data)
   */
  [[nodiscard]] bs2::signal<void(GstPadSPtr)>& padAddedSignal() const;

  /**
   * @brief a GstPad has been removed from the element
   */
  [[nodiscard]] bs2::signal<void(GstPadSPtr)>& padRemovedSignal() const;

private:
  [[nodiscard]] const GstElement* getRawGstElement() const;
  [[nodiscard]] GstElement* getRawGstElement();
};


} // dh::gst

#endif //DH_GST_ELEMENT_H
