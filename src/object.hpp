/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
/* Copyright (C) 2024 Sandro Stiller <sandro.stiller@dragonhills.de>
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

#ifndef DH_GST_OBJECT_HPP
#define DH_GST_OBJECT_HPP

// local includes
#include "gvaluetraits.hpp"
#include "objecttraits.hpp"
#include "signalconnection.hpp"
#include "sharedptrs.hpp"
#include "transfertype.hpp"

// std
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

// C
#include <gst/gst.h>

namespace dh::gst
{

/**
 * @class Object
 * @brief A wrapper class for GstObject, providing additional functionalities.
 */
class Object : public std::enable_shared_from_this<Object>
{
public:
  Object(Object&& other) = delete;
  Object& operator=(Object&& other) = delete;

  Object(const Object& other) = delete; // we can not simply copy that thing
  Object& operator=(const Object&) = delete; // we can not simply copy that thing

protected:
  /**
   * @brief Create a new Object that wraps a GstObject
   * @param gstObject
   */
  Object(GstObjectSPtr gstObject);

   /**
   * @brief Create a new Object that wraps a GstObject.
   * @ref makeGstSharedPtr is used to wrap in a internal shared_ptr
   * @param gstObject
   * @param transferType see if None, then increase use count
   */
  Object(GstObject* gstObject, TransferType transferType);

public:
  virtual ~Object();

  /**
   * @brief get the GstObjectSPtr of the Object
   * @return the internal GstObject.
   * @throws std::logic_error if internal GstObject is empty (moved)
   */
  [[nodiscard]] GstObjectSPtr getGstObject();

  /**
   * @brief get the GstObjectSPtr of the Object
   * @return the internal GstObject.
   * @throws std::logic_error if internal GstObject is empty (moved)
   */
  [[nodiscard]] const GstObjectSPtr getGstObject() const;

  /**
   * @brief Gets the name of the GStreamer object.
   * @return std::string The name of the `GstObject`.
   */
  [[nodiscard]] std::string getName() const;

  /**
   * @brief sets a new name for the Object
   * @param name the new name. If empty, set a unique name.
   * @throws std::logic_error if the name can not be set (if Object has a parent)
   */
  void setName(const std::string& name);

  /**
   * @brief Check if a specific signal exists on the GStreamer element.
   * @param signalName The name of the signal to check.
   * @return true if the signal exists, false otherwise.
   * @throws std::invalid_argument if the signal name is empty.
   */
  [[nodiscard]] bool signalExists(const std::string& signalName) const;

  /**
   * @brief Checks if a property with the given name exists on the GObject.
   * @param name The name of the property to check.
   * @return true if the property exists, false otherwise.
   * @throws std::invalid_argument if the property name is empty.
   */
  [[nodiscard]] bool propertyExists(const std::string& name) const;

  void disconnectSignal(SignalConnection& connection) const;

  /**
   * @brief Read a property of the GObject.
   * @tparam ValueType must match the property type exactly, see GValueTraits for supported types.
   * @param name The name of the property.
   * @return the property value. Object and caps properties are returned as owning shared_ptr.
   * @throws std::invalid_argument if the property does not exist, is not readable
   *         or ValueType does not match the property type.
   */
  template<typename ValueType>
  [[nodiscard]] ValueType getProperty(const std::string& name) const;

  /**
   * @brief Write a property of the GObject.
   * @tparam ValueType must match the property type exactly, see GValueTraits for supported types.
   *         String literals are accepted for string properties.
   * @param name The name of the property.
   * @param value The new value.
   * @throws std::invalid_argument if the property does not exist, is not writable, is construct-only
   *         or ValueType does not match the property type.
   */
  template<typename ValueType>
  void setProperty(const std::string& name, const ValueType& value);

protected:
  [[nodiscard]] const GstObject* getRawGstObject() const;
  [[nodiscard]] GstObject* getRawGstObject();

  /**
   * @brief Check that the wrapped object is an instance of expectedType (or a subclass).
   * Wrapper constructors call this, because the GST_*_CAST macros used to pass C pointers do not check types.
   * @throws std::invalid_argument if the wrapped object has a different type.
   */
  void requireType(GType expectedType) const;

/**
* @brief Connect a callback directly to the GObject signal with the matching name.
* @throws std::runtime_error if signal could not be connected
*/
template<typename... Args>
SignalConnection connectGobjectSignal(
  const std::string& signalName,
  std::function<void(Args...)> callback
) const;

private:
  /**
   * @brief Find a property by name.
   * @throws std::invalid_argument if the name is empty or the property does not exist.
   */
  [[nodiscard]] const GParamSpec* findProperty(const std::string& name) const;

