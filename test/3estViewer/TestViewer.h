#ifndef TES_3EST_VIEWER_VIEWER_H
#define TES_3EST_VIEWER_VIEWER_H

#include "3estViewer/TestViewerConfig.h"

#include <3esview/FrameStamp.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

namespace tes::view
{
namespace data
{
class DataThread;
}  // namespace data
class ThirdEyeScene;
/// An application harness for running a windowless viewer in the test environment.
///
/// This replicates limited functionality from @c tes::view::Viewer omitting UI functionality.
class TestViewer : public TES_MAGNUM_WINDOWLESS_APP
{
public:
  /// The application base class. The underlying type varies between platforms.
  using Application = TES_MAGNUM_WINDOWLESS_APP;
  /// Clock used for timing operations.
  using Clock = std::chrono::steady_clock;
  /// The signature for the function called on each render frame from @c run().
  /// @return True if the simulation should continue.
  using FrameFunction = std::function<bool()>;

  /// Get the default 3es server port.
  /// @return The default server port for 3es.
  static uint16_t defaultPort();

  /// Construct with the given command line arguments.
  /// @param arguments Command line arguments.
  explicit TestViewer(const Arguments &arguments);
  /// Construct with the given command line arguments and configuration.
  /// @param arguments Command line arguments.
  /// @param configuration The apllication configuration.
  TestViewer(const Arguments &arguments, const Configuration &configuration);

  /// Destructor. Magnum base class should be virtual but isn't.
  virtual ~TestViewer();

  /// Instantiate a singleton, shared viewer. A weak singleton reference is kept to this viewer.
  ///
  /// This is to work around an issue whereby the Magnum Graphics windowless app on window fails to
  /// correctly join it's threads, leaving an exception. The exception isn't caught by GTest, so
  /// we can create a global viewer and have it persist for all the test. Test cannot be run
  /// concurrently.
  ///
  /// We use a single instance of the viewer to delay the exception until final exit, allowing all
  /// tests to execute first. Creating a viewer for each test was causing a silent halt before all
  /// tests had run.
  ///
  /// @return The singleton viewer.
  static std::shared_ptr<TestViewer> createViewer();

  /// Get the @c ThirdEyeScene object.
  /// @return The main @c ThirdEyeScene object.
  std::shared_ptr<ThirdEyeScene> tes() const
  {
    std::scoped_lock guard(_mutex);
    return _tes;
  }

  /// Get the @c DataThread if any.
  /// @return The running @c DataThread.
  const std::shared_ptr<data::DataThread> &dataThread() const
  {
    std::scoped_lock guard(_mutex);
    return _data_thread;
  }

  /// Open a 3es playback file using a @c data::StreamThread.
  ///
  /// Always closes the current @c dataThread() first.
  ///
  /// @param path The file path to open.
  /// @return True on success.
  bool open(const std::filesystem::path &path);

  /// Make a 3es network connection using @c data::NetworkThread.
  ///
  /// Always closes the current @c dataThread() first.
  ///
  /// @param host The host IP address string.
  /// @param port The host connection port.
  /// @return True on success.
  bool connect(const std::string &host, uint16_t port);

  /// Close or disconnect the current file or network @c dataThread().
  /// @return True if there was a @c dataThread() to close or disconnect.
  bool closeOrDisconnect();

  /// Set the time at which to quit the @c run() function.
  /// @param when Time after which to quit.
  void quitAtTime(const Clock::time_point &when)
  {
    std::scoped_lock guard(_mutex);
    _end_time = when;
  }

  /// Set the maximum number of frames to render in @c run() after which it should quit.
  /// @param frame_count The frame count.
  void quitAfterFrames(FrameNumber frame_count)
  {
    std::scoped_lock guard(_mutex);
    _quit_after_frames = frame_count;
  }

  /// Quit the @c run() loop.
  void quit() { quitAtTime(Clock::now()); }

  /// @brief
  /// @return
  bool shouldQuit();

  /// Run until @c shouldQuit() is true, optionally calling @c frame_function on each iteration.
  /// @param frame_function Function to invoke on each iteration.
  /// @return An error code value: zero on success.
  int run(const FrameFunction &frame_function = {});

  /// Run until the given time.
  ///
  /// This simply invokes @c quitAtTime() and @c run().
  ///
  /// @param when When to quit.
  /// @param frame_function Function to invoke on each iteration.
  /// @return An error code value: zero on success.
  int runUntil(const Clock::time_point &when, const FrameFunction &frame_function = {})
  {
    quitAtTime(when);
    return run(frame_function);
  }

  /// Run for the specified number of frame steps.
  ///
  /// This simply invokes @c quitAtTime() and @c run().
  ///
  /// @param frame_count Number of frames to run.
  /// @param frame_function Function to invoke on each iteration.
  /// @return An error code value: zero on success.
  int runFor(FrameNumber frame_count, const FrameFunction &frame_function = {})
  {
    quitAfterFrames(frame_count);
    return run(frame_function);
  }

private:
  int exec() override;

  /// Reset for the next test run (if any).
  void reset();

  std::shared_ptr<ThirdEyeScene> _tes;
  std::shared_ptr<data::DataThread> _data_thread;
  std::optional<Clock::time_point> _end_time;
  std::optional<FrameNumber> _quit_after_frames = 0;
  FrameFunction _frame_function = {};
  mutable std::mutex _mutex;
};  // namespace tes::test::view
}  // namespace tes::view

#endif  // TES_3EST_VIEWER_VIEWER_H
