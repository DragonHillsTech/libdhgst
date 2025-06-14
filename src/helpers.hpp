/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file helpers.hpp
 * @author Sandro Stiller
 * @date 2024-10-11
 */
#ifndef DH_GST_HELPERS_HPP
#define DH_GST_HELPERS_HPP

#include <gst/gst.h>
#include <gst/video/video.h>

#include <string>
#include <stdexcept>

namespace dh::gst::helpers
{

/**
 * @brief return the name of the status.
 * @todo since c++20, constexpr is possible with string
 */
std::string gstStreamStatusTypeToString(GstStreamStatusType type);

/**
 * @brief Creates and initializes a GstVideoInfo structure from the given GstCaps.
 *
 * @param caps Reference to GstCaps object containing video capabilities.
 * @return A GstVideoInfo structure initialized with the provided caps.
 * @throws std::runtime_error if initialization from caps fails.
 */
GstVideoInfo createVideoInfo(const GstCaps& caps);

/**
 * @brief Creates and initializes a GstVideoInfo structure from the given GstBuffer.
 *
 * @param buffer Reference to GstBuffer object containing video metadata.
 * @return A GstVideoInfo structure initialized with the provided buffer.
 * @throws std::runtime_error if initialization from buffer fails.
 */
GstVideoInfo createVideoInfo(const GstBuffer& buffer);


} // dh::gst::helpers

#endif //DH_GST_HELPERS_HPP
