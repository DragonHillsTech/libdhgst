/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */

#ifndef DH_GST_BIN_HPP
#define DH_GST_BIN_HPP

#include "element.hpp"
#include "sharedptrs.hpp"

namespace dh::gst
{
class Bin : public Element
{
public:
  /**
   * @brief Create a new Bin object that wraps a GstBinSPtr.
   * @param gstBin
   */
  explicit Bin(GstBinSPtr gstBin);

  /**
   * @brief Create a new Bin object that wraps a GstBin*.
   * @ref makeGstSharedPtr is used to wrap in a internal shared_ptr
   * @param gstBin
   * @param transferType see if None, then increase use count
   */
  Bin(GstBin* gstBin, TransferType transferType);

  /**
  * @brief create an empty Bin with the given name
  * @param name the name of the Bin
  */
  explicit Bin(const std::string& name);

  /**
  * @brief Creates a Bin object from a pipeline description.
  * @param description The GStreamer pipeline description.
  * @param ghostUnlinkedPads whether to automatically create ghost pads for unlinked source or sink pads within the bin
  * @return A Bin object created from the description.
  * @throws std::runtime_error if the description is invalid or parsing fails.
  */
  [[nodiscard]] static Bin fromDescription(const std::string& description, bool ghostUnlinkedPads = false);

  /**
  * @brief create a reference to the same Bin
  * @return the new Bin with the same internal GstBin*
  */
  [[nodiscard]] Bin ref();

 /**
   * @brief get the GstBinSPtr of the Bin
   * @return the GstBin.
   * @todo use getGstObject instead of getGstElement
   */
  [[nodiscard]] GstBinSPtr getGstBin();
  [[nodiscard]] const GstBinSPtr getGstBin() const;

  /**
  * @brief Adds an element to the GstBin using a shared pointer.
  * @param element The Element to add.
  * @throws std::runtime_error if the element cannot be added.
  */
  void addElement(GstElementSPtr element);

  /**
   * @brief Adds an element to the GstBin using a reference.
   * @param element The Element to add.
   * @throws std::runtime_error if the element cannot be added.
   */
  void addElement(Element& element);

 /**
  * @brief Retrieves an element by its name. This function recurses into child bins.
  * @param name The name of the element.
  * @return The element if found
  * @throws std::runtime_error if the element cannot be found.
  */
  [[nodiscard]] Element getElementByName(const std::string& name);

  /**
   * @brief Retrieves an element by its name. If the element is not found, a recursion is performed on the parent bin.
   * @param name
   * @param name The name of the element.
   * @return The element if found
   * @throws std::runtime_error if the element cannot be found.
   */
  [[nodiscard]] Element getElementByNameRecurseUp(const std::string& name);

  /**
   * @brief Removes an element from the GstBin using a shared pointer.
   * @param element The Element to remove.
   * @throws std::runtime_error if the element cannot be removed.
   */
  void removeElement(GstElementSPtr element);

  /**
   * @brief Removes an element from the GstBin using a reference.
   * @param element The Element to remove.
   * @throws std::runtime_error if the element cannot be removed.
   */
  void removeElement(Element& element);

private:
  [[nodiscard]] const GstBin* getRawGstBin() const;
  [[nodiscard]] GstBin* getRawGstBin();
};

} // dh::gst


#endif //DH_GST_BIN_HPP
