#include "ThirdEyeScene.h"

#include "EdlEffect.h"

#include "handler/Camera.h"
#include "handler/Category.h"
#include "handler/MeshResource.h"
#include "handler/MeshSet.h"
#include "handler/MeshShape.h"
#include "handler/Message.h"
#include "handler/Shape.h"
#include "handler/Text2D.h"
#include "handler/Text3D.h"

#include "painter/Arrow.h"
#include "painter/Box.h"
#include "painter/Capsule.h"
#include "painter/Cone.h"
#include "painter/Cylinder.h"
#include "painter/Plane.h"
#include "painter/Pose.h"
#include "painter/ShapePainter.h"
#include "painter/Sphere.h"
#include "painter/Star.h"
#include "painter/Text.h"

#include "settings/Loader.h"

#include "shaders/Flat.h"
#include "shaders/PointGeom.h"
#include "shaders/ShaderLibrary.h"
#include "shaders/VertexColour.h"
#include "shaders/VoxelGeom.h"

#include <3escore/FileConnection.h>
#include <3escore/Finally.h>
#include <3escore/Log.h>

#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>

#include <iterator>

// Things to learn about:
// - UI

// Things to implement:
// - point cloud message handler

namespace tes::view
{
ThirdEyeScene::ThirdEyeScene(const std::vector<settings::Extension> &extended_settings)
  : _main_thread_id(std::this_thread::get_id())
  , _settings(extended_settings)
{
  using namespace Magnum::Math::Literals;

  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::DepthTest);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::Blending);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::ProgramPointSize);
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::ProgramPointSize);
  Magnum::GL::Renderer::setPointSize(8);

  // Add extended settings before we continue.
  restoreSettings();

  _culler = std::make_shared<BoundsCuller>();
  // Initialise the font.
  initialiseFont();
  initialiseShaders();
  initialiseHandlers();

  const auto config = _settings.config();
  onCameraConfigChange(config);

  _settings.addObserver(
    settings::Settings::Category::Camera,
    [this](const settings::Settings::Config &config) { onCameraConfigChange(config); });
}


ThirdEyeScene::~ThirdEyeScene()
{
  try
  {
    // Try saving settings. Don't care about exceptions.
    storeSettings();
  }
  catch (...)
  {}
  // Need an ordered cleanup.
  _message_handlers.clear();
  _ordered_message_handlers.clear();
  _main_draw_handlers.clear();
  _secondary_draw_handlers.clear();
  _painters.clear();
  _text_painter = nullptr;
}


const std::unordered_map<uint32_t, std::string> &ThirdEyeScene::defaultHandlerNames()
{
  static const std::unordered_map<uint32_t, std::string> mappings = {
    { MtNull, "null" },
    { MtServerInfo, "server info" },
    { MtControl, "control" },
    { MtCollatedPacket, "collated packet" },
    { MtMesh, "mesh" },
    { MtCamera, "camera" },
    { MtCategory, "category" },
    { MtMaterial, "material" },
    { SIdSphere, "sphere" },
    { SIdBox, "box" },
    { SIdCone, "cone" },
    { SIdCylinder, "cylinder" },
    { SIdCapsule, "capsule" },
    { SIdPlane, "plane" },
    { SIdStar, "star" },
    { SIdArrow, "arrow" },
    { SIdMeshShape, "mesh shape" },
    { SIdMeshSet, "mesh set" },
    { SIdPointCloudDeprecated, "point cloud (deprecated)" },
    { SIdText3D, "text 3D" },
    { SIdText2D, "text 2D" },
    { SIdPose, "pose" },
  };
  return mappings;
}


void ThirdEyeScene::setActiveFboEffect(std::shared_ptr<FboEffect> effect)
{
  std::swap(_active_fbo_effect, effect);
}


void ThirdEyeScene::clearActiveFboEffect()
{
  _active_fbo_effect = nullptr;
}


