//
// Author: Kazys Stepanas
//
#include "ViewerLog.h"

#include <iostream>

namespace tes::view
{
ViewerLog::View::const_iterator ViewerLog::View::beginFiltered(log::Level filter_level) const
{
  // Filtered result. Check for empty
  if (_log->_lines.empty())
  {
    return end();
  }
  // Find the first relevant item.
  const_iterator iter(_log, _log->beginIndex(), !_log->_lines.empty());
  auto end = this->end();
  for (; iter != end && !iter.isRelevant(filter_level); ++iter)
  {}
  if (iter != end && iter.isRelevant(filter_level))
  {
    return { iter, filter_level };
  }
  return end;
}


size_t ViewerLog::View::calcSize() const
{
  _filtered_size = 0;
  if (!_log)
  {
    return 0;
  }

  if (_filter_level == log::Level::Trace)
  {
    // No filter
    _filtered_size = _log->_count;
  }
  else
  {
    for (size_t i = 0; i < _log->_count; ++i)
    {
      if (static_cast<int>(_log->_lines[i].level) <= static_cast<int>(_filter_level))
      {
        ++_filtered_size;
      }
    }
  }

  return _filtered_size;
}


ViewerLog::ViewerLog(size_t max_lines)
  : _lines(max_lines)
  , _max_lines(max_lines)
{}


void ViewerLog::log(log::Level level, const std::string &msg)
{
  // TODO(KS): assess the latency of this function.
  {
    const std::lock_guard guard(_mutex);
    if (_count == _max_lines)
    {
      // Full ring buffer. Remove the next item.
      // Replace the first item.
      _lines[_next_index] = Entry{ level, msg };
    }
    else
    {
      _lines[_next_index] = Entry{ level, msg };
      ++_count;
    }
    _next_index = (_next_index + 1) % _max_lines;
  }

  // Log to console outside of the mutex lock. cout and cerr are threadsafe, though the << operator
  // may interleave between threads.
  if (level <= consoleLogLevel())
  {
    if (level > log::Level::Error)
    {
      std::cout << msg << std::flush;
    }
    else
    {
      std::cerr << msg << std::flush;
    }
  }
}


size_t ViewerLog::extract(std::vector<Entry> &items, log::Level filter_level, size_t &cursor,
                          size_t max_items) const
{
  const auto view = this->view();
  if (view.size() <= cursor)
  {
    return 0;
  }

  auto iter = view.begin() + cursor;
  const auto end = view.end();
  size_t added = 0;
  for (; (max_items == 0 || added < max_items) && iter != end; ++iter, ++cursor)
  {
    if (static_cast<unsigned>(iter->level) <= static_cast<unsigned>(filter_level))
    {
      items.emplace_back(*iter);
      ++added;
    }
  }
  return added;
}


void ViewerLog::setMaxLines(size_t new_max_lines)
{
  const std::lock_guard guard(_mutex);
  if (new_max_lines == _max_lines)
  {
    // No change.
    return;
  }

  // Need to copy the entire contents.
  std::vector<Entry> new_history(new_max_lines);
  size_t write_index = 0;
  if (_max_lines < new_max_lines)
  {
    // Increasing log size.
    // Copy first part of the current log.
    for (size_t i = beginIndex(); i < _lines.size(); ++i)
    {
      new_history[write_index++] = _lines[i];
    }
    // Copy second part of the current log.
    for (size_t i = 0; i < beginIndex(); ++i)
    {
      new_history[write_index++] = _lines[i];
    }
  }
  else
  {
    // Reduced log size.
    if (_count <= new_max_lines)
    {
      // Have not wrapped the log yet and there's enough room in the new log. copy everything.
      for (size_t i = 0; i < _count; ++i)
      {
        new_history[write_index++] = _lines[i];
      }
    }
    else
    {
      // Need to drop data.
      // Calculate a start index such that it points to the first item to preserve.
      size_t read_index = (beginIndex() + (_count - new_max_lines)) % _max_lines;
      for (; write_index < new_history.size(); ++write_index)
      {
        new_history[write_index] = _lines[read_index];
        // Update the read index, wrapping for the current buffer size.
        read_index = (read_index + 1) % _lines.size();
      }
    }
  }

  // Finalise.
  // Swap buffers.
  std::swap(_lines, new_history);
  // Set new count.
  _count = write_index;
  _next_index = write_index % new_max_lines;
  // Finalise max lines.
  _max_lines = new_max_lines;
}
}  // namespace tes::view
