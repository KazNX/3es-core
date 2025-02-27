//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include <3escore/MeshMessages.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tes::view::shaders
{
class Shader;

/// Stores the common shaders used by the 3rd Eye Scene viewer.
class TES_VIEWER_API ShaderLibrary
{
public:
  /// IDs of known 3es shaders.
  /// @todo Work out if the EDL shader belongs here or if it's just mesh rendering.
  enum class ID
  {
    /// Flat shader with no vertex colour. Suitable for primitive rendering; supports instancing.
    Flat,
    /// Mesh shader with vertex colour support.
    VertexColour,
    /// Line shader.
    Line,
    /// Point cloud shader using hardware points or point geometry.
    PointCloud,
    /// Voxel geometry based shader.
    Voxel,
    /// Number of core sharers. Not to be used as an ID.
    Count
  };

  /// Get the shader name of a known shader.
  /// @param id
  /// @return
  static std::string shaderName(ID id);

  ShaderLibrary();
  ShaderLibrary(const ShaderLibrary &) = delete;
  ~ShaderLibrary();
  ShaderLibrary &operator=(const ShaderLibrary &) = delete;

  /// Lookup a shader by @c ID.
  /// @param id The shader @c ID to lookup
  /// @return The shader of the given name, or a nullptr on lookup failure.
  [[nodiscard]] std::shared_ptr<Shader> lookup(ID id) const;

  /// Lookup a shader by @c ID and try cast to a derived shader type.
  /// @tparam T The type to cast to, changing the return value to a @c std::shared_ptr of type @c T.
  /// @param id The shader @c ID to lookup
  /// @return The shader of the given name, or a nullptr on lookup failure or dynamic cast failure.
  template <typename T>
  [[nodiscard]] std::shared_ptr<T> lookup(ID id) const
  {
    return std::dynamic_pointer_cast<T>(lookup(id));
  }

  /// Lookup a shader by name.
  /// @param name The shader name to lookup
  /// @return The shader of the given name, or a nullptr on lookup failure.
  [[nodiscard]] std::shared_ptr<Shader> lookup(const std::string &name) const;

  /// Lookup a shader by name and try cast to a derived shader type.
  /// @tparam T The type to cast to, changing the return value to a @c std::shared_ptr of type @c T.
  /// @param name The shader name to lookup
  /// @return The shader of the given name, or a nullptr on lookup failure or dynamic cast failure.
  template <typename T>
  [[nodiscard]] std::shared_ptr<T> lookup(const std::string &name) const
  {
    return std::dynamic_pointer_cast<T>(lookup(name));
  }

  /// Lookup a shader by a primitive @c DrawType.
  ///
  /// This maps:
  /// - @c DrawType::Points `->` @c ID::PointCloud
  /// - @c DrawType::Lines `->` @c ID::Line
  /// - @c DrawType::Triangles `->` @c ID::VertexColour
  /// - @c DrawType::Voxels `->` @c ID::Voxel
  ///
  /// @param draw_type The 3escore mesh messages draw type.
  /// @return A shader for drawing the specified type or null if no shader is available for that
  /// type.
  [[nodiscard]] std::shared_ptr<Shader> lookupForDrawType(DrawType draw_type) const;

  /// @overload
  template <typename T>
  [[nodiscard]] std::shared_ptr<T> lookupForDrawTypeAs(DrawType draw_type) const
  {
    return std::dynamic_pointer_cast<T>(lookupForDrawType(draw_type));
  }

  /// Register a shader by known @c ID. This replaces any existing shader with that @c ID.
  ///
  /// @param id The shader @c ID.
  /// @param shader The shader to register.
  void registerShader(ID id, const std::shared_ptr<Shader> &shader);

  /// Register a shader by name. This replaces any existing shader of that name.
  ///
  /// > Note: use the overload for a shader with a known @c ID.
  ///
  /// @param name The shader name.
  /// @param shader The shader to register.
  void registerShader(const std::string &name, std::shared_ptr<Shader> shader);

private:
  /// Shaders by name.
  std::unordered_map<std::string, std::shared_ptr<Shader>> _shaders;
  /// Shaders by @c ID.
  std::vector<std::shared_ptr<Shader>> _core_shaders;
};
}  // namespace tes::view::shaders
