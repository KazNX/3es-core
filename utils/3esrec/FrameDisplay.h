#pragma once

#include <atomic>
#include <cinttypes>
#include <memory>
#include <thread>

namespace tes
{
/// Secondary thread for displaying frame progress. Use start()/stop() to manage the thread.
class FrameDisplay
{
public:
  /// Constructor
  FrameDisplay();

  /// Destructor: attempt to ensure thread terminates cleanly.
  ~FrameDisplay();

  /// Increment the current frame value by 1.
  void incrementFrame() { ++_frame_number; }

  /// Increment the current frame by a given value.
  /// @param increment The increment to add.
  void incrementFrame(int64_t increment) { _frame_number += increment; }

  /// Reset frame number to zero.
  void reset() { _frame_number.store(0); }

  /// Start the display thread. Ignored if already running.
  void start();

  /// Stop the display thread. Ok to call when not running.
  void stop();

private:
  /// Thread loop.
  void run();

  std::atomic_int64_t _frame_number = 0;
  std::atomic_bool _started = false;
  std::atomic_bool _quit = false;
  std::thread _thread = {};
};
}  // namespace tes
