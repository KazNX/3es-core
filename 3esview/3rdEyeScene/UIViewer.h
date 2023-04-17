//
// Author: Kazys Stepanas
//
#ifndef TRD_EYE_SCENE_UIVIEWER_H
#define TRD_EYE_SCENE_UIVIEWER_H

#include <3esview/Viewer.h>

#include "ui/ImGui.h"

#include <Magnum/GL/Renderer.h>

#include <vector>

struct ImGuiContext;

namespace tes::view::ui
{
class Panel;
}  // namespace tes::view::ui

namespace tes::view
{
class TES_VIEWER_API UIViewer : public Viewer
{
public:
  class GuiContext
  {
  public:
    GuiContext(ImGuiContext *context);
    ~GuiContext();

    GuiContext(const GuiContext &other) = delete;
    GuiContext &operator=(const GuiContext &other) = delete;

    ImGuiContext &operator*() { return *_current; }
    const ImGuiContext &operator*() const { return *_current; }
    ImGuiContext *operator->() { return _current; }
    const ImGuiContext *operator->() const { return _current; }

  private:
    ImGuiContext *_current = nullptr;
    ImGuiContext *_restore = nullptr;
  };

  explicit UIViewer(const Arguments &arguments);
  ~UIViewer();

  [[nodiscard]] bool uiEnabled() const { return _ui_enabled; }
  void setUiEnabled(bool enable) { _ui_enabled = enable; }

protected:
  void drawEvent() override;
  DrawMode onDrawStart(float dt) override;
  void onDrawComplete(float dt) override;
  void viewportEvent(ViewportEvent &event) override;
  void mousePressEvent(MouseEvent &event) override;
  void mouseReleaseEvent(MouseEvent &event) override;
  void mouseMoveEvent(MouseMoveEvent &event) override;
  void mouseScrollEvent(MouseScrollEvent &event) override;
  void keyPressEvent(KeyEvent &event) override;
  void keyReleaseEvent(KeyEvent &event) override;
  void textInputEvent(TextInputEvent &event) override;

private:
  void initialiseUi();
  void initialiseImGui();
  void initialiseHud();
  void initialiseIconBarUi();
  void initialisePlaybackUi();
  void updateWindowSize(const settings::Settings::Config &config);

  Magnum::ImGuiIntegration::Context _imgui{ Magnum::NoCreate };
  std::vector<std::shared_ptr<ui::Panel>> _panels;
  /// Window size in sync with settings. Window may be resized on the next update if changed.
  /// This is a bit of an initialisation hack to make sure the virtual viewport functions are
  /// correctly called.
  Magnum::Vector2i _expected_window_size;
  bool _ui_enabled = true;
};
}  // namespace tes::view

#endif  // TRD_EYE_SCENE_UIVIEWER_H
