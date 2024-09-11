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
  Bin(GstBinSPtr gstBin);

  /**
   * @brief Create a new Bin object that wraps a GstBin*.
   * @ref makeGstSharedPtr is used to wrap in a internal shared_ptr
   * @param gstBin
   * @param transferType see if None, then increase use count
   */
  Bin(GstBin* gstBin, TransferType transferType = TransferType::None);

  virtual ~Bin();

 /**
   * @brief get the GstBinSPtr of the Bin
   * @return the GstBin.
   * @todo use getGstObject instead of getGstElement
   */
  GstBinSPtr getGstBin();

//private:
//    class Private;
//    std::unique_ptr<Private> prv;
};

} // dh::gst


#endif //DH_GST_BIN_HPP
