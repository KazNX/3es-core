//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include <string>
#include <unordered_map>

namespace tes::view::painter
{
/// Represents a display category.
struct TES_VIEWER_API CategoryInfo
{
  /// Display name for the category.
  std::string name;
  /// Category ID. Zero is always the root category to which all other categories belong. It can
  /// be given an explicit name.
  uint16_t id = 0;
  /// Parent category, defaulting to the root ID.
  uint16_t parent_id = 0;
  /// Does this category default to the active state?
  bool default_active = false;
  /// Currently active?
  bool active = false;
  /// Should be expanded in the UI view?
  bool expanded = false;
};

/// Tracks and reflects the current state of the categories.
///
/// This reflects what should be visible.
class TES_VIEWER_API CategoryState
{
public:
  /// The category info map type.
  /// @todo There is going to be a lot of indexing this map. The MSC @c std::unordered_map
  /// performance is suspect here, so best to find a simple alternative.
  using CategoryMap = std::unordered_map<unsigned, CategoryInfo>;

  /// Check the active status for a category.
  /// @param category The category ID to check.
  /// @return True if the ID is valid and active.
  [[nodiscard]] bool isActive(unsigned category) const;
  /// Set the active status for a category.
  /// @param category The category ID to set.
  /// @param active True to activate.
  /// @return True if the @p category id is valid.
  bool setActive(unsigned category, bool active = true);

  /// Check the view expended status for a category.
  /// @param category The category ID to check.
  /// @return True if the ID is valid and should be expanded in the UI view.
  [[nodiscard]] bool isExpanded(unsigned category) const;
  /// Set the expanded status for a category.
  /// @param category The category ID to set.
  /// @param expanded True to expand.
  /// @return True if the @p category id is valid.
  bool setExpanded(unsigned category, bool expanded = true);

  /// Add/update category info. This registers @p info using @p info.id .
  /// @param info The category details.
  void addCategory(const CategoryInfo &info);
  /// Add or update a category. This ensures @p info is updated, but leaves the
  /// @c CategoryInfo::active state unchanged.
  /// @param info Category to add/update.
  void updateCategory(const CategoryInfo &info);

  /// Check if the given category ID is present.
  /// @param category The category ID.
  /// @return True if the @p category id is valid.
  [[nodiscard]] bool has(unsigned category) const;

  /// Retrieve information for a category.
  /// @param category The category ID.
  /// @param info The structure to update with info about @p category .
  /// @return True if the @p category id is valid.
  bool lookup(unsigned category, CategoryInfo &info) const;

  /// Read the full category map.
  /// @return
  [[nodiscard]] const CategoryMap &map() const { return _category_map; }

  /// Clear all category data.
  void clear() { _category_map.clear(); }

private:
  CategoryMap _category_map;
};
}  // namespace tes::view::painter
