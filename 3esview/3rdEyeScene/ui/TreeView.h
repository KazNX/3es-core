//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_TABLE_VIEW_H
#define TRD_EYE_SCENE_UI_TABLE_VIEW_H

#include <3rdEyeScene/ClientConfig.h>

#include "Panel.h"

namespace tes::view::ui
{
/// A special view panel which supports drawing items in a tree view with child sub controls.
///
/// To start a new tree branch call @c beginBranch() and @c endSection() .
/// Use @c beginLeaf() and @c endLeaf() for each leaf.
class TreeView : public Panel
{
public:
  TreeView(const std::string &name, Viewer &viewer, const PreferredCoordinates &preferred = {});

protected:
  static bool beginBranch(unsigned idx, const std::string &label, bool draw_label = true);
  static void endBranch(bool open);
  static void beginLeaf(unsigned idx, const std::string &label, const std::string &info = {});
  static void endLeaf();
};
}  // namespace tes::view::ui

#endif  // TRD_EYE_SCENE_UI_TABLE_VIEW_H