void ThirdEyeScene::reset(std::function<bool()> abort)
{
  std::unique_lock lock(_render_mutex);
  bool aborted = false;
  if (std::this_thread::get_id() == _main_thread_id)
  {
    effectReset();
  }
  else
  {
    _reset = true;
    const auto stop_waiting = [target_reset = _reset_marker + 1, &aborted, &abort, this]()  //
    {
      aborted = aborted || abort();
      return aborted || _reset_marker >= target_reset;
    };
    // Use a wait_for loop on the condition variable to avoid a shutdown deadlock where the stream
    // thread can be waiting here for a reset, but also being joined from the main thread to quit.
    // The main thread can't service the reset request in this case (and doesn't need to).
    while (!stop_waiting())
    {
      _reset_notify.wait_for(lock, std::chrono::seconds(1), stop_waiting);
    }
  }
  if (!aborted && _reset_callback)
  {
    _reset_callback();
  }
}


void ThirdEyeScene::render(float dt, const Magnum::Vector2i &window_size, bool visible)
{
  //---------------------------------------------------------------------------
  // This section is protected by the _render_mutex
  // It must ensure that there can be no additional handler::Message::endFrame() calls in between
  // calling handler::Message::prepareFrame() and handler::Message::draw()
  // After the draw calls we release the mutex while finalising the rendering.
  const std::unique_lock guard(_render_mutex);
  if (_reset)
  {
    // Effect (do/execute) the pending reset.
    effectReset();
  }

  // Update frame if needed.
  if (_have_new_frame || _new_server_info)
  {
    // Update server info.
    if (_new_server_info)
    {
      for (auto &handler : _ordered_message_handlers)
      {
        handler->updateServerInfo(_server_info);
      }
      _new_server_info = false;
    }

    if (_have_new_frame)
    {
      _render_stamp.frame_number = _new_frame;
      _have_new_frame = false;

      for (auto &handler : _ordered_message_handlers)
      {
        handler->prepareFrame(_render_stamp);
      }
    }
  }

  // Handle any waiting snapshot.
  handlePendingSnapshot();

  if (!visible)
  {
    return;
  }

  const auto categories = *_category_handler->categories();
  const DrawParams params(_camera, window_size);
  ++_render_stamp.render_mark;

  _culler->cull(_render_stamp.render_mark, Magnum::Frustum::fromMatrix(params.pv_transform));

  if (_active_fbo_effect)
  {
    _active_fbo_effect->prepareFrame(params.pv_transform, FboEffect::ProjectionType::Perspective,
                                     _camera.clip_near, _camera.clip_far);
  }
  else
  {
    bindFinalFrameBuffer();
  }

  drawPrimary(dt, params, categories);
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // This section is not protected by the _render_mutex
  if (_active_fbo_effect)
  {
    bindFinalFrameBuffer();
    _active_fbo_effect->completeFrame();
  }
  //---------------------------------------------------------------------------
  drawSecondary(dt, params, categories);
  updateFps(dt);
}


void ThirdEyeScene::updateToFrame(FrameNumber frame, camera::Camera &camera)
{
  // Called from the data thread, not the main thread.
  // Must invoke endFrame() between prepareFrame() and draw() calls.
  const std::lock_guard guard(_render_mutex);
  if (frame != _render_stamp.frame_number)
  {
    for (auto &handler : _ordered_message_handlers)
    {
      handler->endFrame(_render_stamp);
    }
  }
  camera = _camera;
  _new_frame = frame;
  _have_new_frame = true;
}


void ThirdEyeScene::updateToFrame(FrameNumber frame)
{
  camera::Camera camera = {};
  updateToFrame(frame, camera);
}


void ThirdEyeScene::updateServerInfo(const ServerInfoMessage &server_info)
{
  const std::lock_guard guard(_render_mutex);
  _server_info = server_info;
  _new_server_info = true;
}