  /**
   * @brief Find a property that can be read.
   * @throws std::invalid_argument if the property does not exist or is not readable.
   */
  [[nodiscard]] const GParamSpec* findReadableProperty(const std::string& name) const;

  /**
   * @brief Find a property that can be written after construction.
   * @throws std::invalid_argument if the property does not exist, is not writable or is construct-only.
   */
  [[nodiscard]] const GParamSpec* findWritableProperty(const std::string& name) const;

  /**
   * @throws std::invalid_argument always, with a message about the type mismatch.
   */
  [[noreturn]] static void throwPropertyTypeMismatch(const GParamSpec* propertySpec);

  GstObjectSPtr gstObject;

  // SignalHandler to define the callback function
  template<typename... Args>
  struct SignalHandler
  {
    static void callback(GObject* object, Args... args, gpointer user_data)
    {
      using Callback = std::function<void(typename ConvertToCppType<Args>::type...)>;
      auto* callback = static_cast<Callback*>(user_data);
      if(! callback)
      {
        return;
      }

      // We are called from C code (GLib signal emission, often on a streaming thread).
      // Exceptions must not unwind through it, so catch and log them here.
      try
      {
        (*callback)(convertParamToCppType(args)...);
      }
      catch(const std::exception& e)
      {
        GST_ERROR_OBJECT(object, "Exception in signal callback: %s", e.what());
      }
      catch(...)
      {
        GST_ERROR_OBJECT(object, "Unknown exception in signal callback");
      }
    }
  };
};

template<typename ValueType>
ValueType Object::getProperty(const std::string& name) const
{
  using Traits = GValueTraits<ValueType>;

  const GParamSpec* propertySpec = findReadableProperty(name);
  if(! Traits::matches(propertySpec->value_type))
  {
    throwPropertyTypeMismatch(propertySpec);
  }

  detail::GValueHolder value(propertySpec->value_type);
  g_object_get_property(
    G_OBJECT(const_cast<GstObject*>(getRawGstObject())),
    name.c_str(),
    value.get()
  );
  return Traits::get(value.get());
}

template<typename ValueType>
void Object::setProperty(const std::string& name, const ValueType& value)
{
  // decay so string literals (char arrays) map to const char*
  using Traits = GValueTraits<std::decay_t<ValueType>>;

  const GParamSpec* propertySpec = findWritableProperty(name);
  if(! Traits::matches(propertySpec->value_type))
  {
    throwPropertyTypeMismatch(propertySpec);
  }

  detail::GValueHolder gvalue(propertySpec->value_type);
  Traits::set(gvalue.get(), value);
  g_object_set_property(
    G_OBJECT(getRawGstObject()),
    name.c_str(),
    gvalue.get()
  );
}

/** Connect a callback to a GObject signal.
  * @tparam Args types you want to use. GstObject* types are not allowed, use shared_ptr instead.
  * @param signalName the name of the signal
  * @param callback the callback invoked when the signal is emitted
  * @throws std::invalid_argument if signal name invalid or signal with the name not found.
  * @see Object::signalExists
  */
template<typename ... Args>
SignalConnection Object::connectGobjectSignal(
  const std::string& signalName,
  std::function<void(Args...)> callback
) const
{
  static_assert(
    (... && (! (std::is_pointer<Args>::value && IsGstObject<std::remove_pointer_t<Args>>::value))),
    "Error: Template arguments cannot be of type GstObject*. Use shared_ptr instead, see sharedptrs.hpp"
  );
  if(signalName.empty())
  {
    throw std::invalid_argument("empty signal name");
  }
  if(! signalExists(signalName))
  {
    throw std::invalid_argument("No signal with name " + signalName);
  }
  if(! callback)
  {
    throw std::invalid_argument("empty signal callback");
  }

  using Callback = std::function<void(Args...)>;
  auto callbackData = std::make_unique<Callback>(std::move(callback));

  // Connect the signal
  const auto connectionId = g_signal_connect_data(
    const_cast<GstObject*>(getRawGstObject()),
    signalName.c_str(),
    reinterpret_cast<GCallback>(SignalHandler<typename ConvertToGlibType<Args>::type...>::callback),
    callbackData.get(),
    [](gpointer data, GClosure* /*closure*/)
    {
      delete static_cast<Callback*>(data);
    },
    G_CONNECT_AFTER
  );

  if(! connectionId)
  {
    throw std::runtime_error("failed to connect signal " + signalName);
  }
  callbackData.release();
  return SignalConnection(
    G_OBJECT(const_cast<GstObject*>(getRawGstObject())),
    connectionId
  );
}

} // dh::gst

#endif //OBJECT_HPP
