/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*-  */
/**
 * @file transfertype.hpp
 * @author Sandro Stiller
 * @date 2024-09-14
 */

#ifndef TRANSFERTYPE_HPP
#define TRANSFERTYPE_HPP

namespace dh::gst {

enum class TransferType
{
  Full,
  None,
  Floating
};

} // dh::gst

#endif //TRANSFERTYPE_HPP
