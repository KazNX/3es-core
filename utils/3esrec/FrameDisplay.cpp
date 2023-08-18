#include "FrameDisplay.h"

#include <chrono>
#include <cstdio>
#include <functional>

using namespace tes;

FrameDisplay::FrameDisplay()
  : _frame_number(0)
  , _quit(false)
{}


FrameDisplay::~FrameDisplay()
{
  stop();
}


void FrameDisplay::start()
{
  if (!_started)
  {
    _quit = false;
    _thread = std::thread([this]() { run(); });
    _started = true;
  }
}


void FrameDisplay::stop()
{
  if (_started)
  {
    _quit = true;
    _thread.join();
    _started = false;
    _quit = false;
  }
}


void FrameDisplay::run()
{
  int64_t lastFrame = 0;
  while (!_quit)
  {
    const int64_t frameNumber = _frame_number;

    if (lastFrame > frameNumber)
    {
      // Last frame is larger => takes up more space. Clear the line.
      printf("\r                    ");
    }

    if (lastFrame != frameNumber)
    {
      printf("\r%" PRId64, frameNumber);
      fflush(stdout);
      lastFrame = frameNumber;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Display final frame number.
  const int64_t finalFrame = _frame_number;
  if (finalFrame != lastFrame)
  {
    // Clear the display line.
    printf("\r                    ");
    printf("\r%" PRId64, finalFrame);
  }
}
