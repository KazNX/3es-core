//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_SETTINGS_H
#define TES_VIEW_SETTINGS_SETTINGS_H

#include <3esview/ViewConfig.h>

#include "Camera.h"
#include "Connection.h"
#include "Log.h"
#include "Playback.h"
#include "Render.h"

#include <array>
#include <functional>
#include <mutex>
#include <array>
#include <vector>

namespace tes::view::settings
{
class TES_VIEWER_API Settings
{
public:
  struct Config
  {
    Camera camera;
    Log log;
    Playback playback;
    Render render;
    Connection connection;
  };

  enum class Category : unsigned
  {
    Camera,
    Log,
    Playback,
    Render,
    Connection,

    Count,
    Invalid = Count
  };

  using NotifyCallback = std::function<void(const Config &)>;

  Config config() const;
  void update(const Config &config);
  void update(const Camera &config);
  void update(const Log &config);
  void update(const Playback &config);
  void update(const Render &config);
  void update(const Connection &config);

  void addObserver(NotifyCallback callback);
  void addObserver(Category category, NotifyCallback callback);

private:
  void notify(const Config &config);
  void notify(Category category, const Config &config);

  Config _config;
  mutable std::mutex _mutex;
  mutable std::mutex _observer_mutex;
  std::vector<NotifyCallback> _observers;
  std::array<std::vector<NotifyCallback>, static_cast<unsigned>(Category::Count)> _sub_observers;
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_SETTINGS_H
