#include "Log.h"

#include "CoreUtil.h"

#include <array>
#include <iostream>

namespace tes::log
{
namespace
{
// Clang tidy considers this a global variable. It should be more of a static variable.
// FIXME(KS): Is there a better way than using a singleton? What about thread safety?
// NOLINTNEXTLINE(readability-identifier-naming, cppcoreguidelines-avoid-non-const-global-variables)
LogFunction s_log_function = {};

const std::array<std::string, 5> level_names = {
  "Fatal", "Error", "Warn", "Info", "Trace",
};

struct DefaultLogFunctionInit
{
  DefaultLogFunctionInit()
  {
    if (!s_log_function)
    {
      s_log_function = defaultLogger;
    }
  }
};
}  // namespace


void defaultLogger(Level level, const std::string &message)
{
  std::ostream &o =
    (static_cast<unsigned>(level) <= static_cast<unsigned>(Level::Error)) ? std::cerr : std::cout;
  o << message;
}


LogFunction logger()
{
  static const DefaultLogFunctionInit logger_init;
  return s_log_function;
}


void setLogger(LogFunction logger)
{
  std::ignore = tes::log::logger();  // Ensure initialisation.
  if (logger)
  {
    s_log_function = std::move(logger);
  }
  else
  {
    // Make sure we can't end up with an empty logger.
    s_log_function = defaultLogger;
  }
}


const std::string &toString(Level level)
{
  return level_names.at(static_cast<unsigned>(level));
}


bool fromString(Level &level, const std::string &str)
{
  for (size_t i = 0; i < level_names.size(); ++i)
  {
    std::string name{ level_names[i] };
    bool matched = str == name;
    if (!matched)
    {
      std::transform(name.begin(), name.end(), name.begin(), [](char ch) { return tolower(ch); });
      matched = name == str;
    }
    if (matched)
    {
      level = static_cast<Level>(i);
      return true;
    }
  }
  return false;
}


const std::string &prefix(Level level)
{
  static const std::array<std::string, 5> prefixes = {
    "[Fatal] : ", "[Error] : ", "[Warn] : ", "[Info] : ", "[Trace] : ",
  };
  return prefixes.at(static_cast<unsigned>(level));
}


void log(Level level, const std::string &message)
{
  logger()(level, message);
}

void fatal(const std::string &message)
{
  log(Level::Fatal, message);
  exit(-1);
  // throw std::runtime_error(message);
}
}  // namespace tes::log
