//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_VIEWER_LOG_H
#define TES_VIEW_VIEWER_LOG_H

#include "3esview/ViewConfig.h"

#include <3escore/log.h>

#include <string>
#include <mutex>
#include <vector>

namespace tes::view
{
/// Logging handler for the viewer.
///
/// 3es logging is routed through an instance of this class for the viewer app. The viewer log holds
/// a record of each line output to the log and is used to retrieve the log for display.
class TES_VIEWER_API ViewerLog
{
public:
  /// The default max number of items to maintain in the history.
  static constexpr size_t kDefaultMaxLines = 100'000u;

  /// A log entry.
  struct Entry
  {
    log::Level level;
    std::string message;

    /// Check if this entry is relevant at the given log level.
    /// @param filter_level The log level of interest.
    /// @return True if relevant.
    inline bool isRelevant(log::Level filter_level) const
    {
      return static_cast<int>(level) <= static_cast<int>(filter_level);
    }
  };

  /// Represents an interable view into the log. The log remains mutex locked
  /// while the view is active.
  ///
  /// A @c View is exclusive as it mutex locks the @c ViewLog into which it provides a window.'
  ///
  /// A @c View supports forward (optionally filtered) iteration only.
  ///
  /// FIXME(KS): allow a @c View to have a @c log::Level restriction. Iteration is easy, but the
  /// @c size() must report the correct number of filtered items.
  class View
  {
  public:
    /// Iterator of a @c ViewerLog::View .
    class const_iterator
    {
    public:
      /// Default constructor.
      const_iterator() = default;
      /// Construct at the given position in the given log.
      /// @param log The log to iterate.
      /// @param cursor The position in @p log to view.
      /// @param begin True if the @p cursor referes to the first item of a non empty @p log.
      const_iterator(const ViewerLog *log, size_t cursor, bool begin);
      /// Modified copy constructor, which filters at the specified @p filter_level
      ///
      /// The current position of @p other is assumed to be relevant for the @p filter_level.
      /// @param other The iterator to copy.
      /// @param filter_level The filter level for the iterator.
      const_iterator(const const_iterator &other, log::Level filter_level);
      /// Copy constructor.
      /// @param other The iterator to copy.
      const_iterator(const const_iterator &other);
      /// Move constructor.
      /// @param other The iterator to move.
      const_iterator(const_iterator &&other);
      /// Destructor.
      ~const_iterator() = default;

      /// Copy assignment.
      /// @param other The iterator to copy.
      /// @return `*this`
      const_iterator &operator=(const const_iterator &);
      /// Move assignment.
      /// @param other The iterator to move.
      /// @return `*this`
      const_iterator &operator=(const_iterator &&other);

      /// Equality test. Filter level matching is not required for semantic equality.
      /// @param other The iterator to compare to.
      /// @return True when equal.
      bool operator==(const const_iterator &other) const;
      /// Inequality test : negation of equality comparison.
      /// @param other The iterator to compare to.
      /// @return True when not equal.
      bool operator!=(const const_iterator &other) const;

      //// Prefix increment ot the next item.
      const_iterator &operator++();
      //// Postfix increment ot the next item.
      const_iterator operator++(int);
      /// Increment by @p count.
      /// @param count The amount to increment by. Must remain in range of the valid (filtered) log
      /// items.
      const_iterator &operator+=(size_t count);

      /// Dereference operator.
      /// @return The current log @c Entry.
      const Entry &operator*() const { return _log->_lines[_cursor]; }
      /// Dereference operator.
      /// @return The current log @c Entry.
      const Entry *operator->() const { return &_log->_lines[_cursor]; }

      /// Check if the current entry is relevant at the specified @p filter_level.
      /// @param filter_level The filter level of interest.
      bool isRelevant(log::Level filter_level) const
      {
        return _log->_lines[_cursor].isRelevant(filter_level);
      }

    private:
      /// Bind the @c _next function to either the unfiltered or filtered  version.
      void bindNext()
      {
        if (_filter_level == log::Level::Trace)
        {
          _next = [this](size_t count) { next(count); };
        }
        else
        {
          _next = [this](size_t count) { nextFiltered(count); };
        }
      }

      /// Move to the next filtered item, stopping at @c end .
      /// @param count The number of items to move on by. Must be >= 1
      void nextFiltered(size_t count = 1);
      /// Move to the next unfiltered item, stopping at @c end .
      /// @param count The number of items to move on by. Must be >= 1
      void next(size_t count = 1);
      /// True if the @c end item has been reached.
      bool atEnd() const { return !_begin && _cursor == _log->endIndex(); }

