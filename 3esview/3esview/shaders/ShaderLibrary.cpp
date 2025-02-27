//
// Author: Kazys Stepanas
//
#include "ShaderLibrary.h"

#include <3escore/Log.h>

#include <array>

namespace tes::view::shaders
{
namespace
{
const std::array<std::string, unsigned(ShaderLibrary::ID::Count)> &shaderNames()
{
  static const std::array<std::string, unsigned(ShaderLibrary::ID::Count)> names =  //
    {
      "Flat",          //
      "VertexColour",  //
      "Line",          //
      "PointCloud",    //
      "Voxel",         //
    };
  return names;
}
};  // namespace


ShaderLibrary::ShaderLibrary()
  : _core_shaders(unsigned(ID::Count))
{}


ShaderLibrary::~ShaderLibrary() = default;


std::string ShaderLibrary::shaderName(ID id)
{
  const auto idx = static_cast<unsigned>(id);
  const auto &names = shaderNames();
  return (idx < names.size()) ? names.at(idx) : std::string();
}


std::shared_ptr<Shader> ShaderLibrary::lookup(ID id) const
{
  const auto idx = static_cast<unsigned>(id);
  return (idx < _core_shaders.size()) ? _core_shaders.at(idx) : nullptr;
}


std::shared_ptr<Shader> ShaderLibrary::lookup(const std::string &name) const
{
  const auto search = _shaders.find(name);
  return (search != _shaders.end()) ? search->second : nullptr;
}


std::shared_ptr<Shader> ShaderLibrary::lookupForDrawType(DrawType draw_type) const
{
  switch (draw_type)
  {
  case DrawType::Points:
    return lookup(shaders::ShaderLibrary::ID::PointCloud);
  case DrawType::Lines:
    return lookup(shaders::ShaderLibrary::ID::Line);
  case DrawType::Triangles:
    return lookup(shaders::ShaderLibrary::ID::VertexColour);
  case DrawType::Voxels:
    return lookup(shaders::ShaderLibrary::ID::Voxel);
  default:
    log::error("Unsupported mesh draw type: ", int(draw_type));
    break;
  }
  return nullptr;
}


void ShaderLibrary::registerShader(ID id, const std::shared_ptr<Shader> &shader)
{
  const auto idx = static_cast<unsigned>(id);
  _core_shaders[idx] = shader;
  _shaders.emplace(shaderName(id), shader);
}


void ShaderLibrary::registerShader(const std::string &name, std::shared_ptr<Shader> shader)
{
  _shaders.emplace(name, std::move(shader));
}
}  // namespace tes::view::shaders
