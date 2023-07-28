//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_EXTENSION_H
#define TES_VIEW_SETTINGS_EXTENSION_H

#include <3esview/ViewConfig.h>

#include "Colour.h"
#include "Enum.h"
#include "IOResult.h"
#include "Numeric.h"
#include "YamlFwd.h"

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace tes::view::settings
{
namespace detail
{
enum class PropertyType
{
  Unknown,
  Boolean,
  Colour,
  Enum,
  Int,
  UInt,
  Float,
  Double
};

template <typename T>
struct TES_VIEWER_API PropertyTypeValueOf
{
  static constexpr PropertyType value() { return PropertyType::Unknown; }
};

template <>
struct TES_VIEWER_API PropertyTypeValueOf<settings::Boolean>
{
  static constexpr PropertyType value() { return PropertyType::Boolean; }
};

template <>
struct TES_VIEWER_API PropertyTypeValueOf<settings::Colour>
{
  static constexpr PropertyType value() { return PropertyType::Colour; }
};

template <>
struct TES_VIEWER_API PropertyTypeValueOf<settings::Enum>
{
  static constexpr PropertyType value() { return PropertyType::Enum; }
};

template <>
struct TES_VIEWER_API PropertyTypeValueOf<settings::Int>
{
  static constexpr PropertyType value() { return PropertyType::Int; }
};

template <>
struct TES_VIEWER_API PropertyTypeValueOf<settings::UInt>
{
  static constexpr PropertyType value() { return PropertyType::UInt; }
};

template <>
struct TES_VIEWER_API PropertyTypeValueOf<settings::Float>
{
  static constexpr PropertyType value() { return PropertyType::Float; }
};

template <>
struct TES_VIEWER_API PropertyTypeValueOf<settings::Double>
{
  static constexpr PropertyType value() { return PropertyType::Double; }
};

class TES_VIEWER_API ExtensionPropertyAffordances
{
public:
  ExtensionPropertyAffordances() = default;
  ExtensionPropertyAffordances(const ExtensionPropertyAffordances &other) = default;
  virtual ~ExtensionPropertyAffordances();
  ExtensionPropertyAffordances &operator=(const ExtensionPropertyAffordances &other) = default;

  [[nodiscard]] virtual PropertyType type() const = 0;
  [[nodiscard]] virtual const std::string &label() const = 0;
  [[nodiscard]] virtual const std::string &tip() const = 0;
  virtual IOCode read(const ryml::ConstNodeRef &parent, std::ostream &log) = 0;
  virtual IOCode write(ryml::NodeRef &parent, std::ostream &log) const = 0;
  [[nodiscard]] virtual ExtensionPropertyAffordances *clone() const = 0;
  [[nodiscard]] virtual void *property() = 0;
  [[nodiscard]] virtual const void *property() const = 0;
};
}  // namespace detail

/// A property object wrapper used with @c Extension property settings.
///
/// This class uses type erasure to wrap a property type object for use in configuration settings.
/// The constructor accepts any of the property types, then erases the type information using a
/// derivation of @c ExtensionPropertyAffordances to hold a copy of the property object.
///
/// The following code needs to be added when new property types are added;
/// - Add a @c PropertyType enumeration member.
/// - Define a @c PropertyTypeValueOf specialisation which returns the @c PropertyType value.
/// - Add a constructor for that property type.
/// - Implement @c ExtensionPropertyAffordances for that type (use @c DEFINE_AFFORDANCES() in cpp)
/// - Extend @c operator==() to consider the new property type.
class TES_VIEWER_API ExtensionProperty
{
public:
  explicit ExtensionProperty(const Boolean &property);
  explicit ExtensionProperty(const Enum &property);
  explicit ExtensionProperty(const Colour &property);
  explicit ExtensionProperty(const Int &property);
  explicit ExtensionProperty(const UInt &property);
  explicit ExtensionProperty(const Float &property);
  explicit ExtensionProperty(const Double &property);

  ExtensionProperty(ExtensionProperty &&other) = default;
  ExtensionProperty(const ExtensionProperty &other)
  {
    if (other._affordances)
    {
      _affordances.reset(other._affordances->clone());
    }
  }

  ~ExtensionProperty() = default;

  ExtensionProperty &operator=(ExtensionProperty &&other) = default;
  ExtensionProperty &operator=(const ExtensionProperty &other)
  {
    if (this != &other)
    {
      _affordances = nullptr;
      if (other._affordances)
      {
        _affordances.reset(other._affordances->clone());
      }
    }
    return *this;
  }

  [[nodiscard]] const std::string &label() const { return _affordances->label(); }
  [[nodiscard]] const std::string &tip() const { return _affordances->tip(); }
  IOCode read(const ryml::ConstNodeRef &parent, std::ostream &log)
  {
    return _affordances->read(parent, log);
  }
  IOCode write(ryml::NodeRef &parent, std::ostream &log) const
  {
    return _affordances->write(parent, log);
  }

  void update(const ExtensionProperty &other);

  [[nodiscard]] bool operator==(const ExtensionProperty &other) const;
  [[nodiscard]] bool operator!=(const ExtensionProperty &other) const { return !operator==(other); }

  template <typename T>
  [[nodiscard]] T *getProperty();
  template <typename T>
  [[nodiscard]] const T *getProperty() const;

private:
  std::unique_ptr<detail::ExtensionPropertyAffordances> _affordances;
};

/// A settings extension for storing additional settings, not explicitly known in the @c tes::view
/// API.
class TES_VIEWER_API Extension
{
public:
  using Properties = std::vector<ExtensionProperty>;

  Extension(const std::string &name);
  Extension(const Extension &other) = default;
  Extension(Extension &&other) = default;
  ~Extension();

  Extension &operator=(const Extension &other) = default;
  Extension &operator=(Extension &&other) = default;

  [[nodiscard]] const std::string &name() const { return _name; }

  void add(const ExtensionProperty &property);

  [[nodiscard]] const Properties &properties() const { return _properties; }

  [[nodiscard]] Properties::iterator begin() { return _properties.begin(); }
  [[nodiscard]] Properties::const_iterator begin() const { return _properties.begin(); }
  [[nodiscard]] Properties::iterator end() { return _properties.end(); }
  [[nodiscard]] Properties::const_iterator end() const { return _properties.end(); }

  void update(const Extension &other);

  [[nodiscard]] bool operator==(const Extension &other) const
  {
    return _name == other._name && _properties == other._properties;
  }

  [[nodiscard]] bool operator!=(const Extension &other) const { return !operator==(other); }

  [[nodiscard]] bool hasProperty(const std::string &key) const;
  [[nodiscard]] const ExtensionProperty &operator[](const std::string &key) const;
  [[nodiscard]] ExtensionProperty &operator[](const std::string &key);

private:
  Properties _properties;
  std::string _name;
};


template <typename T>
T *ExtensionProperty::getProperty()
{
  if (_affordances && _affordances->type() == detail::PropertyTypeValueOf<T>::value())
  {
    return reinterpret_cast<T *>(_affordances->property());
  }
  return nullptr;
}


template <typename T>
const T *ExtensionProperty::getProperty() const
{
  if (_affordances && _affordances->type() == detail::PropertyTypeValueOf<T>::value())
  {
    return reinterpret_cast<const T *>(_affordances->property());
  }
  return nullptr;
}
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_EXTENSION_H
