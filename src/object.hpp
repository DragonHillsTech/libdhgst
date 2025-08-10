/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
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

#ifndef DH_GST_OBJECT_HPP
#define DH_GST_OBJECT_HPP

// local includes
#include "objecttraits.hpp"
#include "sharedptrs.hpp"
#include "transfertype.hpp"

// boost
#include <boost/signals2.hpp>

// std
#include <memory>
#include <string>

// C
#include <gst/gst.h>
#include <spdlog/spdlog.h>

namespace dh::gst
{

/**
 * @class Object
 * @brief A wrapper class for GstObject, providing additional functionalities.
 */
class Object : public std::enable_shared_from_this<Object>
{
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

 template<typename ValueType>
  [[nodiscard]] inline ValueType getProperty(const std::string& name) const;

  template<typename ValueType>
  inline void setProperty(const std::string& name, const ValueType& value);

protected:
  [[nodiscard]] const GstObject* getRawGstObject() const;
  [[nodiscard]] GstObject* getRawGstObject();

/**
* @brief create boost::signals2 signal which is connected to the GObject signal with the matching name
* @throws std::runtime_error if signal could not be connected
*/
template<typename... Args>
boost::signals2::signal<void(Args...)>& connectGobjectSignal(const std::string& signalName) const;

private:
  GstObjectSPtr gstObject;

  // SignalConnector to hold Boost.Signals2 signal
  template<typename... Args>
  struct SignalConnector
  {
    using ConvertedArgs = boost::signals2::signal<void(typename ConvertToCppType<Args>::type...)>;
    ConvertedArgs signal;
    // the weak ptr is used to make sure that the Object is alive while the callback runs.
    // When finishing the CB, it is still possible that the GstObject is deleted before the complete signal processing is finished.
    // whoever emits a signal must make sure that the GstObject stays alive.
    GWeakRef weakSignalSource;

    SignalConnector(const GstObject* gstObject)
    {
      g_weak_ref_init(&weakSignalSource, const_cast<GstObject*>(gstObject));
    }

    ~SignalConnector()
    {
      g_weak_ref_clear(&weakSignalSource);
    }
  };

  // SignalHandler to define the callback function
  template<typename... Args>
  struct SignalHandler
  {
    static void callback(GObject* /*object*/, Args... args, gpointer user_data)
    {
      SignalConnector<Args...>* connector = static_cast<SignalConnector<Args...>*>(user_data);
      if(connector)
      {
        const auto* signalSource = g_weak_ref_get(&connector->weakSignalSource);
        //TODO: else print warning message?
        if(signalSource)
        {
          connector->signal(convertParamToCppType(args)...);
        }
      }
    }
  };
};

template<typename ValueType>
[[nodiscard]] inline ValueType Object::getProperty(const std::string& name) const
{
  if(name.empty())
  {
    throw std::invalid_argument("empty property name");
  }
  if(! propertyExists(name))
  {
    throw std::invalid_argument("No property with name " + name);
  }

  ValueType value{};
  g_object_get(
    G_OBJECT(getRawGstObject()),
    name.c_str(),
    &value,
    nullptr
  );
  return value;
}

template<>
[[nodiscard]] inline std::string Object::getProperty<std::string>(const std::string& name) const
{
  if(name.empty())
  {
    throw std::invalid_argument("empty property name");
  }
  if(! propertyExists(name))
  {
    throw std::invalid_argument("No property with name " + name);
  }

  gchar* value{nullptr};
  g_object_get(
    G_OBJECT(getRawGstObject()),
    name.c_str(),
    &value,
    nullptr
  );
  std::string result(value ? value : "");
  if(value)
  {
    g_free(value);
  }
}

template<typename ValueType>
  inline void Object::setProperty(const std::string& name, const ValueType& value)
{
  if(name.empty())
  {
    throw std::invalid_argument("empty property name");
  }
  if(! propertyExists(name))
  {
    throw std::invalid_argument("No property with name " + name);
  }

  g_object_set(
    G_OBJECT(getRawGstObject()),
    name.c_str(),
    value,
    nullptr
  );
}

template<>
inline void Object::setProperty<std::string>(const std::string& name, const std::string& value)
{
  if(name.empty())
  {
    throw std::invalid_argument("Property 'name' must not be empty");
  }

  g_object_set(
    const_cast<char*>(name.c_str()),
    value.c_str(),
    nullptr
  );
}

/** Create a boost.signals2 signal to connect to a gobject signal.
  * @tparam Args types you want to use. GstObject* types are not allowed, use shared_ptr instead.
  * @param signalName the name of the signal
  * @throws std::invalid_argument if signal name invalid or signal with the name not found.
  * @see Object::signalExists
  */
template<typename ... Args>
boost::signals2::signal<void(Args...)>& Object::connectGobjectSignal(const std::string& signalName) const
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
  // Create a new SignalConnector
  auto self = shared_from_this();

  auto* connector = new SignalConnector<Args...>(getRawGstObject());

  // Connect the signal
  const auto connectionId = g_signal_connect_data(
    const_cast<GstObject*>(getRawGstObject()),
    signalName.c_str(),
    reinterpret_cast<GCallback>(SignalHandler<typename ConvertToGlibType<Args>::type...>::callback),
    connector,
    [](gpointer data, GClosure* /*closure*/)
    {
      delete static_cast<SignalConnector<Args...>*>(data);
    },
    G_CONNECT_AFTER
  );

  if(! connectionId)
  {
    throw std::runtime_error("failed to connect signal " + signalName);
  }
  // Return the Boost.Signals2 signal reference
  return connector->signal;
}

} // dh::gst

#endif //OBJECT_HPP
