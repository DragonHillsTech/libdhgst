/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
/**
 * @file bin.hpp
 * @author Sandro Stiller
 */

#include "bin.hpp"

namespace dh::gst
{

Bin::Bin(GstBinSPtr gstBin)
: Element(makeGstSharedPtr(GST_ELEMENT_CAST(gstBin.get()), TransferType::None)) // no pointer_cast because C inheritance
{
}

Bin::Bin(GstBin* gstBin, TransferType transferType)
: Element(GST_ELEMENT_CAST(gstBin), transferType)
{
}

GstBinSPtr Bin::getGstBin()
{
  // we can not use *pointer_cast because these are C types
  return makeGstSharedPtr(GST_BIN_CAST(getGstObject().get()), TransferType::None);
}
} // dh::gst