void ThirdEyeScene::processMessage(PacketReader &packet)
{
  auto handler = _message_handlers.find(packet.routingId());
  if (handler != _message_handlers.end())
  {
    handler->second->readMessage(packet);
  }
  else if (_unknown_handlers.find(packet.routingId()) == _unknown_handlers.end())
  {
    const auto known_ids = defaultHandlerNames();
    const auto search = known_ids.find(packet.routingId());
    if (search == known_ids.end())
    {
      log::error("No message handler for id ", packet.routingId());
    }
    else
    {
      log::error("No message handler for ", search->second);
    }
    _unknown_handlers.emplace(packet.routingId());
  }
}


void ThirdEyeScene::handlePendingSnapshot()
{
  std::unique_lock guard(_snapshot_wait.mutex);
  if (_snapshot_wait.waiting == SnapshotState::Waiting)
  {
    _snapshot_wait.frame_number = _render_stamp.frame_number;
    if (_snapshot_wait.connection && saveCurrentFrameSnapshot(*_snapshot_wait.connection))
    {
      _snapshot_wait.waiting = SnapshotState::Success;
    }
    else
    {
      _snapshot_wait.waiting = SnapshotState::Failed;
    }

    guard.unlock();
    _snapshot_wait.signal.notify_all();
  }
}


std::pair<bool, FrameNumber> ThirdEyeScene::saveSnapshot(const std::filesystem::path &path,
                                                         std::function<bool()> cancel_snapshot)
{
  // Note(KS): The server settings are irrelevant for a file connection. We can use teh default.
  // Arguably this implies the Connection constructor should be refactored.
  FileConnection out(path.string(), ServerSettings());
  const auto result = saveSnapshot(out, std::move(cancel_snapshot));
  out.close();
  return result;
}


std::pair<bool, FrameNumber> ThirdEyeScene::saveSnapshot(tes::Connection &connection,
                                                         std::function<bool()> cancel_snapshot)
{
  std::unique_lock guard(_snapshot_wait.mutex);

  if (std::this_thread::get_id() == _main_thread_id)
  {
    // Main thread. Save now.
    const bool ok = saveCurrentFrameSnapshot(connection);
    return { ok, _render_stamp.frame_number };
  }

  if (_snapshot_wait.connection)
  {
    // Already waiting on a snapshot
    return { false, 0 };
  }

  const auto at_exit = finally([this]() { _snapshot_wait.clear(); });
  _snapshot_wait.connection = &connection;

  // Block until signalled on the wait condition.
  _snapshot_wait.waiting = SnapshotState::Waiting;
  while (_snapshot_wait.waiting == SnapshotState::Waiting)
  {
    _snapshot_wait.signal.wait(guard, [this, &cancel_snapshot]() {
      return cancel_snapshot() || _snapshot_wait.waiting != SnapshotState::Waiting;
    });
  }

  if (cancel_snapshot())
  {
    return { false, 0 };
  }

  const bool ok = _snapshot_wait.waiting == SnapshotState::Success;
  _snapshot_wait.waiting = SnapshotState::None;
  return { ok, _snapshot_wait.frame_number };
}


