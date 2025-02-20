//
// Author: Kazys Stepanas
//
#pragma once

#include <3esview/ViewConfig.h>

#include <3escore/Colour.h>
#include <string>

namespace tes::view::settings
{
/// An enum value for use in settings.
class TES_VIEWER_API Colour
{
public:
  using ValueType = tes::Colour;

  Colour(std::string label, const ValueType &value, std::string tip)
    : _value(value)
    , _label(std::move(label))
    , _tip(std::move(tip))
  {}

  Colour(const Colour &other) = default;
  Colour(Colour &&other) = default;

  ~Colour() = default;

  Colour &operator=(const Colour &other) = default;
  Colour &operator=(Colour &&other) = default;

  [[nodiscard]] const std::string &label() const { return _label; }
  [[nodiscard]] const std::string &tip() const { return _tip; }

  [[nodiscard]] const ValueType &value() const { return _value; }
  void setValue(const ValueType &value) { _value = value; }

  [[nodiscard]] bool operator==(const Colour &other) const
  {
    return _value == other._value && _label == other._label && _tip == other._tip;
  }

  [[nodiscard]] bool operator!=(const Colour &other) const { return !operator==(other); }

private:
  ValueType _value = {};
  std::string _label;
  std::string _tip;
};
}  // namespace tes::view::settings
