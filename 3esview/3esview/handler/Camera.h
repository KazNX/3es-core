//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include "Message.h"

#include <3esview/camera/Camera.h>

#include <3escore/Messages.h>

#include <array>
#include <limits>
#include <mutex>
#include <vector>

namespace tes::view::handler
{
class TES_VIEWER_API Camera : public Message
{
public:
  using CameraId = uint8_t;
  constexpr static CameraId kRecordedCameraID = tes::CameraMessage::kRecordedCameraID;

  Camera();

  /// Enumerate all valid camera ids into @p camera_ids.
  /// @param camera_ids Container to enumerate into. Cleared before adding items.
  /// @return The number of cameras enumerated.
  size_t enumerate(std::vector<CameraId> &camera_ids) const;

  [[nodiscard]] CameraId firstCameraId() const { return _first_valid; }
  bool lookup(CameraId camera_id, camera::Camera &camera) const;

  void initialise() override;
  void reset() override;
  void prepareFrame(const FrameStamp &stamp) override;
  void endFrame(const FrameStamp &stamp) override;
  void draw(DrawPass pass, const FrameStamp &stamp, const DrawParams &params,
            const painter::CategoryState &categories) override;
  void readMessage(PacketReader &reader) override;
  void serialise(Connection &out) override;

  /// Calculate the world basis vectors for the coordinate @p frame.
  /// @param frame The world coordinate frame.
  /// @param side The world side vector (optional).
  /// @param fwd The world forward vector (optional).
  /// @param up The world up vector (optional).
  static void getWorldAxes(tes::CoordinateFrame frame, Magnum::Vector3 *side, Magnum::Vector3 *fwd,
                           Magnum::Vector3 *up);

  /// Calculate the camera @p pitch and @p yaw given camera axes and world axes.
  ///
  /// The fwd/up axis pairs must be unit length and perpendicular.
  ///
  /// @param camera_fwd The camera forward axis.
  /// @param camera_up The camera up axis.
  /// @param world_fwd The world forward axis.
  /// @param world_up The world up axis.
  /// @param[out] pitch Output pitch value (radians).
  /// @param[out] yaw Output yaw value (radians).
  static void calculatePitchYaw(const Magnum::Vector3 &camera_fwd, const Magnum::Vector3 &camera_up,
                                const Magnum::Vector3 &world_fwd, const Magnum::Vector3 &world_up,
                                float &pitch, float &yaw);

  /// Calculate camera forward and up axes based on pitch and yaw values.
  /// @param pitch The camera pitch (radians).
  /// @param yaw The camera yaw (radians).
  /// @param world_fwd The world forward axis.
  /// @param world_up The world up axis.
  /// @param[out] camera_fwd Output forward axis.
  /// @param[out] camera_up Output up axis.
  static void calculateCameraAxes(float pitch, float yaw, const Magnum::Vector3 &world_fwd,
                                  const Magnum::Vector3 &world_up, Magnum::Vector3 &camera_fwd,
                                  Magnum::Vector3 &camera_up);

private:
  mutable std::mutex _mutex;
  /// Array of cameras. The boolean indicates the validity of the entry.
  using CameraEntry = std::pair<camera::Camera, bool>;
  using CameraSet = std::array<CameraEntry, std::numeric_limits<CameraId>::max() + 1>;
  /// Main thread camera state.
  CameraSet _cameras;
  /// Pending thread camera state for next @c prepareFrame().
  std::vector<std::pair<CameraId, camera::Camera>> _pending_cameras;
  CameraId _first_valid = 255u;
  ServerInfoMessage _server_info = {};
};
}  // namespace tes::view::handler