void ThirdEyeScene::createSampleShapes()
{
  Magnum::Matrix4 shape_transform = {};

  // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
  // Axis box markers
  _painters[SIdBox]->add(Id(2), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 10, 0, 0 }) * shape_transform,
                         Magnum::Color4{ 1, 0, 0 });
  _painters[SIdBox]->add(Id(3), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, 10, 0 }) * shape_transform,
                         Magnum::Color4{ 0, 1, 0 });
  _painters[SIdBox]->add(Id(4), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, 0, 10 }) * shape_transform,
                         Magnum::Color4{ 0, 0, 1 });
  _painters[SIdBox]->add(Id(5), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ -10, 0, 0 }) * shape_transform,
                         Magnum::Color4{ 0, 1, 1 });
  _painters[SIdBox]->add(Id(6), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, -10, 0 }) * shape_transform,
                         Magnum::Color4{ 1, 0, 1 });
  _painters[SIdBox]->add(Id(7), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ 0, 0, -10 }) * shape_transform,
                         Magnum::Color4{ 1, 1, 0 });

  // Add debug shapes.
  float x = 0;
  shape_transform = {};
  _painters[SIdSphere]->add(Id(1), painter::ShapePainter::Type::Solid,
                            Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                            Magnum::Color4{ 1, 1, 0 });
  _painters[SIdSphere]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                            Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                            Magnum::Color4{ 0, 1, 1 });
  _painters[SIdSphere]->add(Id(1), painter::ShapePainter::Type::Transparent,
                            Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                            Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -2.5f;
  shape_transform = {};
  _painters[SIdBox]->add(Id(1), painter::ShapePainter::Type::Solid,
                         Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                         Magnum::Color4{ 1, 0, 0 });
  _painters[SIdBox]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                         Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                         Magnum::Color4{ 0, 1, 1 });
  _painters[SIdBox]->add(Id(1), painter::ShapePainter::Type::Transparent,
                         Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                         Magnum::Color4{ 1, 0, 1, 0.4f });

  x = 2.5f;
  shape_transform =
    Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 0.3f, 0.3f, 1.0f });
  _painters[SIdCylinder]->add(Id(1), painter::ShapePainter::Type::Solid,
                              Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                              Magnum::Color4{ 1, 1, 0 });
  _painters[SIdCylinder]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                              Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                              Magnum::Color4{ 0, 1, 1 });
  _painters[SIdCylinder]->add(Id(1), painter::ShapePainter::Type::Transparent,
                              Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                              Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -5.0f;
  shape_transform =
    Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 0.3f, 0.3f, 1.0f });
  _painters[SIdCapsule]->add(Id(1), painter::ShapePainter::Type::Solid,
                             Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                             Magnum::Color4{ 1, 1, 0 });
  _painters[SIdCapsule]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                             Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                             Magnum::Color4{ 0, 1, 1 });
  _painters[SIdCapsule]->add(Id(1), painter::ShapePainter::Type::Transparent,
                             Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                             Magnum::Color4{ 1, 0, 1, 0.4f });

  x = 7.5f;
  shape_transform =
    Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 1.0f, 1.0f, 1.0f });
  _painters[SIdPlane]->add(Id(1), painter::ShapePainter::Type::Solid,
                           Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                           Magnum::Color4{ 1, 1, 0 });
  _painters[SIdPlane]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                           Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                           Magnum::Color4{ 0, 1, 1 });
  _painters[SIdPlane]->add(Id(1), painter::ShapePainter::Type::Transparent,
                           Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                           Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -7.5f;
  shape_transform = Magnum::Matrix4::scaling({ 1.0f, 1.0f, 1.0f });
  _painters[SIdStar]->add(Id(1), painter::ShapePainter::Type::Solid,
                          Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                          Magnum::Color4{ 1, 1, 0 });
  _painters[SIdStar]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                          Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                          Magnum::Color4{ 0, 1, 1 });
  _painters[SIdStar]->add(Id(1), painter::ShapePainter::Type::Transparent,
                          Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                          Magnum::Color4{ 1, 0, 1, 0.4f });

  x = 10.0f;
  shape_transform =
    Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 0.1f, 0.1f, 1.0f });
  _painters[SIdArrow]->add(Id(1), painter::ShapePainter::Type::Solid,
                           Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                           Magnum::Color4{ 1, 1, 0 });
  _painters[SIdArrow]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                           Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                           Magnum::Color4{ 0, 1, 1 });
  _painters[SIdArrow]->add(Id(1), painter::ShapePainter::Type::Transparent,
                           Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                           Magnum::Color4{ 1, 0, 1, 0.4f });

  x = -10.0f;
  shape_transform =
    Magnum::Matrix4::rotationX(Magnum::Deg(35.0f)) * Magnum::Matrix4::scaling({ 1.0f, 1.0f, 1.0f });
  _painters[SIdPose]->add(Id(1), painter::ShapePainter::Type::Solid,
                          Magnum::Matrix4::translation({ x, 8, 0 }) * shape_transform,
                          Magnum::Color4{ 1, 1, 1 });
  _painters[SIdPose]->add(Id(1), painter::ShapePainter::Type::Wireframe,
                          Magnum::Matrix4::translation({ x, 5, 0 }) * shape_transform,
                          Magnum::Color4{ 1, 1, 1 });
  _painters[SIdPose]->add(Id(1), painter::ShapePainter::Type::Transparent,
                          Magnum::Matrix4::translation({ x, 2, 0 }) * shape_transform,
                          Magnum::Color4{ 1, 0, 1, 0.4f });
  // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

  for (auto &painter : _painters)
  {
    painter.second->commit();
  }
}


