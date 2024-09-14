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

 private:
    class Private;
    std::unique_ptr<Private> prv;

   [[nodiscard]] const GstObject* getRawGstObject() const;
   [[nodiscard]] GstObject* getRawGstObject();
};

} // dh::gst

#endif //OBJECT_HPP
