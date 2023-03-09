//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_CATEGORIES_VIEW_H
#define TRD_EYE_SCENE_UI_CATEGORIES_VIEW_H

#include <3rdEyeScene/ClientConfig.h>

#include "TreeView.h"

#include <Magnum/GL/Texture.h>

#include <array>
#include <memory>

namespace tes::view
{
class DataThread;
namespace painter
{
class CategoryState;
struct CategoryInfo;
}  // namespace painter
}  // namespace tes::view

namespace tes::view::command
{
class Command;
}  // namespace tes::view::command

namespace tes::view::ui
{
class CategoriesView : public TreeView
{
public:
  CategoriesView(Viewer &viewer);

private:
  void drawContent(Magnum::ImGuiIntegration::Context &ui, Window &window) override;

  void drawNode(const painter::CategoryState &categories, const painter::CategoryInfo &cat_info);
  void drawBranch(const painter::CategoryState &categories, const painter::CategoryInfo &cat_info,
                  const std::vector<const painter::CategoryInfo *> &children);
  void drawLeaf(const painter::CategoryInfo &cat_info);
  void drawCheckbox(const painter::CategoryInfo &cat_info);

  void effectChanges();

  std::vector<std::pair<unsigned, bool>> _changes;
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_CATEGORIES_VIEW_H
