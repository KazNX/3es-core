//
// Author: Kazys Stepanas
//
#include "Loader.h"

#include <3esview/3p/cfgpath.h>

#include <3escore/Colour.h>

#include <ryml/ryml.hpp>
#include <ryml/ryml_std.hpp>

#include <c4/format.hpp>

#include <array>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace tes::view::settings
{
namespace
{
/// Tag set used for yaml keys.
///
/// Note: most return @c ryml::csubstr which borrows memory from a static string representation.
///
/// The @c Common::trueSet() and @c Common::falseSet() arrays contain @c std::string because of
/// different usage.
struct Tags
{
  struct Root
  {
    static ryml::csubstr camera()
    {
      static const std::string str = "camera";
      return ryml::csubstr(str.c_str(), str.size());
    }
    static ryml::csubstr log()
    {
      static const std::string str = "log";
      return ryml::csubstr(str.c_str(), str.size());
    }
    static ryml::csubstr playback()
    {
      static const std::string str = "playback";
      return ryml::csubstr(str.c_str(), str.size());
    }
    static ryml::csubstr render()
    {
      static const std::string str = "render";
      return ryml::csubstr(str.c_str(), str.size());
    }
    static ryml::csubstr connection()
    {
      static const std::string str = "connection";
      return ryml::csubstr(str.c_str(), str.size());
    }
  };

  struct Connection
  {
    static ryml::csubstr history()
    {
      static const std::string str = "history";
      return ryml::csubstr(str.c_str(), str.size());
    }
    static ryml::csubstr host()
    {
      static const std::string str = "host";
      return ryml::csubstr(str.c_str(), str.size());
    }
    static ryml::csubstr port()
    {
      static const std::string str = "port";
      return ryml::csubstr(str.c_str(), str.size());
    }
  };

  struct Common
  {
    static const std::array<const std::string, 4> &trueSet()
    {
      static const std::array<const std::string, 4> values = { "1", "on", "yes", "true" };
      return values;
    }
    static const std::array<const std::string, 4> &falseSet()
    {
      static const std::array<const std::string, 4> values = { "0", "off", "no", "false" };
      return values;
    }
    static ryml::csubstr trueStr()
    {
      static const std::string str = "true";
      return ryml::csubstr(str.c_str(), str.size());
    }
    static ryml::csubstr falseStr()
    {
      static const std::string str = "false";
      return ryml::csubstr(str.c_str(), str.size());
    }
    static ryml::csubstr red()
    {
      static const std::string str = "red";
      return ryml::csubstr(str.c_str(), str.size());
    }
    static ryml::csubstr green()
    {
      static const std::string str = "green";
      return ryml::csubstr(str.c_str(), str.size());
    }
    static ryml::csubstr blue()
    {
      static const std::string str = "blue";
      return ryml::csubstr(str.c_str(), str.size());
    }
  };
};


bool parse(const ryml::NodeRef &parent, const std::string &key, std::string &value)
{
  if (parent.empty())
  {
    return true;
  }

  const auto key2 = ryml::csubstr(key.c_str(), key.size());
  const auto node = parent[key2];
  if (node.empty())
  {
    return false;
  }

  value = std::string(node.val().data(), node.val().size());
  return true;
}


bool parse(const ryml::NodeRef &parent, Bool &value)
{
  if (parent.empty())
  {
    return true;
  }

  const auto key = ryml::csubstr(value.label().c_str(), value.label().size());
  const auto node = parent[key];
  if (node.empty())
  {
    return false;
  }

  std::string str(node.val().data(), node.val().size());
  std::transform(str.begin(), str.end(), str.begin(), [](char ch) { return ::tolower(ch); });
  if (std::find(Tags::Common::trueSet().begin(), Tags::Common::trueSet().end(), str) !=
      Tags::Common::trueSet().end())
  {
    value.setValue(true);
    return true;
  }
  if (std::find(Tags::Common::falseSet().begin(), Tags::Common::falseSet().end(), str) !=
      Tags::Common::falseSet().end())
  {
    value.setValue(false);
    return true;
  }
  return false;
}


bool parse(const ryml::NodeRef &parent, Colour &value)
{
  if (parent.empty())
  {
    return true;
  }

  const auto key = ryml::csubstr(value.label().c_str(), value.label().size());
  const auto node = parent[key];
  if (node.empty() || !node.type().is_map())
  {
    return false;
  }

  std::array<ryml::NodeRef, 3> nodes = {
    node[Tags::Common::red()],
    node[Tags::Common::green()],
    node[Tags::Common::blue()],
  };
  std::array<tes::Colour::Channel, 3> channels = { tes::Colour::Channel::R, tes::Colour::Channel::G,
                                                   tes::Colour::Channel::B };

  auto colour = value.value();
  for (size_t i = 0; i < nodes.size(); ++i)
  {
    if (nodes[i].empty())
    {
      return false;
    }
    const std::string str(nodes[i].val().data(), nodes[i].val().size());
    std::istringstream in(str);
    int channel = {};
    in >> channel;
    if (in.bad())
    {
      return false;
    }

    colour.channel(channels[i]) = static_cast<uint8_t>(channel);
  }

  value.setValue(colour);

  return true;
}  // namespace


template <typename T>
bool parse(const ryml::NodeRef &parent, Numeric<T> &value)
{
  if (parent.empty())
  {
    return true;
  }

  const auto key = ryml::csubstr(value.label().c_str(), value.label().size());
  const auto node = parent[key];
  if (node.empty())
  {
    return false;
  }

  const std::string str(node.val().data(), node.val().size());
  std::istringstream in(str);
  auto temp = value.value();

  in >> temp;
  if (!in.bad())
  {
    value.setValue(temp);
    return true;
  }

  return false;
}


template <typename E>
bool parse(const ryml::NodeRef &parent, Enum<E> &value)
{
  if (parent.empty())
  {
    return true;
  }

  const auto key = ryml::csubstr(value.label().c_str(), value.label().size());
  const auto node = parent[key];
  if (node.empty())
  {
    return false;
  }

  // use case insensitive compare for enum
  std::string str(node.val().data(), node.val().size());
  std::transform(str.begin(), str.end(), str.begin(), [](char ch) { return ::tolower(ch); });

  const auto named_values = value.namedValues();
  for (size_t i = 0; i < named_values.size(); ++i)
  {
    auto name = named_values[i].second;
    std::transform(name.begin(), name.end(), name.begin(), [](char ch) { return ::tolower(ch); });
    if (str == name)
    {
      value.setValue(named_values[i].first);
      return true;
    }
  }
  return false;
}


template <typename T>
bool write(ryml::NodeRef &parent, const T &prop)
{
  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  parent[key] << prop.value();
  return true;
}


bool write(ryml::NodeRef &parent, const Bool &prop)
{
  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  parent[key] << (prop.value() ? Tags::Common::trueStr() : Tags::Common::falseStr());
  return true;
}


bool write(ryml::NodeRef &parent, const Colour &prop)
{
  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  auto node = parent[ryml::csubstr(prop.label().c_str(), prop.label().size())];
  node |= ryml::MAP;

  node[Tags::Common::red()] << static_cast<int>(prop.value().red());
  node[Tags::Common::green()] << static_cast<int>(prop.value().green());
  node[Tags::Common::blue()] << static_cast<int>(prop.value().blue());

  return true;
}

template <typename T>
bool parse(const ryml::NodeRef &parent, T &prop)
{
  return parse(parent[prop.label()], prop);
}


bool load(const ryml::NodeRef &node, Camera &camera)
{
  bool ok = true;
  ok = parse(node, camera.invert_y) && ok;
  ok = parse(node, camera.allow_remote_settings) && ok;
  ok = parse(node, camera.near_clip) && ok;
  ok = parse(node, camera.far_clip) && ok;
  ok = parse(node, camera.fov) && ok;

  return ok;
}


bool save(ryml::NodeRef node, const Camera &camera)
{
  bool ok = true;
  node |= ryml::MAP;
  ok = write(node, camera.invert_y) && ok;
  ok = write(node, camera.allow_remote_settings) && ok;
  ok = write(node, camera.near_clip) && ok;
  ok = write(node, camera.far_clip) && ok;
  ok = write(node, camera.fov) && ok;

  return ok;
}


bool load(const ryml::NodeRef &node, Log &log)
{
  bool ok = true;
  ok = parse(node, log.log_window_size) && ok;
  return ok;
}


bool save(ryml::NodeRef &node, const Log &log)
{
  bool ok = true;
  node |= ryml::MAP;
  ok = write(node, log.log_window_size) && ok;
  return ok;
}


bool load(const ryml::NodeRef &node, Playback &playback)
{
  bool ok = true;
  ok = parse(node, playback.allow_key_frames) && ok;
  ok = parse(node, playback.keyframe_every_mib) && ok;
  ok = parse(node, playback.keyframe_every_frames) && ok;
  ok = parse(node, playback.keyframe_skip_forward_frames) && ok;
  ok = parse(node, playback.keyframe_compression) && ok;
  ok = parse(node, playback.looping) && ok;
  ok = parse(node, playback.pause_on_error) && ok;
  return ok;
}


bool load(const ryml::NodeRef &node, Connection &connection)
{
  bool ok = true;

  connection.history.clear();
  if (!node.empty())
  {
    if (node.has_child(Tags::Connection::history()))
    {
      const auto history_node = node[Tags::Connection::history()];
      // Read history sequence.
      if (!history_node.empty() && history_node.is_seq())
      {
        for (auto &&child : history_node.children())
        {
          std::string host;
          UInt port = { Tags::Connection::port().data(), 0, "" };
          bool item_ok = true;
          item_ok = parse(child, Tags::Connection::host().data(), host) && item_ok;
          item_ok = parse(child, port) && item_ok;

          if (!item_ok)
          {
            ok = false;
            continue;
          }

          connection.history.emplace_back(host, static_cast<uint16_t>(port.value()));
        }
      }
    }
  }

  return ok;
}


bool save(ryml::NodeRef &node, const Playback &playback)
{
  bool ok = true;
  node |= ryml::MAP;
  ok = write(node, playback.allow_key_frames) && ok;
  ok = write(node, playback.keyframe_every_mib) && ok;
  ok = write(node, playback.keyframe_every_frames) && ok;
  ok = write(node, playback.keyframe_skip_forward_frames) && ok;
  ok = write(node, playback.keyframe_compression) && ok;
  ok = write(node, playback.looping) && ok;
  ok = write(node, playback.pause_on_error) && ok;
  return ok;
}


bool load(const ryml::NodeRef &node, Render &render)
{
  bool ok = true;
  ok = parse(node, render.use_edl_shader) && ok;
  ok = parse(node, render.edl_radius) && ok;
  ok = parse(node, render.edl_exponential_scale) && ok;
  ok = parse(node, render.edl_linear_scale) && ok;
  ok = parse(node, render.point_size) && ok;
  ok = parse(node, render.background_colour) && ok;
  return ok;
}


bool save(ryml::NodeRef &node, const Render &render)
{
  bool ok = true;
  node |= ryml::MAP;
  ok = write(node, render.use_edl_shader) && ok;
  ok = write(node, render.edl_radius) && ok;
  ok = write(node, render.edl_exponential_scale) && ok;
  ok = write(node, render.edl_linear_scale) && ok;
  ok = write(node, render.point_size) && ok;
  ok = write(node, render.background_colour) && ok;
  return ok;
}

bool save(ryml::NodeRef &node, const Connection &connection)
{
  bool ok = true;

  node |= ryml::MAP;

  if (!connection.history.empty())
  {
    auto history_node = node[Tags::Connection::history()];
    history_node |= ryml::SEQ;

    for (const auto &[host, port] : connection.history)
    {
      ryml::NodeRef history_item = history_node.append_child();
      history_item |= ryml::MAP;
      history_item[Tags::Connection::host()] << host;
      history_item[Tags::Connection::port()] << port;
    }
  }

  return ok;
}
std::filesystem::path userConfigPath()
{
  std::array<char, 1024> user_config_path;
  get_user_config_file(user_config_path.data(), static_cast<unsigned>(user_config_path.size()),
                       "3rdEyeScene");
  std::filesystem::path path{ std::string(user_config_path.data()) };
  path = path.replace_extension("yaml");
  return path;
}
}  // namespace


bool load(Settings::Config &config)
{
  return load(config, userConfigPath());
}


bool load(Settings::Config &config, const std::filesystem::path &path)
{
  std::ifstream in_file(path.c_str(), std::ios::binary);
  if (!in_file.is_open())
  {
    return false;
  }

  in_file.seekg(0, std::ios_base::end);
  const auto byte_count = in_file.tellg();
  in_file.seekg(0, std::ios_base::beg);

  std::vector<char> content(byte_count);
  in_file.read(content.data(), content.size());

  ryml::Tree doc = ryml::parse_in_place(ryml::to_substr(content));

  if (doc.empty())
  {
    return true;
  }

  const auto root = doc.rootref();
  if (root.empty())
  {
    return true;
  }

  bool ok = true;
  if (root.has_child(Tags::Root::camera()))
  {
    ok = load(root[Tags::Root::camera()], config.camera) && ok;
  }
  if (root.has_child(Tags::Root::log()))
  {
    ok = load(root[Tags::Root::log()], config.log) && ok;
  }
  if (root.has_child(Tags::Root::playback()))
  {
    ok = load(root[Tags::Root::playback()], config.playback) && ok;
  }
  if (root.has_child(Tags::Root::render()))
  {
    ok = load(root[Tags::Root::render()], config.render) && ok;
  }
  if (root.has_child(Tags::Root::connection()))
  {
    ok = load(root[Tags::Root::connection()], config.connection) && ok;
  }
  return ok;
}


bool save(const Settings::Config &config)
{
  return save(config, userConfigPath());
}


bool save(const Settings::Config &config, const std::filesystem::path &path)
{
  std::ofstream out_file(path.c_str(), std::ios::binary);
  if (!out_file.is_open())
  {
    return false;
  }

  ryml::Tree doc;
  bool ok = true;

  auto root = doc.rootref();
  // Mark root as a map.
  root |= ryml::MAP;

  ok = save(root[Tags::Root::camera()], config.camera) && ok;
  ok = save(root[Tags::Root::log()], config.log) && ok;
  ok = save(root[Tags::Root::playback()], config.playback) && ok;
  ok = save(root[Tags::Root::render()], config.render) && ok;
  ok = save(root[Tags::Root::connection()], config.connection) && ok;

  out_file << doc;
  out_file.flush();
  out_file.close();

  return ok;
}
}  // namespace tes::view::settings
