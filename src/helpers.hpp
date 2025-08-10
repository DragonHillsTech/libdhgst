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
