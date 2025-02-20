#pragma once

#include <memory>

namespace tes::view::util
{
struct CStrPtrDeleter
{
  void operator()(const char *ptr)
  {
    if (ptr)
    {
      // Delete of const pointer is valid in C++.
      free(const_cast<char *>(ptr));
    }
  }
  void operator()(char *ptr)
  {
    if (ptr)
    {
      free(ptr);
    }
  }
};

using CStrPtr = std::unique_ptr<char, CStrPtrDeleter>;
using CStrConstPtr = std::unique_ptr<const char, CStrPtrDeleter>;
}  // namespace tes::view::util
