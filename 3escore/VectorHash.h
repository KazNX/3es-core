/*
License from the NVIDIA source on which this code is based:
Copyright (c) 2009-2011, NVIDIA Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of NVIDIA Corporation nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "CoreConfig.h"

#include <cinttypes>
#include <cmath>
#include <unordered_map>

namespace tes
{
// NOLINTBEGIN(readability-identifier-length,cppcoreguidelines-avoid-magic-numbers, google-readability-casting)
/// Magic number for vector hashing.
inline constexpr uint32_t TES_CORE_API vectorHashMagic()
{
  return 0x9e3779b9u;
}

// By Bob Jenkins, 1996. bob_jenkins@burtleburtle.net.
inline void TES_CORE_API vectorHashJenkinsMix(uint32_t &a, uint32_t &b, uint32_t &c)
{
  a -= b;
  a -= c;
  a ^= (c >> 13u);
  b -= c;
  b -= a;
  b ^= (a << 8u);
  c -= a;
  c -= b;
  c ^= (b >> 13u);
  a -= b;
  a -= c;
  a ^= (c >> 12u);
  b -= c;
  b -= a;
  b ^= (a << 16u);
  c -= a;
  c -= b;
  c ^= (b >> 5u);
  a -= b;
  a -= c;
  a ^= (c >> 3u);
  b -= c;
  b -= a;
  b ^= (a << 10u);
  c -= a;
  c -= b;
  c ^= (b >> 15u);
}

/// Contains functions for hashing vector3/vector4 style vertices for vertex hash maps.
///
/// This hash technique was taken from NVIDIA open source code.
/// Specifically the code for the paper "Efficient Sparse Voxel Octrees"
namespace vhash
{
/// Generate a hash for 2 to 3 components.
/// @param a First component.
/// @param b Second component.
/// @param c Third component.
inline uint32_t hashBits(uint32_t a, uint32_t b = vectorHashMagic(), uint32_t c = 0)
{
  c += vectorHashMagic();
  vectorHashJenkinsMix(a, b, c);
  return c;
}


/// Generate a hash for 4 to 6 components.
/// @param a First component.
/// @param b Second component.
/// @param c Third component.
/// @param d Fourth component.
/// @param e Fifth component.
/// @param f Sixth component.
inline uint32_t hashBits(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e = 0,
                         uint32_t f = 0)
{
  c += vectorHashMagic();
  vectorHashJenkinsMix(a, b, c);
  a += d;
  b += e;
  c += f;
  vectorHashJenkinsMix(a, b, c);
  return c;
}


#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
/// Generate a hash code for a 3-component vertex.
/// @param x A vector coordinate.
/// @param y A vector coordinate.
/// @param z A vector coordinate.
inline uint32_t hash(float x, float y, float z)
{
  static_assert(sizeof(float) == sizeof(uint32_t));
  return hashBits(*(const uint32_t *)&x, *(const uint32_t *)&y, *(const uint32_t *)&z);
}


/// Generate a hash code for a 4-component vertex.
/// @param x A vector coordinate.
/// @param y A vector coordinate.
/// @param z A vector coordinate.
/// @param w A vector coordinate.
inline uint32_t hash(float x, float y, float z, float w)
{
  static_assert(sizeof(float) == sizeof(uint32_t));
  return hashBits(*(const uint32_t *)&x, *(const uint32_t *)&y, *(const uint32_t *)&z,
                  *(const uint32_t *)&w);
}
}  // namespace vhash
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__


/// Hash structure for use with standard library maps.
/// @tparam The vector3 type. Must support x, y, z members (not functions).
template <class T>
class Vector3Hash
{
public:
  /// Operator to convert the vector @p p to its hash code.
  /// @param p A vector3 object.
  /// @return The 32-bit hash code for @p p.
  inline size_t operator()(const T &p) const { return vhash::hash(p.x(), p.y(), p.z()); }
};
// NOLINTEND(readability-identifier-length,cppcoreguidelines-avoid-magic-numbers, google-readability-casting)
}  // namespace tes
