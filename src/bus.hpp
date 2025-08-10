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

#ifndef DH_GST_BUS_HPP
#define DH_GST_BUS_HPP

#include "object.hpp"
#include "sharedptrs.hpp"

// boost
#include <boost/signals2.hpp>

namespace bs2 = boost::signals2;

namespace dh ::gst
{

class Bus : public Object
{
protected:
  /**
   * @brief Create a new Bus object that wraps a GstBusSPtr.
   * @param gstBus
   */
  explicit Bus(GstBusSPtr gstBus);

  /**
   * @brief Create a new Bus object that wraps a GstBus.
   * @ref makeGstSharedPtr is used to wrap in a internal shared_ptr
   * @param gstBus
   * @param transferType see if None, then increase use count
   */
  Bus(GstBus* gstBus, TransferType transferType);

public:
  [[nodiscard]] static std::shared_ptr<Bus> create(GstBusSPtr gstBus);
  [[nodiscard]] static std::shared_ptr<Bus> create(GstBus* gstBus, TransferType transferType);

  /**
   * @brief get the GstBusSPtr of the Bus
   * @return the internal GstBus.
   */
  [[nodiscard]] GstBusSPtr getGstBus();
  [[nodiscard]] const GstBusSPtr getGstBus() const;

  /**
   * @brief Posts a message on the bus.
   */
  void post(const GstMessageSPtr message);

  /**
   * @brief  A message has been posted on the bus.
   * This signal is emitted from the thread that posted the message so one has to be careful with locking.
   * @todo Calling this function creates a new glib signal connection, no matter if that was already done.
   *       This could be improved by keeping track of bs2 signals (in GObject data)
   */
  [[nodiscard]] bs2::signal<void(GstMessageSPtr)>& newSyncMessageSignal() const;

  // TODO: Enable "message" also? (only valid with existing glib main loop)
private:
  [[nodiscard]] GstBus* getRawGstBus();
  [[nodiscard]] const GstBus* getRawGstBus() const;

};


} // dh::gst

#endif //DH_GST_BUS_HPP
