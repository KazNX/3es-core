//
// Author: Kazys Stepanas
//
#include "SettingsView.h"

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>

#include <3esview/Viewer.h>

#include <Magnum/ImGuiIntegration/Context.hpp>

#include <array>

namespace tes::view::ui
{
SettingsView::SettingsView(Viewer &viewer)
  : TreeView("Settings", viewer)
{}


void SettingsView::drawContent(Magnum::ImGuiIntegration::Context &ui, Window &window)
{
  TES_UNUSED(ui);
  TES_UNUSED(window);
  if (!_editing_config)
  {
    _cached_config = _viewer.tes()->settings().config();
  }

  if (ImGui::BeginTable("SettingsSplit", 2,
                        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable))
  {
    unsigned idx = 0;

    ViewStatus status = {};
    status += show(idx++, _cached_config.camera);
    status += show(idx++, _cached_config.render);
    status += show(idx++, _cached_config.playback);
    status += show(idx++, _cached_config.log);

    for (auto &extension : _cached_config.extentions)
    {
      status += show(idx++, extension);
    }

    if (status.dirty)
    {
      // Mark config being edited.
      _editing_config = true;
    }
    else if (!status.active && _editing_config)
    {
      // Done editing.
      _editing_config = false;
      // Notify of changes.
      _viewer.tes()->settings().update(_cached_config);
    }

    ImGui::EndTable();
  }
}


SettingsView::ViewStatus SettingsView::show(unsigned idx, settings::Camera &config)
{
  const bool open = beginBranch(idx, "Camera");
  ViewStatus status = {};

  if (open)
  {
    unsigned idx = 0;
    status += showProperty(idx++, config.invert_y);
    status += showProperty(idx++, config.allow_remote_settings);
    status += showProperty(idx++, config.near_clip);
    status += showProperty(idx++, config.far_clip);
    status += showProperty(idx++, config.fov);
  }

  endBranch(open);

  return status;
}


SettingsView::ViewStatus SettingsView::show(unsigned idx, settings::Log &config)
{
  const bool open = beginBranch(idx, "Log");
  ViewStatus status = {};

  if (open)
  {
    unsigned idx = 0;
    status += showProperty(idx++, config.log_history);
  }

  endBranch(open);

  return status;
}


SettingsView::ViewStatus SettingsView::show(unsigned idx, settings::Playback &config)
{
  const bool open = beginBranch(idx, "Playback");
  ViewStatus status = {};

  if (open)
  {
    unsigned idx = 0;
    status += showProperty(idx++, config.allow_key_frames);
    status += showProperty(idx++, config.keyframe_every_mib);
    status += showProperty(idx++, config.keyframe_every_frames);
    status += showProperty(idx++, config.keyframe_min_separation);
    status += showProperty(idx++, config.keyframe_compression);
    status += showProperty(idx++, config.looping);
    status += showProperty(idx++, config.pause_on_error);
  }

  endBranch(open);

  return status;
}


SettingsView::ViewStatus SettingsView::show(unsigned idx, settings::Render &config)
{
  const bool open = beginBranch(idx, "Render");
  ViewStatus status = {};

  if (open)
  {
    unsigned idx = 0;
    status += showProperty(idx++, config.use_edl_shader);
    status += showProperty(idx++, config.edl_radius);
    status += showProperty(idx++, config.edl_exponential_scale);
    status += showProperty(idx++, config.edl_linear_scale);
    status += showProperty(idx++, config.point_size);
    status += showProperty(idx++, config.background_colour);
  }

  endBranch(open);

  return status;
}


SettingsView::ViewStatus SettingsView::show(unsigned idx, settings::Extension &config)
{
  const bool open = beginBranch(idx, config.name());
  ViewStatus status = {};

  if (open)
  {
    // Show all properties.
    unsigned idx = 0;
    for (auto &property : config)
    {
      // Brute force solution: try convert each property to each of the available types.
      // Show the one which works.
      if (auto *prop = property.getProperty<settings::Boolean>())
      {
        status += showProperty(idx++, *prop);
        continue;
      }
      if (auto *prop = property.getProperty<settings::Colour>())
      {
        status += showProperty(idx++, *prop);
        continue;
      }
      if (auto *prop = property.getProperty<settings::Enum>())
      {
        status += showProperty(idx++, *prop);
        continue;
      }
      if (auto *prop = property.getProperty<settings::Int>())
      {
        status += showProperty(idx++, *prop);
        continue;
      }
      if (auto *prop = property.getProperty<settings::UInt>())
      {
        status += showProperty(idx++, *prop);
        continue;
      }
      if (auto *prop = property.getProperty<settings::Float>())
      {
        status += showProperty(idx++, *prop);
        continue;
      }
      if (auto *prop = property.getProperty<settings::Double>())
      {
        status += showProperty(idx++, *prop);
        continue;
      }
      log::warn("Extension property ", property.label(), " has unknown type");
    }
  }

  endBranch(open);

  return status;
}


SettingsView::ViewStatus SettingsView::showProperty(unsigned idx, settings::Boolean &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  bool value = prop.value();
  ViewStatus status = {};
  status.dirty = ImGui::Checkbox(prop.label().c_str(), &value);
  if (status.dirty)
  {
    prop.setValue(value);
  }
  status.active = ImGui::IsItemActive();
  endLeaf();
  return status;
}


SettingsView::ViewStatus SettingsView::showProperty(unsigned idx, settings::Int &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  int value = prop.value();
  ViewStatus status = {};
  status.dirty = ImGui::InputInt(prop.label().c_str(), &value);
  if (status.dirty)
  {
    prop.setValue(value);
  }
  status.active = ImGui::IsItemActive();
  endLeaf();
  return status;
}


SettingsView::ViewStatus SettingsView::showProperty(unsigned idx, settings::UInt &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  unsigned value = prop.value();
  ViewStatus status = {};
  status.dirty = ImGui::InputInt(prop.label().c_str(), reinterpret_cast<int *>(&value));
  if (status.dirty)
  {
    prop.setValue(value);
  }
  status.active = ImGui::IsItemActive();
  endLeaf();
  return status;
}


SettingsView::ViewStatus SettingsView::showProperty(unsigned idx, settings::Float &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  float value = prop.value();
  ViewStatus status = {};
  status.dirty = ImGui::InputFloat(prop.label().c_str(), &value);
  if (status.dirty)
  {
    prop.setValue(value);
  }
  status.active = ImGui::IsItemActive();
  endLeaf();
  return status;
}


SettingsView::ViewStatus SettingsView::showProperty(unsigned idx, settings::Double &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  double value = prop.value();
  ViewStatus status = {};
  status.dirty = ImGui::InputDouble(prop.label().c_str(), &value);
  if (status.dirty)
  {
    prop.setValue(value);
  }
  status.active = ImGui::IsItemActive();
  endLeaf();
  return status;
}


SettingsView::ViewStatus SettingsView::showProperty(unsigned idx, settings::Colour &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  auto value = prop.value();
  std::array<float, 3> rgb_f = { prop.value().rf(), prop.value().gf(), prop.value().bf() };
  ViewStatus status = {};
  status.dirty = ImGui::ColorEdit3(prop.label().c_str(), rgb_f.data());
  if (status.dirty)
  {
    value.setRf(rgb_f[0]);
    value.setGf(rgb_f[1]);
    value.setBf(rgb_f[2]);
    prop.setValue(value);
  }
  status.active = ImGui::IsItemActive();
  endLeaf();
  return status;
}


SettingsView::ViewStatus SettingsView::showProperty(unsigned idx, settings::Enum &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  ViewStatus status = {};
  auto value = prop.value();
  const auto named_values = prop.namedValues();
  int value_index = 0;
  std::vector<const char *> names(named_values.size());
  for (size_t i = 0; i < named_values.size(); ++i)
  {
    names[i] = named_values[i].second.c_str();
    if (value == named_values[i].first)
    {
      value_index = static_cast<int>(i);
    }
  }
  if (ImGui::Combo(prop.label().c_str(), &value_index, names.data(),
                   static_cast<int>(names.size())))
  {
    prop.setValueByName(names[static_cast<unsigned>(value_index)]);
    status.dirty = true;
  }
  status.active = ImGui::IsItemActive();
  endLeaf();
  return status;
}
}  // namespace tes::view::ui
