/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
/**
 * @file object.hpp
 * @author Sandro Stiller
 * @date 2024-09-12
 */

#ifndef DH_GST_OBJECT_HPP
#define DH_GST_OBJECT_HPP

// local includes
#include "sharedptrs.hpp"
#include "transfertype.hpp"

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
class Object
{
protected:
 /**
  * @brief move constructor is protected because the (abstract) GstObject can and shall not be created directly
  * Just implemented as default. That means, prv of other invalid. Is that okay?
  */
 Object(Object&& other) noexcept;
 Object& operator=(Object&& other) noexcept;

 Object(const Object& other) = delete; // we can not simply copy that thing
 Object& operator=(const Object&) = delete; // we can not simply copy that thing

public:
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
  Object(GstObject* gstObject, TransferType transferType = TransferType::None);

  virtual ~Object();

  /**
   * @brief create a reference to the same GstObject
   * @return the new Object with the same internal GstObject*
   */
  [[nodiscard]] Object ref();

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

  template<typename ValueType>
  [[nodiscard]] inline ValueType getProperty(const std::string& name) const;

  template<typename ValueType>
  inline void setProperty(const std::string& name, const ValueType& value);

protected:
 [[nodiscard]] const GstObject* getRawGstObject() const;
 [[nodiscard]] GstObject* getRawGstObject();

 private:
    class Private;
    std::unique_ptr<Private> prv;
};

// implemetation template functions
template<typename ValueType>
[[nodiscard]] inline ValueType Object::getProperty(const std::string& name) const
{
  if(name.empty())
  {
    throw std::invalid_argument("empty property name");
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
} // dh::gst

#endif //OBJECT_HPP
