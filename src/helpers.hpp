/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file helpers.hpp
 * @author Sandro Stiller
 * @date 2024-10-11
 */
#ifndef DH_GST_HELPERS_HPP
#define DH_GST_HELPERS_HPP

#include <string>
#include <gst/gst.h>

namespace dh::gst::helpers
{

/**
 * @brief return the name of the status.
 * @todo since c++20, constexpr is possible with string
 */
std::string gstStreamStatusTypeToString(GstStreamStatusType type);

} // dh::gst::helpers

#endif //DH_GST_HELPERS_HPP
