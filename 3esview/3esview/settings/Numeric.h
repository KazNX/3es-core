//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_NUMERIC_H
#define TES_VIEW_SETTINGS_NUMERIC_H

#include <3esview/ViewConfig.h>

#include <limits>
#include <optional>
#include <string>

namespace tes::view::settings
{
/// A numeric value for use in settings.
template <typename T>
class Numeric
{
public:
  Numeric(std::string label, T value, std::string tip)
    : _value(value)
    , _label(std::move(label))
    , _tip(std::move(tip))
  {}
  Numeric(std::string label, T value, T minimum, std::string tip)
    : _value(value)
    , _minimum(minimum)
    , _label(std::move(label))
    , _tip(std::move(tip))
  {}
  Numeric(std::string label, T value, std::string tip, T maximum)
    : _value(value)
    , _maximum(maximum)
    , _label(std::move(label))
    , _tip(std::move(tip))
  {}
  Numeric(std::string label, T value, T minimum, T maximum, std::string tip)
    : _value(value)
    , _minimum(minimum)
    , _maximum(maximum)
    , _label(std::move(label))
    , _tip(std::move(tip))
  {}

  Numeric(const Numeric<T> &other) = default;
  Numeric(Numeric<T> &&other) noexcept = default;

  ~Numeric() = default;

  Numeric<T> &operator=(const Numeric<T> &other) = default;
  Numeric<T> &operator=(Numeric<T> &&other) noexcept = default;

  [[nodiscard]] const std::string &label() const { return _label; }
  [[nodiscard]] const std::string &tip() const { return _tip; }

  [[nodiscard]] const T &value() const { return _value; }
  void setValue(T value) { _value = std::max(minimum(), std::min(value, maximum())); }

  [[nodiscard]] bool hasMinimum() const { return _minimum.has_value(); }
  [[nodiscard]] T minimum() const
  {
    return (hasMinimum()) ? *_minimum : std::numeric_limits<T>::lowest();
  }
  void setMinimum(T minimum) { _minimum = minimum; }

  [[nodiscard]] bool hasMaximum() const { return _maximum.has_value(); }
  [[nodiscard]] T maximum() const
  {
    return (hasMaximum()) ? *_maximum : std::numeric_limits<T>::max();
  }
  void setMaximum(T maximum) { _maximum = maximum; }

  [[nodiscard]] bool operator==(const Numeric<T> &other) const
  {
    return _value == other._value && _minimum == other._minimum && _maximum == other._maximum &&
           _label == other._label && _tip == other._tip;
  }

  [[nodiscard]] bool operator!=(const Numeric<T> &other) const { return !operator==(other); }

private:
  T _value = {};
  std::optional<T> _minimum;
  std::optional<T> _maximum;
  std::string _label;
  std::string _tip;
};

using Int = Numeric<int>;
using UInt = Numeric<unsigned>;
using Float = Numeric<float>;
using Double = Numeric<double>;

class TES_VIEWER_API Boolean
{
public:
  Boolean(std::string label, bool value, std::string tip)
    : _value(value)
    , _label(std::move(label))
    , _tip(std::move(tip))
  {}

  Boolean(const Boolean &other) = default;
  Boolean(Boolean &&other) = default;

  ~Boolean() = default;

  Boolean &operator=(const Boolean &other) = default;
  Boolean &operator=(Boolean &&other) = default;

  [[nodiscard]] const std::string &label() const { return _label; }
  [[nodiscard]] const std::string &tip() const { return _tip; }

  [[nodiscard]] bool value() const { return _value; }
  void setValue(bool value) { _value = value; }

  [[nodiscard]] bool operator==(const Boolean &other) const
  {
    return _value == other._value && _label == other._label && _tip == other._tip;
  }

  [[nodiscard]] bool operator!=(const Boolean &other) const { return !operator==(other); }

private:
  bool _value = false;
  std::string _label;
  std::string _tip;
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_NUMERIC_H
