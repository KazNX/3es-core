//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_ENUM_H
#define TES_VIEW_SETTINGS_ENUM_H

#include <3esview/ViewConfig.h>

#include <algorithm>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

namespace tes::view::settings
{
/// An enum value for use in settings.
class Enum
{
public:
  template <typename E>
  Enum(const std::string &label, E value, const std::string &tip,
       std::initializer_list<std::pair<E, std::string>> named_values);

  Enum(const Enum &other) = default;
  Enum(Enum &&other) = default;

  ~Enum() = default;

  Enum &operator=(const Enum &other) = default;
  Enum &operator=(Enum &&other) = default;

  [[nodiscard]] const std::string &label() const { return _label; }
  [[nodiscard]] const std::string &tip() const { return _tip; }

  [[nodiscard]] const int &value() const { return _value; }
  template <typename E>
  [[nodiscard]] const E &enumValue() const
  {
    return static_cast<E>(_value);
  }
  void setValue(int value) { _value = value; }
  template <typename E>
  void setValue(E value)
  {
    _value = static_cast<int>(value);
  }

  bool setValueByName(const std::string &name);

  [[nodiscard]] std::string valueName() const { return enumName(_value); }

  template <typename E>
  [[nodiscard]] std::string enumValueName(E value) const
  {
    return enumValue(static_cast<int>(value));
  }

  [[nodiscard]] std::string enumName(int value) const;

  [[nodiscard]] const std::vector<std::pair<int, std::string>> &namedValues() const
  {
    return _named_values;
  }

  [[nodiscard]] bool operator==(const Enum &other) const
  {
    return _value == other._value && _named_values == other._named_values &&
           _label == other._label && _tip == other._tip;
  }

  [[nodiscard]] bool operator!=(const Enum &other) const { return !operator==(other); }

private:
  int _value = {};
  std::vector<std::pair<int, std::string>> _named_values;
  std::string _label;
  std::string _tip;
};


template <typename E>
inline Enum::Enum(const std::string &label, E value, const std::string &tip,
                  std::initializer_list<std::pair<E, std::string>> named_values)
  : _label(label)
  , _value(static_cast<int>(value))
  , _tip(tip)
{
  for (const auto &[value, name] : named_values)
  {
    _named_values.emplace_back(static_cast<int>(value), name);
  }
}


inline bool Enum::setValueByName(const std::string &name)
{
  return std::any_of(_named_values.begin(), _named_values.end(),
                     [&name, this](std::pair<int, std::string> &item) {
                       if (name == item.second)
                       {
                         _value = item.first;
                         return true;
                       }
                       return false;
                     });
}


inline std::string Enum::enumName(int value) const
{
  for (const auto &[e, str] : _named_values)
  {
    if (value == e)
    {
      return str;
    }
  }
  return {};
}
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_ENUM_H
