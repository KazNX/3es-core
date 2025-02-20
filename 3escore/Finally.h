#pragma once

#include "CoreConfig.h"

#include <type_traits>

namespace tes
{
/// A scope based object which invokes a function on exiting the scope.
///
/// Usage:
/// - Construct on the stack with a lambda expression containing the work to do.
/// - Work function is invoked on leaving the current scope.
template <class F>
class FinalAction
{
public:
  explicit FinalAction(const F &func) noexcept
    : _work(func)
  {}
  explicit FinalAction(F &&func) noexcept
    : _work(std::move(func))
  {}

  FinalAction(FinalAction &&other) noexcept
    : _work(std::move(other._work))
    , _invoke(std::exchange(other._invoke, false))
  {}

  FinalAction(const FinalAction &) = delete;
  void operator=(const FinalAction &) = delete;
  void operator=(FinalAction &&) = delete;

  ~FinalAction()
  {
    if (_invoke)
    {
      _work();
    }
  }

private:
  std::function<void()> _work;
  bool _invoke = true;
};


// finally() - convenience function to generate a final_action
template <class F>
[[nodiscard]] auto finally(F &&f) noexcept
{
  return FinalAction<std::decay_t<F>>{ std::forward<F>(f) };
}
}  // namespace tes


// This source file copies functionality from libGSL, copyright below.
// https://github.com/microsoft/GSL

// Copyright (c) 2015 Microsoft Corporation. All rights reserved.
//
// This code is licensed under the MIT License (MIT).
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
