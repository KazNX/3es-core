//
// author: Kazys Stepanas
//
#ifndef _3ESTRANSFERPROGRESS_H_
#define _3ESTRANSFERPROGRESS_H_

#include "CoreConfig.h"

#include <cstdint>

namespace tes
{
/// A structure tracking progress of a data transfer.
///
/// The semantics of @c progress and @p phase depend on usage.
///
/// Most notably used with @c Resource.
struct TES_CORE_API TransferProgress
{
  /// Progress value for the current phase.
  int64_t progress;
  /// Phase value.
  int phase;
  /// Progress complete?
  bool complete;
  /// Transfer failed?
  bool failed;

  /// Reset to zero, incomplete, not failed.
  inline void reset()
  {
    progress = 0;
    phase = 0;
    complete = failed = false;
  }
};
}  // namespace tes

#endif  // _3ESTRANSFERPROGRESS_H_
