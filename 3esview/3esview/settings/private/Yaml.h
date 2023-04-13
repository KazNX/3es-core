//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_PRIVATE_YAML_H
#define TES_VIEW_SETTINGS_PRIVATE_YAML_H

#include <3esview/ViewConfig.h>

#include <3esview/settings/IOResult.h>
#include <3esview/settings/YamlFwd.h>

#include <iosfwd>

namespace tes::view::settings
{
class Bool;
class Colour;
class Enum;
template <typename T>
class Numeric;
using Int = Numeric<int>;
using UInt = Numeric<unsigned>;
using Float = Numeric<float>;
using Double = Numeric<double>;
}  // namespace tes::view::settings

namespace tes::view::settings::priv
{
IOCode read(const ryml::NodeRef &parent, const std::string &key, std::string &value,
            std::ostream &log);
IOCode read(const ryml::NodeRef &parent, Bool &prop, std::ostream &log);
IOCode read(const ryml::NodeRef &parent, Colour &prop, std::ostream &log);
IOCode read(const ryml::NodeRef &parent, Enum &prop, std::ostream &log);
IOCode read(const ryml::NodeRef &parent, Int &prop, std::ostream &log);
IOCode read(const ryml::NodeRef &parent, UInt &prop, std::ostream &log);
IOCode read(const ryml::NodeRef &parent, Float &prop, std::ostream &log);
IOCode read(const ryml::NodeRef &parent, Double &prop, std::ostream &log);

IOCode write(ryml::NodeRef &parent, const Bool &prop, std::ostream &log);
IOCode write(ryml::NodeRef &parent, const Colour &prop, std::ostream &log);
IOCode write(ryml::NodeRef &parent, const Enum &prop, std::ostream &log);
IOCode write(ryml::NodeRef &parent, const Int &prop, std::ostream &log);
IOCode write(ryml::NodeRef &parent, const UInt &prop, std::ostream &log);
IOCode write(ryml::NodeRef &parent, const Float &prop, std::ostream &log);
IOCode write(ryml::NodeRef &parent, const Double &prop, std::ostream &log);
}  // namespace tes::view::settings::priv

#endif  // TES_VIEW_SETTINGS_PRIVATE_YAML_H