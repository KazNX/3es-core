//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include "Camera.h"
#include "Connection.h"
#include "Extension.h"
#include "Log.h"
#include "Playback.h"
#include "Render.h"

#include <array>
#include <functional>
#include <mutex>
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
    std::vector<Extension> extentions;


    [[nodiscard]] inline bool operator==(const Config &other) const
    {
      return camera == other.camera && log == other.log && playback == other.playback &&
             render == other.render && connection == other.connection &&
             extentions == other.extentions;
    }

    [[nodiscard]] inline bool operator!=(const Config &other) const { return !operator==(other); }
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

  explicit Settings(const std::vector<settings::Extension> &extended_settings = {});

  Config config() const;
  void update(const Config &config);
  void update(const Camera &config);
  void update(const Log &config);
  void update(const Playback &config);
  void update(const Render &config);
  void update(const Connection &config);
  void update(const Extension &extension);

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
