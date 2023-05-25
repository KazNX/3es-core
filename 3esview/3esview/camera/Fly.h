#ifndef TES_VIEW_CAMERA_FLY
#define TES_VIEW_CAMERA_FLY

#include <3esview/ViewConfig.h>

#include "Controller.h"

namespace tes::view::camera
{
class TES_VIEWER_API Fly : public Controller
{
public:
  static constexpr float kDefaultMoveSpeed = 8.0f;
  static constexpr float kDefaultTurnRateDegPerSec = 90.0f;
  static constexpr float kDefaultMouseSensitivityDegPerPixel = 2.0f;

  Fly();

  /// Get the movement speed for key translation updates: m/s.
  [[nodiscard]] float moveSpeed() const { return _move_speed; }
  /// Set the movement speed for key translation updates: m/s.
  void setMoveSpeed(float move_speed) { _move_speed = move_speed; }
  /// Get the rotation speed for key rotation updates: radians/s.
  [[nodiscard]] float rotationSpeed() const { return _rotation_speed; }
  /// Set the rotation speed for key rotation updates: radians/s.
  void setRotationSpeed(float rotation_speed) { _rotation_speed = rotation_speed; }
  /// Get the mouse sensitivity: radians/pixel.
  [[nodiscard]] float mouseSensitivity() const { return _mouse_sensitivity; }
  /// Set the mouse sensitivity: radians/pixel.
  void setMouseSensitivity(float mouse_sensitivity) { _mouse_sensitivity = mouse_sensitivity; }
  /// Get the movement key speed multiplier.
  [[nodiscard]] float moveMultiplier() const { return _move_multiplier; }
  /// Set the movement key speed multiplier.
  void setMoveMultiplier(float move_multiplier) { _move_multiplier = move_multiplier; }
  /// Get the rotation key speed multiplier.
  [[nodiscard]] float rotationMultiplier() const { return _rotation_multiplier; }
  /// Set the rotation key speed multiplier.
  void setRotationMultiplier(float rotation_multiplier)
  {
    _rotation_multiplier = rotation_multiplier;
  }
  /// Get the mouse sensitivity multiplier.
  [[nodiscard]] float mouseMultiplier() const { return _mouse_multiplier; }
  /// Set the mouse sensitivity multiplier.
  void setMouseMultiplier(float mouse_multiplier) { _mouse_multiplier = mouse_multiplier; }

  void updateMouse(float dx, float dy, Camera &camera) override;

  void updateKeys(float dt, Magnum::Vector3i translate, Magnum::Vector3i rotate,
                  Camera &camera) override;

private:
  /// Movement speed for key translation updates: m/s.
  float _move_speed = kDefaultMoveSpeed;
  /// Rotation speed for key rotation updates: radians/s.
  float _rotation_speed = float(Magnum::Rad(Magnum::Deg(kDefaultTurnRateDegPerSec)));
  /// Mouse sensitivity: radians/pixel.
  float _mouse_sensitivity = float(Magnum::Rad(Magnum::Deg(kDefaultMouseSensitivityDegPerPixel)));
  /// Current movement multiplier.
  float _move_multiplier = 1.0f;
  /// Current rotation multiplier.
  float _rotation_multiplier = 1.0f;
  /// Current mouse sensitivity multiplier.
  float _mouse_multiplier = 1.0f;
};
}  // namespace tes::view::camera

#endif  // TES_VIEW_CAMERA_FLY
