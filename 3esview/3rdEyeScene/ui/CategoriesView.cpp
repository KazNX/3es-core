//
// Author: Kazys Stepanas
//
#include "CategoriesView.h"

#include <3esview/Viewer.h>
#include <3esview/handler/Category.h>

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

namespace tes::view::ui
{
CategoriesView::CategoriesView(Viewer &viewer)
  : TreeView("Categories", viewer)
{}


void CategoriesView::drawContent(Magnum::ImGuiIntegration::Context &ui, Window &window)
{
  TES_UNUSED(ui);
  if (!ImGui::BeginTable("CategoriesSplit", 2,
                         ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable))
  {
    ImGui::End();
    return;
  }

  // Get the categories.
  const auto categories = *viewer().tes()->categoryHandler().categories();
  _active_changes.clear();
  _view_changes.clear();

  // Start with the root node. Use default name if not present.
  painter::CategoryInfo root_info = {};
  if (!categories.lookup(0, root_info))
  {
    root_info.name = "root";
    root_info.active = true;
  }
  drawNode(categories, root_info);

  ImGui::EndTable();
  window.end();

  effectChanges();
}


void CategoriesView::drawNode(const painter::CategoryState &categories,
                              const painter::CategoryInfo &cat_info)
{
  // collect children first.
  std::vector<const painter::CategoryInfo *> children;

  for (const auto &[id, info] : categories.map())
  {
    if (info.parent_id == cat_info.id && info.parent_id != info.id)
    {
      children.emplace_back(&info);
    }
  }

  if (children.empty())
  {
    drawLeaf(cat_info);
    return;
  }

  drawBranch(categories, cat_info, children);
}


void CategoriesView::drawBranch(const painter::CategoryState &categories,
                                const painter::CategoryInfo &cat_info,
                                const std::vector<const painter::CategoryInfo *> &children)
{
  // Draw branch
  const auto branch_flags = (cat_info.expanded) ? BranchFlag::ForceOpen : BranchFlag::None;
  const bool open = beginBranch(cat_info.id, cat_info.name, branch_flags);

  drawCheckbox(cat_info);

  if (open)
  {
    for (const auto child : children)
    {
      drawNode(categories, *child);
    }
  }

  endBranch(open);

  if (open != cat_info.expanded)
  {
    _view_changes.emplace_back(cat_info.id, open);
  }
}


void CategoriesView::drawLeaf(const painter::CategoryInfo &cat_info)
{
  beginLeaf(cat_info.id, cat_info.name);
  drawCheckbox(cat_info);
  endLeaf();
}


void CategoriesView::drawCheckbox(const painter::CategoryInfo &cat_info)
{
  bool value = cat_info.active;
  const auto dirty = ImGui::Checkbox(cat_info.name.c_str(), &value);
  if (dirty)
  {
    _active_changes.emplace_back(static_cast<unsigned>(cat_info.id), value);
  }
}


void CategoriesView::effectChanges()
{
  auto categories = viewer().tes()->categoryHandler().categories();
  for (const auto &[id, activate] : _active_changes)
  {
    categories->setActive(id, activate);
  }
  _active_changes.clear();
  for (const auto &[id, expanded] : _view_changes)
  {
    categories->setExpanded(id, expanded);
  }
  _view_changes.clear();
}
}  // namespace tes::view::ui
