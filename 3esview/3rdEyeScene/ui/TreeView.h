//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UI_TABLE_VIEW_H
#define TRD_EYE_SCENE_UI_TABLE_VIEW_H

#include <3rdEyeScene/ClientConfig.h>

#include "Panel.h"

#include <3esview/util/Enum.h>

namespace tes::view::ui
{
/// A special view panel which supports drawing items in a tree view with child sub controls.
///
/// To start a new tree branch call @c beginBranch() and @c endSection() .
/// Use @c beginLeaf() and @c endLeaf() for each leaf.
class TES_VIEWER_API TreeView : public Panel
{
public:
  /// Flags for @c beginBranch()
  enum BranchFlag : unsigned
  {
    /// No flags set.
    None = 0u,
    /// Draw the branch label in the right hand column?
    DrawLabel = (1u << 0u),
    /// Force the open state for the branch.
    ForceOpen = (1u << 1u)
  };

  TreeView(const std::string &name, Viewer &viewer, const PreferredCoordinates &preferred = {});

protected:
  static bool beginBranch(unsigned idx, const std::string &label,
                          BranchFlag flags = BranchFlag::DrawLabel);
  static void endBranch(bool open);
  static void beginLeaf(unsigned idx, const std::string &label, const std::string &info = {});
  static void endLeaf();
};
}  // namespace tes::view::ui

TES_ENUM_FLAGS(tes::view::ui::TreeView::BranchFlag, unsigned);

#endif  // TRD_EYE_SCENE_UI_TABLE_VIEW_H
