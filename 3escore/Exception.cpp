//
// Author: Kazys Stepanas
//
#include "Exception.h"

#include <sstream>

namespace tes
{
Exception::Exception(const char *msg, const char *filename, int line_number)
{
  if (filename)
  {
    std::ostringstream o;
    o << filename;
    if (line_number > 0)
    {
      o << '(' << line_number << ')';
    }
    o << ": " << msg;
    _message = o.str();
  }
  else
  {
    _message = msg;
  }
}


Exception::Exception(const Exception &other) = default;
Exception::Exception(Exception &&other) noexcept = default;


Exception::~Exception() = default;


const char *Exception::what() const noexcept
{
  return _message.c_str();
}


Exception &Exception::operator=(Exception &&other) noexcept = default;


void Exception::swap(Exception &other) noexcept
{
  std::swap(_message, other._message);
}
}  // namespace tes
