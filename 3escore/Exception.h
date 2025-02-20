//
// Author: Kazys Stepanas
//
#pragma once

#include "CoreConfig.h"

#include <string>

namespace tes
{
/// Exception type used by this library.
class TES_CORE_API Exception : public std::exception
{
public:
  /// Constructor
  /// @param msg Exception message
  /// @param filename File name where the exception was thrown from, if available.
  /// @param line_number Line nubmer where the exception was thrown, if available.
  Exception(const char *msg = nullptr, const char *filename = nullptr, int line_number = 0);
  /// Copy constructor.
  /// @param other
  Exception(const Exception &other);
  /// Move constructor.
  /// @param other
  Exception(Exception &&other) noexcept;

  /// Destrutor.
  ~Exception() override;

  /// Get the exception message.
  [[nodiscard]] const char *what() const noexcept override;

  /// Copy/move assignment.
  /// @param other Object to copy/move.
  /// @return @c *this
  Exception &operator=(const Exception &other)
  {
    Exception local(other);
    local.swap(*this);
    return *this;
  }

  /// Move assignment.
  /// @param other Object to copy/move.
  /// @return @c *this
  Exception &operator=(Exception &&other) noexcept;

  /// Swap the contents of @c this with @p other.
  /// @param other Object to swap with.
  void swap(Exception &other) noexcept;

  /// External swap function.
  /// @param first An object to swap.
  /// @param second An object to swap.
  friend void swap(Exception &first, Exception &second) { first.swap(second); }

private:
  std::string _message;
};
}  // namespace tes
