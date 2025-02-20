//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include "Message.h"

#include <mutex>

namespace tes::view::handler
{
class TES_VIEWER_API Category : public Message
{
public:
  class TES_VIEWER_API CategoriesRef
  {
  public:
    CategoriesRef() = default;
    CategoriesRef(std::mutex &mutex, painter::CategoryState &state)
      : _mutex(&mutex)
      , _state(&state)
    {
      mutex.lock();
    }
    CategoriesRef(const CategoriesRef &other) = delete;
    CategoriesRef(CategoriesRef &&other) noexcept
      : _mutex(std::exchange(other._mutex, nullptr))
      , _state(std::exchange(other._state, nullptr))
    {}

    CategoriesRef &operator=(const CategoriesRef &other) = delete;
    CategoriesRef &operator=(CategoriesRef &&other) noexcept
    {
      _mutex = std::exchange(other._mutex, _mutex);
      _state = std::exchange(other._state, _state);
      return *this;
    }

    ~CategoriesRef() { release(); }

    void release()
    {
      if (_mutex)
      {
        _mutex->unlock();
      }
      _mutex = nullptr;
      _state = nullptr;
    }

    const painter::CategoryState &operator*() const { return *_state; }
    painter::CategoryState &operator*() { return *_state; }
    const painter::CategoryState *operator->() const { return _state; }
    painter::CategoryState *operator->() { return _state; }

  private:
    std::mutex *_mutex = nullptr;
    painter::CategoryState *_state = nullptr;
  };

  Category();

  /// Attain a @c CategoriesRef to the internal categories state.
  ///
  /// Note the returned object should be shortlived and must be released quickly. Copy the
  /// @c painter::CategoryState for longer running operations.
  ///
  /// @return A reference to the internal category state.
  CategoriesRef categories() { return { _mutex, _categories }; }

  void initialise() override;
  void reset() override;
  void prepareFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;
  void draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params,
            const painter::CategoryState &categories) override;
  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out) override;

private:
  /// Ensure the root category is present and has a name.
  void ensureRoot();

  mutable std::mutex _mutex;
  painter::CategoryState _categories;
  std::vector<painter::CategoryInfo> _pending;
};
}  // namespace tes::view::handler
