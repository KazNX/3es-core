//
// Author: Kazys Stepanas
//
#ifndef TES_CORE_UTIL_ENUM_H
#define TES_CORE_UTIL_ENUM_H

#include <3escore/CoreConfig.h>

#include <type_traits>

// NOLINTBEGIN(bugprone-macro-parentheses)

/// A helper which defines bitwise operations for an enum class which defines bit flag values.
#define TES_ENUM_FLAGS(Enum)                                                     \
  inline Enum operator|(Enum a, Enum b)                                          \
  {                                                                              \
    using IntType = std::underlying_type_t<Enum>;                                \
    return static_cast<Enum>(static_cast<IntType>(a) | static_cast<IntType>(b)); \
  }                                                                              \
                                                                                 \
  inline Enum operator&(Enum a, Enum b)                                          \
  {                                                                              \
    using IntType = std::underlying_type_t<Enum>;                                \
    return static_cast<Enum>(static_cast<IntType>(a) & static_cast<IntType>(b)); \
  }                                                                              \
                                                                                 \
  inline Enum &operator|=(Enum &a, Enum b)                                       \
  {                                                                              \
    a = a | b;                                                                   \
    return a;                                                                    \
  }                                                                              \
                                                                                 \
  inline Enum &operator&=(Enum &a, Enum b)                                       \
  {                                                                              \
    a = a & b;                                                                   \
    return a;                                                                    \
  }                                                                              \
  inline Enum operator~(Enum a)                                                  \
  {                                                                              \
    using IntType = std::underlying_type_t<Enum>;                                \
    return static_cast<Enum>(~static_cast<IntType>(a));                          \
  }                                                                              \
  struct __TES_ENUM_REQUIRE_SEMICOLCON__

// NOLINTEND(bugprone-macro-parentheses)


#endif  // TES_CORE_UTIL_ENUM_H
