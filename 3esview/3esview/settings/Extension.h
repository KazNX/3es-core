//
// Author: Kazys Stepanas
//
#ifndef TES_VIEW_SETTINGS_EXTENSION_H
#define TES_VIEW_SETTINGS_EXTENSION_H

#include <3esview/ViewConfig.h>

#include "Enum.h"
#include "IOResult.h"
#include "Colour.h"
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
enum PropertyType
{
  Unknown,
  Bool,
  Colour,
  Enum,
  Int,
  UInt,
  Float,
  Double
};

class TES_VIEWER_API ExtensionPropertyAffordances
{
public:
  virtual ~ExtensionPropertyAffordances();

  virtual PropertyType type() const = 0;
  virtual const std::string &label() const = 0;
  virtual const std::string &tip() const = 0;
  virtual IOCode read(const ryml::NodeRef &parent, std::ostream &log) = 0;
  virtual IOCode write(ryml::NodeRef &parent, std::ostream &log) const = 0;
  virtual std::unique_ptr<ExtensionPropertyAffordances> clone() const = 0;
  virtual void *property() = 0;
  virtual const void *property() const = 0;
};
}  // namespace detail

class TES_VIEWER_API ExtensionProperty
{
public:
  explicit ExtensionProperty(const Bool &property);
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
      _affordances = other._affordances->clone();
    }
  }

  ExtensionProperty &operator=(ExtensionProperty &&other) = default;
  ExtensionProperty &operator=(const ExtensionProperty &other)
  {
    if (this != &other)
    {
      _affordances = nullptr;
      if (other._affordances)
      {
        _affordances = other._affordances->clone();
      }
    }
    return *this;
  }

  const std::string &label() const { return _affordances->label(); }
  const std::string &tip() const { return _affordances->tip(); }
  IOCode read(const ryml::NodeRef &parent, std::ostream &log)
  {
    return _affordances->read(parent, log);
  }
  IOCode write(ryml::NodeRef &parent, std::ostream &log) const
  {
    return _affordances->write(parent, log);
  }

  void update(const ExtensionProperty &other);

  template <typename T>
  T *getProperty();
  template <typename T>
  const T *getProperty() const;

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
  virtual ~Extension();

  Extension &operator=(const Extension &other) = default;
  Extension &operator=(Extension &&other) = default;

  const std::string &name() const { return _name; }

  void add(const ExtensionProperty &property);

  const Properties &properties() const { return _properties; }

  Properties::iterator begin() { return _properties.begin(); }
  Properties::const_iterator begin() const { return _properties.begin(); }
  Properties::iterator end() { return _properties.end(); }
  Properties::const_iterator end() const { return _properties.end(); }

  void update(const Extension &other);

private:
  Properties _properties;
  std::string _name;
};
}  // namespace tes::view::settings

#endif  // TES_VIEW_SETTINGS_EXTENSION_H
