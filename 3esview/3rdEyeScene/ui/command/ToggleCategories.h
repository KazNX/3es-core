#ifndef TRD_EYE_SCENE_UI_COMMAND_TOGGLE_CATEGORIES_H
#define TRD_EYE_SCENE_UI_COMMAND_TOGGLE_CATEGORIES_H

#include <3rdEyeScene/ClientConfig.h>

#include "ToggleView.h"

namespace tes::view::ui
{
class IconBar;
}

namespace tes::view::ui::command
{
class ToggleCategories : public ToggleView
{
public:
  ToggleCategories(ui::IconBar &icon_bar);
};
}  // namespace tes::view::ui::command

#endif  // SCENE_UI_COMMAND_TOGGLE_CATEGORIES_H
