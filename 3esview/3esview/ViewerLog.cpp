//
// Author: Kazys Stepanas
//
#include "ViewerLog.h"

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
  const_iterator iter(_log, _log->_begin_index, !_log->_lines.empty());
  const auto end = this->end();
  for (; iter != end && !iter.isRelevant(filter_level); ++iter)
    ;
  if (iter != end && iter.isRelevant(filter_level))
  {
    return const_iterator(iter, filter_level);
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
  const std::lock_guard guard(_mutex);
  if (_count == _max_lines)
  {
    // Full ring buffer. Remove the next item.
    // Replace the first item.
    _lines[_begin_index] = Entry{ level, msg };
    _begin_index = (_begin_index + 1) % _count;
  }
  else
  {
    _lines[_begin_index + _count] = Entry{ level, msg };
    ++_count;
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
}  // namespace tes::view
