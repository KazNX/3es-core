//
// Author: Kazys Stepanas
//
#include "CategoryState.h"

namespace tes::view::painter
{
bool CategoryState::isActive(unsigned category) const
{
  const auto search = _category_map.find(category);
  if (search == _category_map.end())
  {
    // Default to unknown being active.
    return true;
  }
  // return search->second.active;
  if (search->second.active)
  {
    // Possible active. Check parent.
    const auto parent_id = search->second.parent_id;
    if (parent_id == category)
    {
      return true;
    }
    return isActive(parent_id);
  }
  return false;
}


bool CategoryState::setActive(unsigned category, bool active)
{
  const auto search = _category_map.find(category);
  if (search == _category_map.end())
  {
    return false;
  }

  search->second.active = active;
  return true;
}


bool CategoryState::isExpanded(unsigned category) const
{
  const auto search = _category_map.find(category);
  if (search == _category_map.end())
  {
    // Default to unknown being collapsed.
    return false;
  }
  return search->second.expanded;
}


bool CategoryState::setExpanded(unsigned category, bool expanded)
{
  const auto search = _category_map.find(category);
  if (search == _category_map.end())
  {
    return false;
  }

  search->second.expanded = expanded;
  return true;
}


void CategoryState::addCategory(const CategoryInfo &info)
{
  _category_map[info.id] = info;
}


void CategoryState::updateCategory(const CategoryInfo &info)
{
  auto existing = _category_map.find(info.id);
  if (existing == _category_map.end())
  {
    _category_map[info.id] = info;
    return;
  }
  const bool current_active = existing->second.active;
  existing->second = info;
  existing->second.active = current_active;
}


bool CategoryState::has(unsigned category) const
{
  const auto search = _category_map.find(category);
  if (search != _category_map.end())
  {
    return true;
  }
  return false;
}


bool CategoryState::lookup(unsigned category, CategoryInfo &info) const
{
  const auto search = _category_map.find(category);
  if (search != _category_map.end())
  {
    info = search->second;
    return true;
  }
  return false;
}
}  // namespace tes::view::painter
