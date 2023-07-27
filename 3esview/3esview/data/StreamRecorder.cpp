//
// Author: Kazys Stepanas
//
#include "StreamRecorder.h"

#include <3esview/Constants.h>
#include <3esview/MagnumV3.h>
#include <3esview/camera/Camera.h>
#include <3esview/handler/Camera.h>

#include <3escore/CoreUtil.h>
#include <3escore/Log.h>
#include <3escore/PacketReader.h>

namespace tes::view::data
{
StreamRecorder::StreamRecorder(std::filesystem::path path, const ServerInfoMessage &server_info)
  : _connection(path.string(), serverSettings())
  , _server_info(server_info)
  , _path(std::move(path))
{
  _connection.sendServerInfo(server_info);
}


StreamRecorder::~StreamRecorder()
{
  close();
}


void StreamRecorder::recordPacket(const PacketReader &packet)
{
  _connection.send(reinterpret_cast<const uint8_t *>(&packet.packet()),
                   int_cast<int>(packet.packetSize()));
}


void StreamRecorder::recordCamera(const camera::Camera &camera)
{
  // Add a message for the camera state.
  // Size it large enough, plus some extra space by doubling the header size.
  std::array<uint8_t, 2 * sizeof(PacketHeader) + sizeof(CameraMessage) + sizeof(CrcType)> buffer{};
  PacketWriter writer(buffer.data(), buffer.size());

  const Magnum::Vector3 world_fwd = convert(worldForward<float>());
  const Magnum::Vector3 world_up = convert(worldUp<float>());
  Magnum::Vector3 cam_fwd = {};
  Magnum::Vector3 cam_up = {};

  // Resolve the camera axes.
  handler::Camera::calculateCameraAxes(camera.pitch, camera.yaw, world_fwd, world_up, cam_fwd,
                                       cam_up);

  writer.reset(MtCamera, 0);
  CameraMessage msg = {};
  msg.camera_id = CameraMessage::kRecordedCameraID;
  msg.flags = CFExplicitFrame;
  msg.coordinate_frame = static_cast<uint16_t>(kViewerCoordinateFrame);
  msg.x = camera.position.x();
  msg.y = camera.position.y();
  msg.z = camera.position.z();
  msg.dirX = cam_fwd.x();
  msg.dirY = cam_fwd.y();
  msg.dirZ = cam_fwd.z();
  msg.upX = cam_up.x();
  msg.upY = cam_up.y();
  msg.upZ = cam_up.z();
  msg.near = camera.clip_near;
  msg.far = camera.clip_far;
  msg.fov = camera.fov_horizontal_deg;

  if (msg.write(writer))
  {
    _connection.send(writer);
  }
}


void StreamRecorder::flush(float dt)
{
  _connection.updateFrame(dt, true);
}


void StreamRecorder::close()
{
  if (_connection.isConnected())
  {
    _connection.close();
  }
  _status = State::Closed;
}


ServerSettings StreamRecorder::serverSettings()
{
  // Most settings are irrelevant.
  ServerSettings settings = {};
  settings.compression_level = CompressionLevel::High;
  return settings;
}
}  // namespace tes::view::data