void ThirdEyeScene::effectReset()
{
  // _render_mutex must be locked before calling.
  for (auto &handler : _ordered_message_handlers)
  {
    handler->reset();
  }
  _unknown_handlers.clear();
  ++_reset_marker;
  _reset = false;
  // Slight inefficiency as we notify while the mutex is still locked.
  _reset_notify.notify_all();
}


void ThirdEyeScene::initialiseFont()
{
  // TODO(KS): get resources strings passed in as it's the exe which must include the resources.
  _text_painter = std::make_shared<painter::Text>(_font_manager);
}


void ThirdEyeScene::initialiseHandlers()
{
  _painters.emplace(SIdSphere, std::make_shared<painter::Sphere>(_culler, _shader_library));
  _painters.emplace(SIdBox, std::make_shared<painter::Box>(_culler, _shader_library));
  _painters.emplace(SIdCone, std::make_shared<painter::Cone>(_culler, _shader_library));
  _painters.emplace(SIdCylinder, std::make_shared<painter::Cylinder>(_culler, _shader_library));
  _painters.emplace(SIdCapsule, std::make_shared<painter::Capsule>(_culler, _shader_library));
  _painters.emplace(SIdPlane, std::make_shared<painter::Plane>(_culler, _shader_library));
  _painters.emplace(SIdStar, std::make_shared<painter::Star>(_culler, _shader_library));
  _painters.emplace(SIdArrow, std::make_shared<painter::Arrow>(_culler, _shader_library));
  _painters.emplace(SIdPose, std::make_shared<painter::Pose>(_culler, _shader_library));

  const auto category_handler = std::make_shared<handler::Category>();
  _ordered_message_handlers.emplace_back(category_handler);
  _category_handler = category_handler.get();

  const auto camera_handler = std::make_shared<handler::Camera>();
  _ordered_message_handlers.emplace_back(camera_handler);
  _camera_handler = camera_handler.get();

  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdSphere, "sphere", _painters[SIdSphere]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdBox, "box", _painters[SIdBox]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdCone, "cone", _painters[SIdCone]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdCylinder, "cylinder", _painters[SIdCylinder]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdCapsule, "capsule", _painters[SIdCapsule]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdPlane, "plane", _painters[SIdPlane]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdStar, "star", _painters[SIdStar]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdArrow, "arrow", _painters[SIdArrow]));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::Shape>(SIdPose, "pose", _painters[SIdPose]));

  auto mesh_resources = std::make_shared<handler::MeshResource>(_shader_library);
  _ordered_message_handlers.emplace_back(mesh_resources);
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::MeshShape>(_culler, _shader_library));
  _ordered_message_handlers.emplace_back(
    std::make_shared<handler::MeshSet>(_culler, mesh_resources));

  // Copy main draw handlers
  _main_draw_handlers = _ordered_message_handlers;

  // Add secondary draw handlers
  _secondary_draw_handlers.emplace_back(std::make_shared<handler::Text2D>(_text_painter));
  _secondary_draw_handlers.emplace_back(std::make_shared<handler::Text3D>(_text_painter));

  std::copy(_secondary_draw_handlers.begin(), _secondary_draw_handlers.end(),
            std::back_inserter(_ordered_message_handlers));

  // TODO:
  // - point cloud
  // - multi-shape

  // Copy message handlers to the routing set and initialise.
  for (auto &handler : _ordered_message_handlers)
  {
    handler->initialise();
    _message_handlers.emplace(handler->routingId(), handler);
  }
}


