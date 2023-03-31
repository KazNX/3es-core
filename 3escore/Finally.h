// Author: Kazys Stepanas
#ifndef TES_CORE_FINALLY_H
#define TES_CORE_FINALLY_H

#include "CoreConfig.h"

#include <functional>

namespace tes
{
/// A scope based object which invokes a function on exiting the scope.
///
/// Usage:
/// - Construct on the stack with a lambda expression containing the work to do.
/// - Work function is invoked on leaving the current scope.
class TES_CORE_API Finally
{
public:
  Finally(std::function<void()> work)
    : _work(std::move(work))
  {}

  ~Finally()
  {
    if (_work)
    {
      _work();
    }
  }

  Finally(const Finally &) = delete;
  Finally(Finally &&) = delete;

  Finally &operator=(const Finally &) = delete;
  Finally &operator=(Finally &&) = delete;

private:
  std::function<void()> _work;
};
}  // namespace tes

#endif  // TES_CORE_FINALLY_H
