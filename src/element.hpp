/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */

#ifndef DH_GST_ELEMENT_H
#define DH_GST_ELEMENT_H

// local includes
#include "sharedptrs.hpp"
#include "transfertype.hpp"

// std
#include <memory>
#include <vector>

// C
#include <gst/gst.h>

namespace dh::gst
{

/**
 * @class Element
 * @brief A wrapper class for GstElement, providing additional functionalities.
 */
class Element
{
public:
  /**
   * @brief Type alias for signal handler ID to improve code readability.
   */
  using HandlerId = gulong;

  /**
   * @brief Create a new Element object that wraps a GstElementSPtr.
   * @param gstElement
   */
  Element(GstElementSPtr gstElement);

  /**
   * @brief Create a new Element object thet wraps a GstElement*.
   * @ref makeGstSharedPtr is used to wrap in a internal shared_ptr
   * @param gstElement
   * @param transferType see if None, then increase use count
   */
  Element(GstElement* gstElement, TransferType transferType = TransferType::None);

  Element(const Element& other) = delete; // we can not simply copy that thing at the moment

  /**
   * @todo checking ref cnt for stopping and unref is not thread safe
   */
  virtual ~Element();

  /**
   * @brief create a reference to the same Element
   * @return the new Element with the same internal GstElement*
   */
  Element ref();

  /**
   * @brief get the GstElementSPtr of the Element
   * @return the internal GstElement.
   */
  GstElementSPtr getGstElement();


   /**
   * @brief Gets the name of the GStreamer element.
   * Retrieves the name of the underlying `GstElement` associated with this `Element`.
   * @return std::string The name of the `GstElement`.
   */
  std::string getName() const;

  GstStateChangeReturn setState(GstState newState);

 /**
  * @brief Gets all pads of the GStreamer element.
  * @return (transfer none): A vector containing pointers to all GstPad objects.
  * @note The caller does not own the returned GstPad pointers. Use gst_object_ref() if you need to keep them.
  */
 std::vector<GstPad*> getPads();

 /**
  * @brief Gets all sink pads of the GStreamer element.
  * @return (transfer none): A vector containing pointers to sink GstPad objects.
  * @note The caller does not own the returned GstPad pointers. Use gst_object_ref() if you need to keep them.
  */
 std::vector<GstPad*> getSinkPads();

 /**
  * @brief Gets all source pads of the GStreamer element.
  * @return (transfer none): A vector containing pointers to source GstPad objects.
  * @note The caller does not own the returned GstPad pointers. Use gst_object_ref() if you need to keep them.
  */
 std::vector<GstPad*> getSrcPads();

 /**
  * @brief Finds a compatible pad for a given pad and caps.
  * @param pad (transfer none): The GstPad to find a compatible pad for.
  * @param caps (transfer none): The GstCaps that the compatible pad must match.
  * @return (transfer none): A pointer to the compatible GstPad, or nullptr if none found.
  * @note The caller does not own the returned GstPad pointer. Use gst_object_ref() if you need to keep it.
  */
 GstPad* getCompatiblePad(GstPad* pad, GstCaps* caps);

 /**
  * @brief Gets a static pad by name.
  * @param name The name of the pad to retrieve.
  * @return (transfer none): A pointer to the GstPad if found, nullptr otherwise.
  * @note The caller does not own the returned GstPad pointer. Use gst_object_ref() if you need to keep it.
  */
 GstPad* getStaticPad(const std::string& name);

 /**
  * @brief Links this element with another element.
  * @param other (transfer none): The Element to link with.
  * @return Reference to the current Element for chaining.
  * @throws std::runtime_error if the elements cannot be linked.
  */
 Element& link(Element& other);

   /**
   * @brief Gets the start time of the element.
   * @return The start time in nanoseconds.
   */
  GstClockTime getStartTime() const;

  /**
   * @brief Gets the current state of the element.
   * @return The current GstState of the element.
   */
  GstState getState() const;

  /**
   * @brief Unlinks the element from another element.
   * @param other The Element to unlink from.
   */
  void unlink(Element& other);

  /**
   * @brief Synchronizes the state of the element with its parent.
   * @return true if the synchronization was successful, false otherwise.
   * @todo throw on error
   */
  bool syncStateWithParent();

  /**
   * @brief Check if a specific signal exists on the GStreamer element.
   * @param signalName The name of the signal to check.
   * @return true if the signal exists, false otherwise.
   */
  bool signalExists(const std::string& signalName) const;

private:
    class Private;
    std::unique_ptr<Private> prv;
};


} // dh::gst

#endif //DH_GST_ELEMENT_H
