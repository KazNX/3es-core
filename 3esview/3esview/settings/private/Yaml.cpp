//
// Author: Kazys Stepanas
//
#include "Yaml.h"

#include <3esview/settings/Colour.h>
#include <3esview/settings/Enum.h>
#include <3esview/settings/Numeric.h>

#include <ryml/ryml.hpp>
#include <ryml/ryml_std.hpp>

#include <c4/format.hpp>

#include <iostream>
#include <sstream>
#include <string>

namespace
{
struct Tags
{
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

std::ostream &append(std::ostream &out)
{
  out.flush();
  if (out.tellp() != 0)
  {
    out << "\n";
  }
  return out;
}
}  // namespace

namespace tes::view::settings::priv
{
IOCode read(const ryml::ConstNodeRef &parent, const std::string &key, std::string &value,
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


IOCode read(const ryml::ConstNodeRef &parent, Bool &prop, std::ostream &log)
{
  if (parent.empty())
  {
    append(log) << "Empty parent for: " << prop.label();
    return IOCode::Partial;
  }

  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  const auto node = parent[key];
  if (node.empty())
  {
    append(log) << "Empty node: " << prop.label();
    return IOCode::Partial;
  }

  std::string str(node.val().data(), node.val().size());
  std::transform(str.begin(), str.end(), str.begin(), [](char ch) { return ::tolower(ch); });
  if (std::find(Tags::Common::trueSet().begin(), Tags::Common::trueSet().end(), str) !=
      Tags::Common::trueSet().end())
  {
    prop.setValue(true);
    return IOCode::Ok;
  }
  if (std::find(Tags::Common::falseSet().begin(), Tags::Common::falseSet().end(), str) !=
      Tags::Common::falseSet().end())
  {
    prop.setValue(false);
    return IOCode::Ok;
  }

  append(log) << "Parse error for boolean node: " << prop.label() << " <- " << str;
  return IOCode::Partial;
}


IOCode read(const ryml::ConstNodeRef &parent, Colour &prop, std::ostream &log)
{
  if (parent.empty())
  {
    append(log) << "Empty parent for: " << prop.label();
    return IOCode::Partial;
  }

  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  const auto node = parent[key];
  if (node.empty() || !node.type().is_map())
  {
    append(log) << "Empty node: " << prop.label();
    return IOCode::Partial;
  }

  std::array<ryml::ConstNodeRef, 3> nodes = {
    node[Tags::Common::red()],
    node[Tags::Common::green()],
    node[Tags::Common::blue()],
  };
  std::array<tes::Colour::Channel, 3> channels = { tes::Colour::Channel::R, tes::Colour::Channel::G,
                                                   tes::Colour::Channel::B };

  auto colour = prop.value();
  for (size_t i = 0; i < nodes.size(); ++i)
  {
    if (nodes[i].empty())
    {
      append(log) << "Error parsing colour prop: " << prop.label();
      return IOCode::Partial;
    }
    const std::string str(nodes[i].val().data(), nodes[i].val().size());
    std::istringstream in(str);
    int channel = {};
    in >> channel;
    if (in.bad())
    {
      append(log) << "Error parsing colour value: " << prop.label();
      return IOCode::Partial;
    }

    colour.channel(channels[i]) = static_cast<uint8_t>(channel);
  }

  prop.setValue(colour);

  return IOCode::Ok;
}  // namespace


template <typename T>
IOCode readNumeric(const ryml::ConstNodeRef &parent, Numeric<T> &prop, std::ostream &log)
{
  if (parent.empty())
  {
    append(log) << "Empty parent for: " << prop.label();
    return IOCode::Partial;
  }


  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  if (!parent.has_child(key))
  {
    append(log) << "Missing node: " << prop.label();
    return IOCode::Partial;
  }

  const auto node = parent[key];
  if (node.empty())
  {
    append(log) << "Empty node: " << prop.label();
    return IOCode::Partial;
  }

  const std::string str(node.val().data(), node.val().size());
  std::istringstream in(str);
  auto temp = prop.value();

  in >> temp;
  if (!in.bad())
  {
    prop.setValue(temp);
    return IOCode::Ok;
  }

  log << "Error parsing numeric value for: " << prop.label() << " <- " << str;
  return IOCode::Partial;
}


IOCode read(const ryml::ConstNodeRef &parent, Enum &prop, std::ostream &log)
{
  if (parent.empty())
  {
    append(log) << "Empty parent for: " << prop.label();
    return IOCode::Partial;
  }


  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  if (!parent.has_child(key))
  {
    append(log) << "Missing node: " << prop.label();
    return IOCode::Partial;
  }

  const auto node = parent[key];
  if (node.empty())
  {
    append(log) << "Empty node: " << prop.label();
    return IOCode::Partial;
  }

  const std::string str(node.val().data(), node.val().size());

  if (prop.setValueByName(str))
  {
    return IOCode::Ok;
  }

  log << "Error parsing enum value for: " << prop.label() << " <- " << str;
  return IOCode::Partial;
}


IOCode read(const ryml::ConstNodeRef &parent, Int &prop, std::ostream &log)
{
  return readNumeric(parent, prop, log);
}


IOCode read(const ryml::ConstNodeRef &parent, UInt &prop, std::ostream &log)
{
  return readNumeric(parent, prop, log);
}


IOCode read(const ryml::ConstNodeRef &parent, Float &prop, std::ostream &log)
{
  return readNumeric(parent, prop, log);
}


IOCode read(const ryml::ConstNodeRef &parent, Double &prop, std::ostream &log)
{
  return readNumeric(parent, prop, log);
}


template <typename T>
IOCode writeNumeric(ryml::NodeRef &parent, const T &prop, std::ostream &log)
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


IOCode write(ryml::NodeRef &parent, const Enum &prop, std::ostream &log)
{
  TES_UNUSED(log);
  const auto key = ryml::csubstr(prop.label().c_str(), prop.label().size());
  parent[key] << prop.valueName();
  return IOCode::Ok;
}


IOCode write(ryml::NodeRef &parent, const Int &prop, std::ostream &log)
{
  return writeNumeric(parent, prop, log);
}


IOCode write(ryml::NodeRef &parent, const UInt &prop, std::ostream &log)
{
  return writeNumeric(parent, prop, log);
}


IOCode write(ryml::NodeRef &parent, const Float &prop, std::ostream &log)
{
  return writeNumeric(parent, prop, log);
}


IOCode write(ryml::NodeRef &parent, const Double &prop, std::ostream &log)
{
  return writeNumeric(parent, prop, log);
}
}  // namespace tes::view::settings::priv