void ThirdEyeScene::initialiseShaders()
{
  _shader_library = std::make_shared<shaders::ShaderLibrary>();
  _shader_library->registerShader(shaders::ShaderLibrary::ID::Flat,
                                  std::make_shared<shaders::Flat>());
  auto vertex_colour_shader = std::make_shared<shaders::VertexColour>();
  _shader_library->registerShader(shaders::ShaderLibrary::ID::VertexColour, vertex_colour_shader);
  _shader_library->registerShader(shaders::ShaderLibrary::ID::Line, vertex_colour_shader);
  _shader_library->registerShader(shaders::ShaderLibrary::ID::PointCloud,
                                  std::make_shared<shaders::PointGeom>());
  _shader_library->registerShader(shaders::ShaderLibrary::ID::Voxel,
                                  std::make_shared<shaders::VoxelGeom>());
}


// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void ThirdEyeScene::bindFinalFrameBuffer()
{
  Magnum::GL::defaultFramebuffer
    .clear(Magnum::GL::FramebufferClear::Color | Magnum::GL::FramebufferClear::Depth)
    .bind();
}


void ThirdEyeScene::drawPrimary(float dt, const DrawParams &params,
                                const painter::CategoryState &categories)
{
  draw(dt, params, categories, _main_draw_handlers);
}


void ThirdEyeScene::drawSecondary(float dt, const DrawParams &params,
                                  const painter::CategoryState &categories)
{
  draw(dt, params, categories, _secondary_draw_handlers);
}


void ThirdEyeScene::draw(float dt, const DrawParams &params,
                         const painter::CategoryState &categories,
                         const std::vector<std::shared_ptr<handler::Message>> &drawers)
{
  (void)dt;
  // Draw opaque then transparent for proper blending.
  for (const auto &handler : drawers)
  {
    handler->draw(handler::Message::DrawPass::Opaque, _render_stamp, params, categories);
  }
  Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::Blending);
  for (const auto &handler : drawers)
  {
    handler->draw(handler::Message::DrawPass::Transparent, _render_stamp, params, categories);
  }
  Magnum::GL::Renderer::disable(Magnum::GL::Renderer::Feature::Blending);
  for (const auto &handler : drawers)
  {
    handler->draw(handler::Message::DrawPass::Overlay, _render_stamp, params, categories);
  }
}


void ThirdEyeScene::updateFps(float dt)
{
  // Update stats.
  _fps.push(dt);
}


void ThirdEyeScene::onCameraConfigChange(const settings::Settings::Config &config)
{
  _camera.clip_far = config.camera.far_clip.value();
  _camera.clip_near = config.camera.near_clip.value();
  _camera.fov_horizontal_deg = config.camera.fov.value();
}


bool ThirdEyeScene::saveCurrentFrameSnapshot(tes::Connection &connection)
{
  // Write the server info message. We don't need a frame count.
  connection.sendServerInfo(_server_info);
  for (const auto &handler : _ordered_message_handlers)
  {
    handler->serialise(connection);
  }
  connection.updateTransfers(0);
  connection.updateFrame(0.0f, true);
  return true;
}


void ThirdEyeScene::restoreSettings()
{
  // Start by copying what we have. We need to do this to get all the extension settings.
  auto config = _settings.config();
  const auto io_result = settings::load(config);
  if (io_result.code != settings::IOCode::Error)
  {
    if (!io_result.message.empty())
    {
      tes::log::info(io_result.message);
    }
    _settings.update(config);
  }
  else if (!io_result.message.empty())
  {
    tes::log::warn(io_result.message);
  }
}


void ThirdEyeScene::storeSettings()
{
  const auto config = _settings.config();
  const auto io_result = settings::save(config);
  if (!io_result.message.empty())
  {
    if (io_result.code == settings::IOCode::Ok)
    {
      tes::log::info(io_result.message);
    }
    else
    {
      tes::log::warn(io_result.message);
    }
  }
}
}  // namespace tes::view
