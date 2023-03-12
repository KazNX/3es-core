//
// Author: Kazys Stepanas
//

#include "KeyframeStore.h"

#include <functional>

namespace tes::view::data
{
KeyframeStore::~KeyframeStore()
{
  clear();
}


void KeyframeStore::add(Keyframe keyframe)
{
  _keyframes.emplace_back(std::move(keyframe));
}


bool KeyframeStore::remove(FrameNumber keyframe_number)
{
  // TODO(KS): binary search.
  const auto [index, found] = exactKeyframeIndex(keyframe_number);
  if (!found)
  {
    return false;
  }
  _keyframes.erase(_keyframes.begin() + index);
  return true;
}


bool KeyframeStore::lookupNearest(FrameNumber target_frame, Keyframe &keyframe) const
{
  if (_keyframes.empty() || _keyframes[0].frame_number > target_frame)
  {
    return false;
  }

  // TODO(KS): binary search.
  const auto [candidate, found] = precedingKeyframeIndex(target_frame);
  if (found)
  {
    keyframe = _keyframes[candidate];
  }
  return found;
}


KeyframeStore::Keyframe KeyframeStore::last() const
{
  if (_keyframes.empty())
  {
    return {};
  }

  return _keyframes.back();
}


void KeyframeStore::clear()
{
  for (const auto &keyframe : _keyframes)
  {
    try
    {
      std::filesystem::remove(keyframe.snapshot_path);
    }
    catch (std::filesystem::filesystem_error &)
    {
      // Ignore errors deleting files
    }
  }
}


std::pair<size_t, bool> KeyframeStore::precedingKeyframeIndex(FrameNumber target_frame) const
{
  if (_keyframes.empty() || _keyframes[0].frame_number >= target_frame)
  {
    return { 0, false };
  }

  // Already checked index 0. We are looking for the next later keyframe
  size_t candidate = 0;
  for (; candidate + 1 < _keyframes.size(); ++candidate)
  {
    if (_keyframes[candidate + 1].frame_number >= target_frame)
    {
      break;
    }
  }

  return { candidate, true };
}


std::pair<size_t, bool> KeyframeStore::exactKeyframeIndex(FrameNumber target_frame) const
{
  for (size_t i = 0; i < _keyframes.size(); ++i)
  {
    if (_keyframes[i].frame_number == target_frame)
    {
      return { i - 1, true };
    }
  }

  return { 0, false };
}
}  // namespace tes::view::data
