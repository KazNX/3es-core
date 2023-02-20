// Copyright (c) 2018
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// ABN 41 687 119 230
//
// Author: Kazys Stepanas
#ifndef TES_CORE_MATHS_STREAM_H
#define TES_CORE_MATHS_STREAM_H

#include "CoreConfig.h"

#include "Colour.h"
#include "Matrix3.h"
#include "Matrix4.h"
#include "Quaternion.h"
#include "Vector3.h"
#include "Vector4.h"

#include "MathsManip.h"

#include <iostream>

/// @defgroup tesiostream IO Stream Operators
// Defines a set of streaming operators for displaying 3es maths types to @c std::iostream.
///
// The display behaviour of some types may be adjusted using some io manipilators.

/// @ingroup tesiostream
/// Write @c Vector3 type to stream.
/// @param o Output stream.
/// @param v Vector to display.
/// @return @p o
template <typename REAL>
inline std::ostream &operator<<(std::ostream &o, const tes::Vector3<REAL> &v)
{
  o << '(' << v.x() << ',' << v.y() << ',' << v.z() << ')';
  return o;
}

/// @ingroup tesiostream
/// Write @c Vector4 type to stream.
///
/// By default, the W component is displayed last. Use @c tesV4WMode() to adjust this behaviour.
///
/// @param o Output stream.
/// @param v Vector to display.
/// @return @p o
template <typename REAL>
inline std::ostream &operator<<(std::ostream &o, const tes::Vector4<REAL> &v)
{
  if (tes::getV4WMode(o) == tes::WMode::Last)
  {
    o << '(' << v.x() << ',' << v.y() << ',' << v.z() << ',' << v.w() << ')';
  }
  else
  {
    o << '(' << v.w() << ',' << v.x() << ',' << v.y() << ',' << v.z() << ')';
  }
  return o;
}

/// @ingroup tesiostream
/// Write @c Quaternion type to stream.
///
/// By default, the W component is displayed last. Use @c tesQuatWMode() to adjust this behaviour.
///
/// @param o Output stream.
/// @param q Quaternion to display.
/// @return @p o
template <typename REAL>
inline std::ostream &operator<<(std::ostream &o, const tes::Quaternion<REAL> &q)
{
  if (tes::getQuatWMode(o) == tes::WMode::Last)
  {
    o << '(' << q.x() << ',' << q.y() << ',' << q.z() << ',' << q.w() << ')';
  }
  else
  {
    o << '(' << q.w() << ',' << q.x() << ',' << q.y() << ',' << q.z() << ')';
  }
  return o;
}

/// @ingroup tesiostream
/// Write @c Matrix3 type to stream.
///
/// By default, all elements are displayed inline. Use @c tesMatrixMode() to adjust this behaviour.
///
/// @param o Output stream.
/// @param m Matrix to display.
/// @return @p o
template <typename REAL>
inline std::ostream &operator<<(std::ostream &o, const tes::Matrix3<REAL> &m)
{
  char endOfRow = (tes::getMatMode(o) == tes::MatrixMode::Inline) ? ',' : '\n';
  o << "[ "                                                              //
    << m.rc[0][0] << ',' << m.rc[0][1] << ',' << m.rc[0][2] << endOfRow  //
    << m.rc[1][0] << ',' << m.rc[1][1] << ',' << m.rc[1][2] << endOfRow  //
    << m.rc[2][0] << ',' << m.rc[2][1] << ',' << m.rc[2][2]              //
    << " ]";
  return o;
}

/// @ingroup tesiostream
/// Write @c Matrix4 type to stream.
///
/// By default, all elements are displayed inline. Use @c tesMatrixMode() to adjust this behaviour.
///
/// @param o Output stream.
/// @param m Matrix to display.
/// @return @p o
template <typename REAL>
inline std::ostream &operator<<(std::ostream &o, const tes::Matrix4<REAL> &m)
{
  char endOfRow = (tes::getMatMode(o) == tes::MatrixMode::Inline) ? ',' : '\n';
  o << "[ "                                                                                   //
    << m.rc[0][0] << ',' << m.rc[0][1] << ',' << m.rc[0][2] << ',' << m.rc[0][3] << endOfRow  //
    << m.rc[1][0] << ',' << m.rc[1][1] << ',' << m.rc[1][2] << ',' << m.rc[1][3] << endOfRow  //
    << m.rc[2][0] << ',' << m.rc[2][1] << ',' << m.rc[2][2] << ',' << m.rc[2][3] << endOfRow  //
    << m.rc[3][0] << ',' << m.rc[3][1] << ',' << m.rc[3][2] << ',' << m.rc[3][3]              //
    << " ]";
  return o;
}

/// @ingroup tesiostream
/// Write @c Colour type to stream, separating RGBA components.
/// @param o Output stream.
/// @param c Colour to display.
/// @return @p o
inline std::ostream &operator<<(std::ostream &o, const tes::Colour &c)
{
  o << '[' << int(c.r()) << ',' << int(c.g()) << ',' << int(c.b()) << ',' << int(c.a()) << ']';
  return o;
}

#endif  // TES_CORE_MATHS_STREAM_H
