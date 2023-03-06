//
// Author: Kazys Stepanas
//
#include "TreeView.h"

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <3esview/Viewer.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

#include <array>

namespace tes::view::ui
{
TreeView::TreeView(Viewer &viewer)
  : Panel(viewer)
{}


bool TreeView::beginBranch(unsigned idx, const std::string &label, bool draw_label)
{
  // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
  ImGui::PushID(idx);
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::AlignTextToFramePadding();
  const bool node_open = ImGui::TreeNode(label.c_str());
  ImGui::TableSetColumnIndex(1);
  if (draw_label)
  {
    ImGui::Text(label.c_str());
  }

  return node_open;
}


void TreeView::endBranch(bool open)
{
  if (open)
  {
    ImGui::TreePop();
  }
  ImGui::PopID();
}


void TreeView::beginLeaf(unsigned idx, const std::string &label, const std::string &info)
{
  TES_UNUSED(info);
  ImGui::PushID(idx);  // Use field index as identifier.
  ImGui::TableNextRow();
  ImGui::TableSetColumnIndex(0);
  ImGui::AlignTextToFramePadding();
  const ImGuiTreeNodeFlags flags =
    ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
  // We must give the tree node a name unique from the label as the label will be used for the child
  // UI node.
  const std::string tree_node_name = label + "_leaf";
  ImGui::TreeNodeEx(tree_node_name.c_str(), flags);
  if (!info.empty() && ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::SetTooltip(info.c_str());
    ImGui::EndTooltip();
  }

  ImGui::TableSetColumnIndex(1);
  ImGui::SetNextItemWidth(-FLT_MIN);
}


void TreeView::endLeaf()
{
  ImGui::NextColumn();
  ImGui::PopID();
}
}  // namespace tes::view::ui
