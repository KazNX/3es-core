//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_UTIL_ENUM_H
#define TES_VIEW_UTIL_ENUM_H

#include <3esview/ViewConfig.h>

// NOLINTBEGIN(bugprone-macro-parentheses)

/// A helper which defines bitwise operations for an enum class which defines bit flag values.
#define TES_ENUM_FLAGS(Enum, IntType)                                            \
  inline Enum operator|(Enum a, Enum b)                                          \
  {                                                                              \
    return static_cast<Enum>(static_cast<IntType>(a) | static_cast<IntType>(b)); \
  }                                                                              \
                                                                                 \
  inline Enum operator&(Enum a, Enum b)                                          \
  {                                                                              \
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
    return static_cast<Enum>(~static_cast<IntType>(a));                          \
  }

// NOLINTEND(bugprone-macro-parentheses)


#endif  // TES_VIEW_UTIL_ENUM_H
