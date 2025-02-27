#pragma once

#include "3esview/ViewConfig.h"

#include <vector>

namespace tes::view
{
/// A simple frames per second tracking, over N frames
class TES_VIEWER_API FramesPerSecondWindow
{
public:
  /// Construct to track the give number of frames.
  /// @param window_size The number of frame to track.
  FramesPerSecondWindow(unsigned window_size = 100);

  /// Add a frame dt value.
  /// @param dt The last frame time (seconds).
  void push(float dt);

  /// Calculate the average time taken to display each frame.
  /// @return The average frame time (seconds).
  [[nodiscard]] float averageFrameTime() const;

  /// Calculate the average frames per second over the window.
  /// @return A frames per second average.
  [[nodiscard]] float fps() const;

private:
  std::vector<float> _window;
  unsigned _next = 0;
  unsigned _count = 0;
};

}  // namespace tes::view
