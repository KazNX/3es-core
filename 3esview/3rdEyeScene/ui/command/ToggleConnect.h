#ifndef TRD_EYE_SCENE_UI_COMMAND_TOGGLE_CONNECT_H
#define TRD_EYE_SCENE_UI_COMMAND_TOGGLE_CONNECT_H

#include <3rdEyeScene/ClientConfig.h>

#include "ToggleView.h"

namespace tes::view::ui::command
{
class ToggleConnect : public ToggleView
{
public:
  ToggleConnect(ui::IconBar &icon_bar);
};
}  // namespace tes::view::ui::command

#endif  // TRD_EYE_SCENE_UI_COMMAND_TOGGLE_CONNECT_H
