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
  auto config = _viewer.tes()->settings().config();
  if (ImGui::BeginTable("SettingsSplit", 2,
                        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable))
  {
    unsigned idx = 0;

    const bool camera_dirty = show(idx++, config.camera);
    const bool render_dirty = show(idx++, config.render);
    const bool playback_dirty = show(idx++, config.playback);
    // const bool log_dirty = show(idx++,config.log);
    bool extensions_dirty = false;

    for (auto &extension : config.extentions)
    {
      extensions_dirty = show(idx++, extension) || extensions_dirty;
    }

    // If more than two things are dirty, then update everything. Really should never happen as we
    // can only modify one property at a time.
    unsigned dirty_count = 0;
    dirty_count += !!camera_dirty;
    dirty_count += !!render_dirty;
    dirty_count += !!playback_dirty;
    // Force full refresh if any extensions are dirty.
    dirty_count += (extensions_dirty) ? 2 : 0;
    // dirty_count += !!log_dirty;
    if (dirty_count == 1)
    {
      if (camera_dirty)
      {
        _viewer.tes()->settings().update(config.camera);
      }
      else if (render_dirty)
      {
        _viewer.tes()->settings().update(config.render);
      }
      else if (playback_dirty)
      {
        _viewer.tes()->settings().update(config.playback);
      }
      // else if (log_dirty)
      // {
      //   _viewer.tes()->settings().update(config.log);
      // }
    }
    else if (dirty_count > 1)
    {
      _viewer.tes()->settings().update(config);
    }

    ImGui::EndTable();
  }
}


bool SettingsView::show(unsigned idx, settings::Camera &config)
{
  const bool open = beginBranch(idx, "Camera");
  bool dirty = false;

  if (open)
  {
    unsigned idx = 0;
    dirty = showProperty(idx++, config.invert_y) || dirty;
    dirty = showProperty(idx++, config.allow_remote_settings) || dirty;
    dirty = showProperty(idx++, config.near_clip) || dirty;
    dirty = showProperty(idx++, config.far_clip) || dirty;
    dirty = showProperty(idx++, config.fov) || dirty;
  }

  endBranch(open);

  return dirty;
}


bool SettingsView::show(unsigned idx, settings::Log &config)
{
  const bool open = beginBranch(idx, "Log");
  bool dirty = false;

  if (open)
  {
    unsigned idx = 0;
    dirty = showProperty(idx++, config.log_history) || dirty;
  }

  endBranch(open);

  return dirty;
}


bool SettingsView::show(unsigned idx, settings::Playback &config)
{
  const bool open = beginBranch(idx, "Playback");
  bool dirty = false;

  if (open)
  {
    unsigned idx = 0;
    dirty = showProperty(idx++, config.allow_key_frames) || dirty;
    dirty = showProperty(idx++, config.keyframe_every_mib) || dirty;
    dirty = showProperty(idx++, config.keyframe_every_frames) || dirty;
    dirty = showProperty(idx++, config.keyframe_min_separation) || dirty;
    dirty = showProperty(idx++, config.keyframe_compression) || dirty;
    dirty = showProperty(idx++, config.looping) || dirty;
    dirty = showProperty(idx++, config.pause_on_error) || dirty;
  }

  endBranch(open);

  return dirty;
}


bool SettingsView::show(unsigned idx, settings::Render &config)
{
  const bool open = beginBranch(idx, "Render");
  bool dirty = false;

  if (open)
  {
    unsigned idx = 0;
    dirty = showProperty(idx++, config.use_edl_shader) || dirty;
    dirty = showProperty(idx++, config.edl_radius) || dirty;
    dirty = showProperty(idx++, config.edl_exponential_scale) || dirty;
    dirty = showProperty(idx++, config.edl_linear_scale) || dirty;
    dirty = showProperty(idx++, config.point_size) || dirty;
    dirty = showProperty(idx++, config.background_colour) || dirty;
  }

  endBranch(open);

  return dirty;
}


