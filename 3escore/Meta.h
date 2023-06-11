//
// author: Kazys Stepanas
//
// This file contains utility macros and defines mostly used to avoid compiler
// warnings.
//
#ifndef TES_CORE_META_H
#define TES_CORE_META_H

#include <tuple>

// Do not include 3es-core for now. That would be circular.

/// Suppress unused warning.
/// @param x The expression resulting in an unused result.
#define TES_UNUSED(x) std::ignore = (x)

/// @def TES_FALLTHROUGH
/// Switch statement fallthrough directive indicating the lack of @c break is intentional.

#if __cplusplus >= 201703L  // C++17
#define TES_FALLTHROUGH [[fallthrough]]
#elif defined(__GNUC__)
#define TES_FALLTHROUGH [[clang::fallthrough]]
#endif

// Fall back definitions.
#ifndef TES_FALLTHROUGH
#define TES_FALLTHROUGH
#endif  // TES_FALLTHROUGH

#endif  // TES_CORE_META_H
