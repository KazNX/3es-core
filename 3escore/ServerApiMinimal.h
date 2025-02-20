///
/// This defines a complete server API which can be disabled at either compile time or runtime.
///
//
// author Kazys Stepanas
//
// Copyright (c) Kazys Stepanas 2023
#pragma once

#include <memory>

#ifndef TES
#define TES(statement)
#endif  // TES
#ifndef TES2
#define TES2(cond, statement)
#endif  // TES2
#ifndef TES_IF
#define TES_IF(condition) if constexpr (false)
#endif  // TES_IF

namespace tes
{
//-----------------------------------------------------------------------------
// Declarations.
// Most are redundant when TES_ENABLE is defined. They are present for the
// case when it is not defined.
//-----------------------------------------------------------------------------
class Connection;
class ConnectionMonitor;
class Resource;
class Server;
class Shape;
struct ServerInfoMessage;
struct ServerSettings;

/// Shared pointer definition for a @c Server object.
using ServerPtr = std::shared_ptr<Server>;
using ResourcePtr = std::shared_ptr<const Resource>;
}  // namespace tes
