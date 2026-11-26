/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/* Copyright (C) 2026 Sandro Stiller <sandro.stiller@dragonhills.de>
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This file is part of Libdhgst <https://dragonhills.de/>.
 *
 * Libdhgst is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Libdhgst is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Libdhgst. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DH_GST_INTERNAL_APPCALLBACKS_HPP
#define DH_GST_INTERNAL_APPCALLBACKS_HPP

#include <glib-object.h>

#include <memory>
#include <mutex>

namespace dh::gst
{

/**
 * Helpers for the callbacks of appsink and appsrc, which are installed with a set_callbacks function
 * (gst_app_sink_set_callbacks, gst_app_src_set_callbacks): one struct of C function pointers and one
 * user data pointer per object, instead of GObject signals.
 *
 * The callback slots (a struct Slots of CallbackSlots) belong to the GObject, not to a C++ wrapper,
 * so all wrappers of the same object share them. They are owned twice:
 * - by the object as qdata, so every wrapper finds them,
 * - by the installed callbacks as user data, so they stay valid as long as the callbacks can be called.
 * Slots must have a member `std::atomic<bool> installed{false}`: it is reset when the callbacks are
 * replaced by another set_callbacks call, so they are installed again on the next connect.
 */

/**
 * @brief The Slots passed as user data to a callback installed by getInstalledCallbackSlots().
 */
template<typename Slots>
Slots& slotsFromUserData(gpointer userData)
{
  return **static_cast<std::shared_ptr<Slots>*>(userData);
}

/**
 * @brief Get the Slots stored on the object, create them and install the callbacks if needed.
 * @param object the GObject, e.g. a GstAppSink
 * @param quark the qdata key of the Slots, unique for Slots
 * @param callbacks the C callbacks, they get the Slots as user data (see slotsFromUserData())
 * @param setCallbacks e.g. gst_app_sink_set_callbacks
 */
template<typename Slots, typename GstType, typename Callbacks>
std::shared_ptr<Slots> getInstalledCallbackSlots(
  GstType& object,
  GQuark quark,
  Callbacks callbacks,
  void (*setCallbacks)(GstType*, Callbacks*, gpointer, GDestroyNotify)
)
{
  using SlotsSPtr = std::shared_ptr<Slots>;

  // guards looking up and installing the Slots of any object
  static std::mutex installMutex;
  const std::scoped_lock lock(installMutex);

  auto* slots = static_cast<SlotsSPtr*>(g_object_get_qdata(G_OBJECT(&object), quark));
  if(!slots)
  {
    auto ownedSlots = std::make_unique<SlotsSPtr>(std::make_shared<Slots>());
    slots = ownedSlots.release();
    g_object_set_qdata_full(
      G_OBJECT(&object),
      quark,
      slots,
      [](gpointer data)
      {
        // clang-tidy complains about a simple "delete static_cast<SlotsSPtr*>(data)"
        const std::unique_ptr<SlotsSPtr> owned{static_cast<SlotsSPtr*>(data)};
      }
    );
  }

  if(!(*slots)->installed)
  {
    // allocate before setting the flag: a failed allocation must not leave the callbacks marked as
    // installed. Set the flag before installing: a concurrent set_callbacks call may replace our
    // callbacks right away, and the destroy notify it triggers on our user data must reset the
    // flag.
    auto userData = std::make_unique<SlotsSPtr>(*slots);
    (*slots)->installed = true;
    setCallbacks(
      &object,
      &callbacks,
      userData.release(),
      [](gpointer data)
      {
        const std::unique_ptr<SlotsSPtr> owned{static_cast<SlotsSPtr*>(data)};
        (*owned)->installed = false;
      }
    );
  }
  return *slots;
}

} // namespace dh::gst

#endif // DH_GST_INTERNAL_APPCALLBACKS_HPP
