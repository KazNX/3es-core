//
// Author: Kazys Stepanas
//
#include "CategoryState.h"

namespace tes::view::painter
{
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
