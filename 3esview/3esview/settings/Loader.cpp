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


/// Merge to IO codes into the appropriate collective result.
/// @param a The first code.
/// @param b The second code.
/// @return The merged code.
IOCode mergeCode(IOCode a, IOCode b)
{
  return static_cast<IOCode>(std::max(static_cast<unsigned>(a), static_cast<unsigned>(b)));
}


std::ostream &append(std::ostream &out)
{
  out.flush();
  if (out.tellp() != 0)
  {
    out << "\n";
  }
  return out;
}


IOCode parse(const ryml::NodeRef &parent, const std::string &key, std::string &value,
             std::ostream &log)
{
  if (parent.empty())
  {
    append(log) << "Empty parent for: " << key;
    return IOCode::Partial;
  }

  const auto key2 = ryml::csubstr(key.c_str(), key.size());
  if (!parent.has_child(key2))
  {
    append(log) << "Missing node: " << key;
    return IOCode::Partial;
  }

  const auto node = parent[key2];
  if (node.empty())
  {
    append(log) << "Empty node: " << key;
    return IOCode::Partial;
  }

  value = std::string(node.val().data(), node.val().size());
  return IOCode::Ok;
}


IOCode parse(const ryml::NodeRef &parent, Bool &value, std::ostream &log)
{
  if (parent.empty())
  {
    append(log) << "Empty parent for: " << value.label();
    return IOCode::Partial;
  }

  const auto key = ryml::csubstr(value.label().c_str(), value.label().size());
  const auto node = parent[key];
  if (node.empty())
  {
    append(log) << "Empty node: " << value.label();
    return IOCode::Partial;
  }

  std::string str(node.val().data(), node.val().size());
  std::transform(str.begin(), str.end(), str.begin(), [](char ch) { return ::tolower(ch); });
  if (std::find(Tags::Common::trueSet().begin(), Tags::Common::trueSet().end(), str) !=
      Tags::Common::trueSet().end())
  {
    value.setValue(true);
    return IOCode::Ok;
  }
  if (std::find(Tags::Common::falseSet().begin(), Tags::Common::falseSet().end(), str) !=
      Tags::Common::falseSet().end())
  {
    value.setValue(false);
    return IOCode::Ok;
  }

  append(log) << "Parse error for boolean node: " << value.label() << " <- " << str;
  return IOCode::Partial;
}


IOCode parse(const ryml::NodeRef &parent, Colour &value, std::ostream &log)
{
  if (parent.empty())
  {
    append(log) << "Empty parent for: " << value.label();
    return IOCode::Partial;
  }

  const auto key = ryml::csubstr(value.label().c_str(), value.label().size());
  const auto node = parent[key];
  if (node.empty() || !node.type().is_map())
  {
    append(log) << "Empty node: " << value.label();
    return IOCode::Partial;
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
      append(log) << "Error parsing colour value: " << value.label();
      return IOCode::Partial;
    }
    const std::string str(nodes[i].val().data(), nodes[i].val().size());
    std::istringstream in(str);
    int channel = {};
    in >> channel;
    if (in.bad())
    {
      append(log) << "Error parsing colour value: " << value.label();
      return IOCode::Partial;
    }

    colour.channel(channels[i]) = static_cast<uint8_t>(channel);
  }

  value.setValue(colour);

  return IOCode::Ok;
}  // namespace


template <typename T>
IOCode parse(const ryml::NodeRef &parent, Numeric<T> &value, std::ostream &log)
{
  if (parent.empty())
  {
    append(log) << "Empty parent for: " << value.label();
    return IOCode::Partial;
  }


  const auto key = ryml::csubstr(value.label().c_str(), value.label().size());
  if (!parent.has_child(key))
  {
    append(log) << "Missing node: " << value.label();
    return IOCode::Partial;
  }

  const auto node = parent[key];
  if (node.empty())
  {
    append(log) << "Empty node: " << value.label();
    return IOCode::Partial;
  }

  const std::string str(node.val().data(), node.val().size());
  std::istringstream in(str);
  auto temp = value.value();

  in >> temp;
  if (!in.bad())
  {
    value.setValue(temp);
    return IOCode::Ok;
  }

  log << "Error parsing numeric value for: " << value.label() << " <- " << str;
  return IOCode::Partial;
}


template <typename E>
IOCode parse(const ryml::NodeRef &parent, Enum<E> &value, std::ostream &log)
{
  if (parent.empty())
  {
    append(log) << "Empty parent for: " << value.label();
    return IOCode::Partial;
  }

  const auto key = ryml::csubstr(value.label().c_str(), value.label().size());
  if (!parent.has_child(key))
  {
    append(log) << "Missing node: " << value.label();
    return IOCode::Partial;
  }

  const auto node = parent[key];
  if (node.empty())
  {
    append(log) << "Empty node: " << value.label();
    return IOCode::Partial;
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
      return IOCode::Ok;
    }
  }

  log << "Error parsing enum value for: " << value.label() << " <- " << str << '\n';
  log << "Allowed values:\n";
  std::copy(named_values.begin(), named_values.end(),
            std::ostream_iterator<std::string>(log, "\n"));

  return false;
}


