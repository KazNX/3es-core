#ifndef TES_VIEW_UTIL_CSTR_PTR_H
#define TES_VIEW_UTIL_CSTR_PTR_H

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

#endif  // TES_VIEW_UTIL_CSTR_PTR_H
