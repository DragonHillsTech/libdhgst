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
  explicit Bin(GstBin* gstBin, TransferType transferType = TransferType::None);

  /**
  * @brief create a reference to the same Bin
  * @return the new Bin with the same internal GstBin*
  */
  Bin ref();

 /**
   * @brief get the GstBinSPtr of the Bin
   * @return the GstBin.
   * @todo use getGstObject instead of getGstElement
   */
  GstBinSPtr getGstBin();

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
};

} // dh::gst


#endif //DH_GST_BIN_HPP