      const ViewerLog *_log = nullptr;
      size_t _cursor = 0;
      log::Level _filter_level = log::Level::Trace;
      /// True if the cursor is referring to the first item.
      /// The _cursor can have the same value at begin/end once the ring buffer is full. This value
      /// disambiguates. It is true only for the first *valid* item.
      /// It must be set false when calling @c View::begin() on an empty view.
      bool _begin = false;
      /// Bound to either @c next() or @c nextFiltered() if filtering is used.
      std::function<void(size_t)> _next;
    };

    /// Construct a view for @p log with an optional filter level.
    ///
    /// This mutex locks the @p log.
    /// @param log The log to view.
    /// @param filter_level The filter level of interest.
    View(const ViewerLog &log, log::Level filter_level = log::Level::Trace)
      : _log(&log)
      , _filter_level(filter_level)
    {
      log._mutex.lock();
    }
    View(const View &) = delete;
    /// Move constructor.
    /// @param other The object to move.
    View(View &&other)
      : _log(std::exchange(other._log, nullptr))
    {}
    ~View() { release(); }

    View &operator=(const View &) = delete;
    /// Move assignment.
    /// @param other The object to move.
    /// @return `*this`
    View &operator=(View &&other)
    {
      _log = std::exchange(other._log, nullptr);
      return *this;
    }

    /// Is this a valid view - i.e., does it have a target log?
    /// @return True when valid.
    inline bool isValid() const { return _log != nullptr; }

    /// Get an iterator to the first item in the log.
    ///
    /// The view filter is installed if used, ensuring the first item is the first relevant item.
    /// @return An iterator to the first item. Matches @c end() when the log is empty or there are
    /// no relevant items at the current filter level.
    const_iterator begin() const
    {
      if (_filter_level == log::Level::Trace)
      {
        // Unfiltered.
        return const_iterator(_log, _log->beginIndex(), !_log->_lines.empty());
      }
      return beginFiltered(_filter_level);
    }

    /// Get an iterator to the end item.
    /// @return The end iterator.
    const_iterator end() const { return const_iterator(_log, _log->endIndex(), false); }

    /// Return the number of (relevant filtered) items in the log view.
    ///
    /// The filtered log size is calcualted on first call.
    ///
    /// @return The number of items in the view.
    size_t size() const { return (_filtered_size != kInvalidSize) ? kInvalidSize : calcSize(); }

    /// Release the view, unlocking the log mutex and invalidating the view.
    ///
    /// Does nothing if the view is already invalid.
    void release()
    {
      if (_log)
      {
        _log->_mutex.unlock();
        _log = nullptr;
      }
    }

  private:
    /// Find the @c begin() iterator for a filtered view.
    /// @param filter_level The filter level of interest.
    /// @return An iterator to the first relevant item at the specified @p filter_level.
    const_iterator beginFiltered(log::Level filter_level) const;
    /// Calculate the view size considering the @c _filter_level.
    size_t calcSize() const;

    static constexpr size_t kInvalidSize = ~static_cast<size_t>(0);
    const ViewerLog *_log = nullptr;
    mutable size_t _filtered_size = kInvalidSize;
    log::Level _filter_level = log::Level::Trace;
  };

  /// Construct a log with the specified history size.
  /// @param max_lines The maximum number of lines to track; equivalent to the history size.
  ViewerLog(size_t max_lines = kDefaultMaxLines);

  /// Add a message to the log.
  void log(log::Level level, const std::string &msg);

  /// Extract a subsection of the log into @p items.
  /// @param items Where to write the subsection.
  /// @param filter_level Only add items of this log level or more severe.
  /// @param cursor Cursor value used for progressive extraction. Start at zero, then it tracks the
  /// index of the next log item to try add.
  /// @param max_items The maximum number of items to retrieve.
  /// @return May be less than @c max_items . This will be zero and the cursor will not if there are
  /// no more items to extract.
  size_t extract(std::vector<Entry> &items, log::Level filter_level, size_t &cursor,
                 size_t max_items) const;

  /// Attain a view into the log. This locks the log from writing or further viewing.
  View view() const { return View(*this); }

  /// Attain a filtered log view showing only messages up to the given level.
  View view(log::Level filter_level) const { return View(*this, filter_level); }

