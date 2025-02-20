#pragma once

#include "CoreConfig.h"

#include <functional>
#include <sstream>
#include <string>

namespace tes::log
{
/// Logging levels.
enum class Level
{
  /// Fatal error. Log a message and terminate.
  Fatal,
  /// Error message.
  Error,
  /// Warning message.
  Warn,
  /// General information message.
  Info,
  /// Debug level tracing message.
  Trace
};

/// Logging function signature.
/// @param level The logging level. This is provided for information purposes; any logging prefix
/// will already be added.
/// @param message The message to log.
using LogFunction = std::function<void(Level level, const std::string &message)>;

/// Comparison operator for @c tes::log::Level .
/// @param a First argument to compare.
/// @param b Second argument to compare
/// @return True if `a < b`.
[[nodiscard]] inline bool operator<(Level a, Level b)
{
  return static_cast<int>(a) < static_cast<int>(b);
}

/// Comparison operator for @c tes::log::Level .
/// @param a First argument to compare.
/// @param b Second argument to compare
/// @return True if `a <= b`.
[[nodiscard]] inline bool operator<=(Level a, Level b)
{
  return static_cast<int>(a) <= static_cast<int>(b);
}

/// Comparison operator for @c tes::log::Level .
/// @param a First argument to compare.
/// @param b Second argument to compare
/// @return True if `a == b`.
[[nodiscard]] inline bool operator==(Level a, Level b)
{
  return static_cast<int>(a) == static_cast<int>(b);
}

/// Comparison operator for @c tes::log::Level .
/// @param a First argument to compare.
/// @param b Second argument to compare
/// @return True if `a != b`.
[[nodiscard]] inline bool operator!=(Level a, Level b)
{
  return static_cast<int>(a) != static_cast<int>(b);
}

/// Comparison operator for @c tes::log::Level .
/// @param a First argument to compare.
/// @param b Second argument to compare
/// @return True if `a >= b`.
[[nodiscard]] inline bool operator>=(Level a, Level b)
{
  return static_cast<int>(a) >= static_cast<int>(b);
}

/// Comparison operator for @c tes::log::Level .
/// @param a First argument to compare.
/// @param b Second argument to compare
/// @return True if `a > b`.
[[nodiscard]] inline bool operator>(Level a, Level b)
{
  return static_cast<int>(a) > static_cast<int>(b);
}

/// The default logging function.
/// @param level The logging level. This is provided for information purposes; any logging prefix
/// will already be added.
/// @param message The message to log.
void TES_CORE_API defaultLogger(Level level, const std::string &message);

/// Get the logging function. This is used to log all messages.
/// @return The current loging function.
[[nodiscard]] LogFunction TES_CORE_API logger();
/// Set the logging function.
///
/// Not threadsafe.
/// @param logger
void TES_CORE_API setLogger(LogFunction logger);

/// Log level to string
/// @param level The level to convert.
/// @return The level string text.
[[nodiscard]] const std::string TES_CORE_API &toString(Level level);

/// Parse a string to a log level.
/// @param[out] level Variable to parse into.
/// @param str The string to parse. May exactly match the enum name or a fully lower case matching.
/// @retrun True if parsing succeeds.
bool TES_CORE_API fromString(Level &level, const std::string &str);

/// Get the logging prefix for a particular logging level.
///
/// Of the form @c "[{toString(level)}]"
/// @param level The level to get the prefix for.
/// @return The message prefix.
[[nodiscard]] const std::string TES_CORE_API &prefix(Level level);

/// Log the given message is is. No prefix or newlines are added.
/// @param message Message to log.
void TES_CORE_API log(Level level, const std::string &message);

/// @overload
inline void log(const std::string &message)
{
  return log(Level::Info, message);
}

/// Log a fatal error and terminate execution.
/// @param message Message to log.
void TES_CORE_API fatal(const std::string &message);

/// Helper function for assempling a log message.
/// @param str Stream to append to.
/// @param arg Value to append to the stream.
template <typename T>
void message(std::ostringstream &str, const T &arg)
{
  str << arg << std::flush;
}

/// Helper function for building log messages from the given args.
///
/// Uses streaming operators to append to the string buffer.
/// @param str Stream to append to.
/// @param arg Next value to append to the stream.
/// @param ...args Additional arguments.
template <typename T, typename... Args>
void message(std::ostringstream &str, const T &arg, Args... args)
{
  str << arg;
  message(str, args...);
}

/// Log a fatal error message and throw a @c std::runtime_error .
///
/// All arguments must be convertable to string via the streaming operator to @c std::ostream .
/// @param ...args Arguments to log.
template <typename... Args>
void fatal(Args... args)
{
  std::ostringstream str;
  message(str, prefix(Level::Fatal), args...);
  str << std::endl;
  fatal(str.str());
}

/// Log an error message.
///
/// All arguments must be convertable to string via the streaming operator to @c std::ostream .
/// @param ...args Arguments to log.
template <typename... Args>
void error(Args... args)
{
  std::ostringstream str;
  message(str, prefix(Level::Error), args...);
  str << std::endl;
  log(Level::Error, str.str());
}

/// Log a warning message.
///
/// All arguments must be convertable to string via the streaming operator to @c std::ostream .
/// @param ...args Arguments to log.
template <typename... Args>
void warn(Args... args)
{
  std::ostringstream str;
  message(str, prefix(Level::Warn), args...);
  str << std::endl;
  log(Level::Warn, str.str());
}

/// Log an info message.
///
/// All arguments must be convertable to string via the streaming operator to @c std::ostream .
/// @param ...args Arguments to log.
template <typename... Args>
void info(Args... args)
{
  std::ostringstream str;
  message(str, prefix(Level::Info), args...);
  str << std::endl;
  log(Level::Info, str.str());
}

/// Log a trace level message.
///
/// All arguments must be convertable to string via the streaming operator to @c std::ostream .
/// @param ...args Arguments to log.
template <typename... Args>
void trace(Args... args)
{
  std::ostringstream str;
  message(str, prefix(Level::Trace), args...);
  str << std::endl;
  log(Level::Trace, str.str());
}

inline std::ostream &operator<<(std::ostream &o, tes::log::Level level)
{
  o << toString(level);
  return o;
}

inline std::istream &operator>>(std::istream &in, tes::log::Level &level)
{
  std::string name;
  in >> name;

  if (!fromString(level, name))
  {
    in.setstate(std::istream::failbit);
  }

  return in;
}
}  // namespace tes::log