template <typename T>
IOCode write(ryml::NodeRef &parent, const T &prop, std::ostream &log)
{
  TES_UNUSED(log);
  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  parent[key] << prop.value();
  return IOCode::Ok;
}


IOCode write(ryml::NodeRef &parent, const Bool &prop, std::ostream &log)
{
  TES_UNUSED(log);
  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  parent[key] << (prop.value() ? Tags::Common::trueStr() : Tags::Common::falseStr());
  return IOCode::Ok;
}


IOCode write(ryml::NodeRef &parent, const Colour &prop, std::ostream &log)
{
  TES_UNUSED(log);
  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  auto node = parent[ryml::csubstr(prop.label().c_str(), prop.label().size())];
  node |= ryml::MAP;

  node[Tags::Common::red()] << static_cast<int>(prop.value().red());
  node[Tags::Common::green()] << static_cast<int>(prop.value().green());
  node[Tags::Common::blue()] << static_cast<int>(prop.value().blue());

  return IOCode::Ok;
}

template <typename T>
IOCode parse(const ryml::NodeRef &parent, T &prop, std::ostream &log)
{
  return parse(parent[prop.label()], prop, log);
}


IOCode load(const ryml::NodeRef &node, Camera &camera, std::ostream &log)
{
  auto code = IOCode::Ok;
  code = mergeCode(parse(node, camera.invert_y, log), code);
  code = mergeCode(parse(node, camera.allow_remote_settings, log), code);
  code = mergeCode(parse(node, camera.near_clip, log), code);
  code = mergeCode(parse(node, camera.far_clip, log), code);
  code = mergeCode(parse(node, camera.fov, log), code);

  return code;
}


IOCode save(ryml::NodeRef node, const Camera &camera, std::ostream &log)
{
  auto code = IOCode::Ok;
  node |= ryml::MAP;
  code = mergeCode(write(node, camera.invert_y, log), code);
  code = mergeCode(write(node, camera.allow_remote_settings, log), code);
  code = mergeCode(write(node, camera.near_clip, log), code);
  code = mergeCode(write(node, camera.far_clip, log), code);
  code = mergeCode(write(node, camera.fov, log), code);

  return code;
}


IOCode load(const ryml::NodeRef &node, Log &log_config, std::ostream &log)
{
  auto code = IOCode::Ok;
  code = mergeCode(parse(node, log_config.log_window_size, log), code);
  return code;
}


IOCode save(ryml::NodeRef &node, const Log &log_settings, std::ostream &log)
{
  auto code = IOCode::Ok;
  node |= ryml::MAP;
  code = mergeCode(write(node, log_settings.log_window_size, log), code);
  return code;
}


IOCode load(const ryml::NodeRef &node, Playback &playback, std::ostream &log)
{
  auto code = IOCode::Ok;
  code = mergeCode(parse(node, playback.allow_key_frames, log), code);
  code = mergeCode(parse(node, playback.keyframe_every_mib, log), code);
  code = mergeCode(parse(node, playback.keyframe_every_frames, log), code);
  code = mergeCode(parse(node, playback.keyframe_min_separation, log), code);
  code = mergeCode(parse(node, playback.keyframe_compression, log), code);
  code = mergeCode(parse(node, playback.looping, log), code);
  code = mergeCode(parse(node, playback.pause_on_error, log), code);
  return code;
}


IOCode load(const ryml::NodeRef &node, Connection &connection, std::ostream &log)
{
  connection.history.clear();
  if (node.empty())
  {
    append(log) << "Empty node: " << Tags::Root::connection().data();
    return IOCode::Partial;
  }

  auto code = IOCode::Ok;
  if (node.has_child(Tags::Connection::history()))
  {
    const auto history_node = node[Tags::Connection::history()];
    // Read history sequence.
    if (history_node.empty() && history_node.is_seq())
    {
      code = mergeCode(IOCode::Partial, code);
      append(log) << "Empty node: " << Tags::Connection::history().data();
    }
    else
    {
      for (auto &&child : history_node.children())
      {
        std::string host;
        UInt port = { Tags::Connection::port().data(), 0, "" };
        auto item_code = IOCode::Ok;
        item_code = mergeCode(parse(child, Tags::Connection::host().data(), host, log), item_code);
        item_code = mergeCode(parse(child, port, log), item_code);

        if (item_code == IOCode::Ok)
        {
          connection.history.emplace_back(host, static_cast<uint16_t>(port.value()));
        }

        code = mergeCode(item_code, code);
      }
    }
  }

  return code;
}


