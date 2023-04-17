//
// Author: Kazys Stepanas
//
#include "Extension.h"

#include "private/Yaml.h"

#include <iostream>

/// Implement @c ExtensionPropertyAffordances for properties of @c Type .
#define DEFINE_AFFORDANCES(Type)                                         \
  class Type##Affordances : public ExtensionPropertyAffordances          \
  {                                                                      \
  public:                                                                \
    Type##Affordances(const settings::Type &property)                    \
      : _property(property)                                              \
    {}                                                                   \
                                                                         \
    PropertyType type() const override                                   \
    {                                                                    \
      return PropertyType::Type;                                         \
    }                                                                    \
    const std::string &label() const override                            \
    {                                                                    \
      return _property.label();                                          \
    }                                                                    \
    const std::string &tip() const override                              \
    {                                                                    \
      return _property.tip();                                            \
    }                                                                    \
    IOCode read(const ryml::NodeRef &parent, std::ostream &log) override \
    {                                                                    \
      return priv::read(parent, _property, log);                         \
    }                                                                    \
    IOCode write(ryml::NodeRef &node, std::ostream &log) const override  \
    {                                                                    \
      return priv::write(node, _property, log);                          \
    }                                                                    \
    std::unique_ptr<ExtensionPropertyAffordances> clone() const override \
    {                                                                    \
      return std::make_unique<Type##Affordances>(_property);             \
    }                                                                    \
    void *property() override                                            \
    {                                                                    \
      return &_property;                                                 \
    }                                                                    \
    const void *property() const override                                \
    {                                                                    \
      return &_property;                                                 \
    }                                                                    \
                                                                         \
  private:                                                               \
    settings::Type _property;                                            \
  }

namespace tes::view::settings
{
namespace detail
{
ExtensionPropertyAffordances::~ExtensionPropertyAffordances() = default;

DEFINE_AFFORDANCES(Bool);
DEFINE_AFFORDANCES(Enum);
DEFINE_AFFORDANCES(Colour);
DEFINE_AFFORDANCES(Int);
DEFINE_AFFORDANCES(UInt);
DEFINE_AFFORDANCES(Float);
DEFINE_AFFORDANCES(Double);
}  // namespace detail


ExtensionProperty::ExtensionProperty(const Bool &property)
  : _affordances(std::make_unique<detail::BoolAffordances>(property))
{}


ExtensionProperty::ExtensionProperty(const Enum &property)
  : _affordances(std::make_unique<detail::EnumAffordances>(property))
{}


ExtensionProperty::ExtensionProperty(const Colour &property)
  : _affordances(std::make_unique<detail::ColourAffordances>(property))
{}


ExtensionProperty::ExtensionProperty(const Int &property)
  : _affordances(std::make_unique<detail::IntAffordances>(property))
{}


ExtensionProperty::ExtensionProperty(const UInt &property)
  : _affordances(std::make_unique<detail::UIntAffordances>(property))
{}


ExtensionProperty::ExtensionProperty(const Float &property)
  : _affordances(std::make_unique<detail::FloatAffordances>(property))
{}


ExtensionProperty::ExtensionProperty(const Double &property)
  : _affordances(std::make_unique<detail::DoubleAffordances>(property))
{}


void ExtensionProperty::update(const ExtensionProperty &other)
{
  // Note(KS): this isn't very nice or efficient, but it's simple.
  if (other._affordances)
  {
    _affordances = other._affordances->clone();
  }
  else
  {
    _affordances = nullptr;
  }
}


bool ExtensionProperty::operator==(const ExtensionProperty &other) const
{
  // TODO(KS): find a better way to do the type comparison such that we don't need to keep adding
  // code if we add property types.
  if (const auto *this_bool = getProperty<Bool>())
  {
    const auto *other_bool = other.getProperty<Bool>();
    if (other_bool)
    {
      return *this_bool == *other_bool;
    }
  }
  if (const auto *this_colour = getProperty<Colour>())
  {
    const auto *other_colour = other.getProperty<Colour>();
    if (other_colour)
    {
      return *this_colour == *other_colour;
    }
  }
  if (const auto *this_enum = getProperty<Enum>())
  {
    const auto *other_enum = other.getProperty<Enum>();
    if (other_enum)
    {
      return *this_enum == *other_enum;
    }
  }
  if (const auto *this_int = getProperty<Int>())
  {
    const auto *other_int = other.getProperty<Int>();
    if (other_int)
    {
      return *this_int == *other_int;
    }
  }
  if (const auto *this_uint = getProperty<UInt>())
  {
    const auto *other_uint = other.getProperty<UInt>();
    if (other_uint)
    {
      return *this_uint == *other_uint;
    }
  }
  if (const auto *this_float = getProperty<Float>())
  {
    const auto *other_float = other.getProperty<Float>();
    if (other_float)
    {
      return *this_float == *other_float;
    }
  }
  if (const auto *this_double = getProperty<Double>())
  {
    const auto *other_double = other.getProperty<Double>();
    if (other_double)
    {
      return *this_double == *other_double;
    }
  }

  return false;
}

Extension::Extension(const std::string &name)
  : _name(name)
{}


Extension::~Extension() = default;


void Extension::add(const ExtensionProperty &property)
{
  _properties.emplace_back(property);
}


void Extension::update(const Extension &other)
{
  for (const auto &prop : other.properties())
  {
    const auto property_name = prop.label();
    auto iter = std::find_if(
      _properties.begin(), _properties.end(),
      [&property_name](const ExtensionProperty &other) { return property_name == other.label(); });
    if (iter == _properties.end())
    {
      continue;
    }

    iter->update(prop);
  }
}


bool Extension::hasProperty(const std::string &key) const
{
  for (auto iter = _properties.begin(); iter != _properties.end(); ++iter)
  {
    if (iter->label() == key)
    {
      return true;
    }
  }
  return false;
}


const ExtensionProperty &Extension::operator[](const std::string &key) const
{
  for (auto iter = _properties.begin(); iter != _properties.end(); ++iter)
  {
    if (iter->label() == key)
    {
      return *iter;
    }
  }

  throw std::runtime_error("Unknown property key: " + key);
}


ExtensionProperty &Extension::operator[](const std::string &key)
{
  for (auto iter = _properties.begin(); iter != _properties.end(); ++iter)
  {
    if (iter->label() == key)
    {
      return *iter;
    }
  }

  throw std::runtime_error("Unknown property key: " + key);
}
}  // namespace tes::view::settings