  /// Change the lost history size to the given number of lines.
  void setMaxLines(size_t new_max_lines);
  /// Query the log history size as a maximum number of lines to store.
  size_t maxLines() const { return _max_lines; }

private:
  friend View;
  friend View::const_iterator;

  /// Get the index of the @c begin entry.
  size_t beginIndex() const { return (_count < _max_lines) ? 0 : _next_index; }

  /// Get the index of the @c end entry.
  size_t endIndex() const { return _next_index; }

  // Let's see how a naive implementation goes.
  /// Lines ring buffer.
  /// Is full once size reaches _max_lines.
  std::vector<Entry> _lines;
  /// Ring buffer index for the next item to write to.
  /// This wraps once the @c _count reaches _max_lines .
  size_t _next_index = 0;
  /// Number of items in the ring buffer.
  size_t _count = 0;
  /// Maximum number of lines allowed.
  size_t _max_lines = kDefaultMaxLines;
  mutable std::mutex _mutex;
};


inline ViewerLog::View::const_iterator::const_iterator(const ViewerLog *log, size_t cursor,
                                                       bool begin)
  : _log(log)
  , _cursor(cursor)
  , _begin(begin)
{
  bindNext();
}


inline ViewerLog::View::const_iterator::const_iterator(const const_iterator &other,
                                                       log::Level filter_level)
  : _log(other._log)
  , _cursor(other._cursor)
  , _filter_level(filter_level)
  , _begin(true)
{
  bindNext();
}


inline ViewerLog::View::const_iterator::const_iterator(const const_iterator &other)
  : _log(other._log)
  , _cursor(other._cursor)
  , _filter_level(other._filter_level)
  , _begin(other._begin)
{
  bindNext();
}


inline ViewerLog::View::const_iterator::const_iterator(const_iterator &&other)
  : _log(std::exchange(other._log, nullptr))
  , _cursor(std::exchange(other._cursor, 0))
  , _filter_level(std::exchange(other._filter_level, log::Level::Trace))
  , _begin(std::exchange(other._begin, false))
{
  bindNext();
}


inline ViewerLog::View::const_iterator &ViewerLog::View::const_iterator::operator=(
  const const_iterator &other)
{
  _log = other._log;
  _cursor = other._cursor;
  _filter_level = other._filter_level;
  _begin = other._begin;
  bindNext();
  return *this;
}


inline ViewerLog::View::const_iterator &ViewerLog::View::const_iterator::operator=(
  const_iterator &&other)
{
  std::swap(_log, other._log);
  std::swap(_cursor, other._cursor);
  std::swap(_filter_level, other._filter_level);
  std::swap(_begin, other._begin);
  bindNext();
  return *this;
}


inline bool ViewerLog::View::const_iterator::operator==(const const_iterator &other) const
{
  // Do not check the filter level here. We can address the same item regardless of filter level.
  return _log == other._log && _cursor == other._cursor && _begin == other._begin;
}


inline bool ViewerLog::View::const_iterator::operator!=(const const_iterator &other) const
{
  return !operator==(other);
}


inline ViewerLog::View::const_iterator &ViewerLog::View::const_iterator::operator++()
{
  _next(1);
  return *this;
}


inline ViewerLog::View::const_iterator ViewerLog::View::const_iterator::operator++(int)
{
  const_iterator old = *this;
  _next(1);
  return old;
}


inline ViewerLog::View::const_iterator &ViewerLog::View::const_iterator::operator+=(size_t count)
{
  _next(count);
  return *this;
}


inline void ViewerLog::View::const_iterator::nextFiltered(size_t count)
{
  size_t i = 0;
  bool is_relevant = false;
  do
  {
    next();
    is_relevant = isRelevant(_filter_level);
    // Increment i if the item is relevant. Otherwise we are skipping.
    // Using !!is_relevant to convert to 1 or 0.
    i += !!is_relevant;
  } while (!atEnd() && !is_relevant && i < count);
}


inline void ViewerLog::View::const_iterator::next(size_t count)
{
  _cursor = (_cursor + count) % _log->_max_lines;
  // Set _begin to false if count is not zero.
  // We do so by multiplication tro avoid branching, effectively:
  // _begin = _begin && !count
  _begin = _begin * !count;
}


inline ViewerLog::View::const_iterator operator+(const ViewerLog::View::const_iterator &iter,
                                                 size_t inc)
{
  auto updated = iter;
  updated += inc;
  return updated;
}
}  // namespace tes::view

#endif  // TES_VIEW_VIEWER_LOG_H