IOCode save(ryml::NodeRef &node, const Playback &playback, std::ostream &log)
{
  auto code = IOCode::Ok;
  node |= ryml::MAP;
  code = mergeCode(write(node, playback.allow_key_frames, log), code);
  code = mergeCode(write(node, playback.keyframe_every_mib, log), code);
  code = mergeCode(write(node, playback.keyframe_every_frames, log), code);
  code = mergeCode(write(node, playback.keyframe_min_separation, log), code);
  code = mergeCode(write(node, playback.keyframe_compression, log), code);
  code = mergeCode(write(node, playback.looping, log), code);
  code = mergeCode(write(node, playback.pause_on_error, log), code);
  return code;
}


IOCode load(const ryml::NodeRef &node, Render &render, std::ostream &log)
{
  auto code = IOCode::Ok;
  code = mergeCode(parse(node, render.use_edl_shader, log), code);
  code = mergeCode(parse(node, render.edl_radius, log), code);
  code = mergeCode(parse(node, render.edl_exponential_scale, log), code);
  code = mergeCode(parse(node, render.edl_linear_scale, log), code);
  code = mergeCode(parse(node, render.point_size, log), code);
  code = mergeCode(parse(node, render.background_colour, log), code);
  return code;
}


IOCode save(ryml::NodeRef &node, const Render &render, std::ostream &log)
{
  auto code = IOCode::Ok;
  node |= ryml::MAP;
  code = mergeCode(write(node, render.use_edl_shader, log), code);
  code = mergeCode(write(node, render.edl_radius, log), code);
  code = mergeCode(write(node, render.edl_exponential_scale, log), code);
  code = mergeCode(write(node, render.edl_linear_scale, log), code);
  code = mergeCode(write(node, render.point_size, log), code);
  code = mergeCode(write(node, render.background_colour, log), code);
  return code;
}


IOCode save(ryml::NodeRef &node, const Connection &connection, std::ostream &log)
{
  TES_UNUSED(log);
  auto code = IOCode::Ok;

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

  return code;
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


IOResult load(Settings::Config &config)
{
  return load(config, userConfigPath());
}


IOResult load(Settings::Config &config, const std::filesystem::path &path)
{
  std::ifstream in_file(path.c_str(), std::ios::binary);
  if (!in_file.is_open())
  {
    return { IOCode::Error, "Failed to read file " + path.string() };
  }

  in_file.seekg(0, std::ios_base::end);
  const auto byte_count = in_file.tellg();
  in_file.seekg(0, std::ios_base::beg);

  std::vector<char> content(byte_count);
  in_file.read(content.data(), content.size());

  ryml::Tree doc = ryml::parse_in_place(ryml::to_substr(content));

  if (doc.empty())
  {
    return { IOCode::Ok, {} };
  }

  const auto root = doc.rootref();
  if (root.empty())
  {
    return { IOCode::Ok, {} };
  }

  IOCode code = IOCode::Ok;
  std::ostringstream log_stream;
  if (root.has_child(Tags::Root::camera()))
  {
    code = mergeCode(load(root[Tags::Root::camera()], config.camera, log_stream), code);
  }
  if (root.has_child(Tags::Root::log()))
  {
    code = mergeCode(load(root[Tags::Root::log()], config.log, log_stream), code);
  }
  if (root.has_child(Tags::Root::playback()))
  {
    code = mergeCode(load(root[Tags::Root::playback()], config.playback, log_stream), code);
  }
  if (root.has_child(Tags::Root::render()))
  {
    code = mergeCode(load(root[Tags::Root::render()], config.render, log_stream), code);
  }
  if (root.has_child(Tags::Root::connection()))
  {
    code = mergeCode(load(root[Tags::Root::connection()], config.connection, log_stream), code);
  }

  log_stream.flush();
  return { code, log_stream.str() };
}


IOResult save(const Settings::Config &config)
{
  return save(config, userConfigPath());
}


IOResult save(const Settings::Config &config, const std::filesystem::path &path)
{
  std::ofstream out_file(path.c_str(), std::ios::binary);
  if (!out_file.is_open())
  {
    return { IOCode::Error, "Unable to write settings file " + path.string() };
  }

  ryml::Tree doc;
  auto code = IOCode::Ok;
  std::ostringstream log_stream;

  auto root = doc.rootref();
  // Mark root as a map.
  root |= ryml::MAP;

  code = mergeCode(save(root[Tags::Root::camera()], config.camera, log_stream), code);
  code = mergeCode(save(root[Tags::Root::log()], config.log, log_stream), code);
  code = mergeCode(save(root[Tags::Root::playback()], config.playback, log_stream), code);
  code = mergeCode(save(root[Tags::Root::render()], config.render, log_stream), code);
  code = mergeCode(save(root[Tags::Root::connection()], config.connection, log_stream), code);

  out_file << doc;
  out_file.flush();
  out_file.close();

  log_stream.flush();
  return { code, log_stream.str() };
}
}  // namespace tes::view::settings
