/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -'- */
/**
 * @file objecttraits.hpp
 * @author Sandro Stiller
 * @date 2024-10-11
 */

#ifndef DH_GST_OBJECTTRAITS_HPP
#define DH_GST_OBJECTTRAITS_HPP

// local includes
#include "sharedptrs.hpp"

// std
#include <string>
#include <memory>

// C libs
#include <gst/gst.h>

namespace dh::gst
{

// free functions & type traits used for signal connections, TODO: move to another header?

// Type conversion traits
template<typename GlibType, typename = void>
struct ConvertToGlibType
{
  using type = GlibType; // Default: no conversion
};
template<> struct ConvertToGlibType<std::string>
{
  using type = const gchar*;
};

template<typename GlibType>
struct ConvertToGlibType<std::shared_ptr<GlibType>,
                          std::enable_if_t<
                            IsGstObject<GlibType>::value
                            || IsGstMiniObject<GlibType>::value
                          >
                        >
{
  // Static assertion to produce a compilation error if GlibType is a pointer to a GstObject
  static_assert(
    !(std::is_pointer<GlibType>::value && IsGstObject<std::remove_pointer_t<GlibType>>::value),
    "GlibType cannot be a raw pointer to a GstObject."
  );
  using type = GlibType*;
};


template<typename GlibType, typename = void>
struct ConvertToCppType
{
  using type = GlibType; // Default: no conversion
};

template<> struct ConvertToCppType<gchar*>
{
  using type = std::string;
};

template<> struct ConvertToCppType<const gchar*>
{
  using type = std::string;
};

template<typename GlibType>
struct ConvertToCppType<GlibType*, std::enable_if_t<IsGstObject<GlibType>::value || IsGstMiniObject<GlibType>::value>>
{
  using type = std::shared_ptr<GlibType>;
};


// General case: For types not needing special handling
template<typename T>
typename std::enable_if<
  !IsGObjectType<typename std::remove_pointer<T>::type>::value
   && !IsGstMiniObject<typename std::remove_pointer<T>::type>::value
  ,T>::type
convertParamToCppType(T value)
{
  return value;
}

// Overload for gchar* (null-terminated C strings)
inline std::string convertParamToCppType(gchar* value)
{
  return value ? std::string(value) : std::string();
}

// Overload for const gchar*
inline std::string convertParamToCppType(const gchar* value)
{
  return value ? std::string(value) : std::string();
}

template<typename GstType>
inline std::enable_if_t<IsGstObject<GstType>::value || IsGstMiniObject<GstType>::value, std::shared_ptr<GstType>>
convertParamToCppType(GstType* value)
{
  return makeGstSharedPtr(value, TransferType::None);
}



} // dh::gst
#endif //DH_GST_OBJECTTRAITS_HPP