bool SettingsView::show(unsigned idx, settings::Extension &config)
{
  const bool open = beginBranch(idx, config.name());
  bool dirty = false;

  if (open)
  {
    // Show all properties.
    unsigned idx = 0;
    for (auto &property : config)
    {
      // Brute force solution: try convert each property to each of the available types.
      // Show the one which works.
      if (auto *prop = property.getProperty<settings::Bool>())
      {
        dirty = showProperty(idx++, *prop) || dirty;
        continue;
      }
      if (auto *prop = property.getProperty<settings::Colour>())
      {
        dirty = showProperty(idx++, *prop) || dirty;
        continue;
      }
      if (auto *prop = property.getProperty<settings::Enum>())
      {
        dirty = showProperty(idx++, *prop) || dirty;
        continue;
      }
      if (auto *prop = property.getProperty<settings::Int>())
      {
        dirty = showProperty(idx++, *prop) || dirty;
        continue;
      }
      if (auto *prop = property.getProperty<settings::UInt>())
      {
        dirty = showProperty(idx++, *prop) || dirty;
        continue;
      }
      if (auto *prop = property.getProperty<settings::Float>())
      {
        dirty = showProperty(idx++, *prop) || dirty;
        continue;
      }
      if (auto *prop = property.getProperty<settings::Double>())
      {
        dirty = showProperty(idx++, *prop) || dirty;
        continue;
      }
      log::warn("Extension property ", property.label(), " has unknown type");
    }
  }

  endBranch(open);

  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::Bool &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  bool value = prop.value();
  const auto dirty = ImGui::Checkbox(prop.label().c_str(), &value);
  if (dirty)
  {
    prop.setValue(value);
  }
  endLeaf();
  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::Int &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  int value = prop.value();
  const bool dirty = ImGui::InputInt(prop.label().c_str(), &value);
  if (dirty)
  {
    prop.setValue(value);
  }
  endLeaf();
  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::UInt &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  unsigned value = prop.value();
  const bool dirty = ImGui::InputInt(prop.label().c_str(), reinterpret_cast<int *>(&value));
  if (dirty)
  {
    prop.setValue(value);
  }
  endLeaf();
  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::Float &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  float value = prop.value();
  const bool dirty = ImGui::InputFloat(prop.label().c_str(), &value);
  if (dirty)
  {
    prop.setValue(value);
  }
  endLeaf();
  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::Double &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  double value = prop.value();
  const bool dirty = ImGui::InputDouble(prop.label().c_str(), &value);
  if (dirty)
  {
    prop.setValue(value);
  }
  endLeaf();
  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::Colour &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  auto value = prop.value();
  std::array<float, 3> rgb_f = { prop.value().rf(), prop.value().gf(), prop.value().bf() };
  const bool dirty = ImGui::ColorEdit3(prop.label().c_str(), rgb_f.data());
  if (dirty)
  {
    value.setRf(rgb_f[0]);
    value.setGf(rgb_f[1]);
    value.setBf(rgb_f[2]);
    prop.setValue(value);
  }
  endLeaf();
  return dirty;
}


bool SettingsView::showProperty(unsigned idx, settings::Enum &prop)
{
  beginLeaf(idx, prop.label(), prop.tip());
  bool dirty = false;
  auto value = prop.value();
  const auto named_values = prop.namedValues();
  int value_index = 0;
  std::vector<const char *> names(named_values.size());
  for (int i = 0; i < static_cast<int>(named_values.size()); ++i)
  {
    names[i] = named_values[i].second.c_str();
    if (value == named_values[i].first)
    {
      value_index = i;
    }
  }
  if (ImGui::Combo(prop.label().c_str(), &value_index, names.data(),
                   static_cast<int>(names.size())))
  {
    prop.setValueByName(names[value_index]);
    dirty = true;
  }
  endLeaf();
  return dirty;
}
}  // namespace tes::view::ui
