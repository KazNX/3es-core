//
// Author: Kazys Stepanas
//
#include "Extension.h"

#include "private/Yaml.h"

#include <iostream>

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

#define GET_PROPERTY_IMPL(Type)                                             \
  template <>                                                               \
  Type *ExtensionProperty::getProperty<Type>()                              \
  {                                                                         \
    if (_affordances && _affordances->type() == detail::PropertyType::Type) \
    {                                                                       \
      return reinterpret_cast<Type *>(_affordances->property());            \
    }                                                                       \
    return nullptr;                                                         \
  }                                                                         \
  template <>                                                               \
  const Type *ExtensionProperty::getProperty<Type>() const                  \
  {                                                                         \
    if (_affordances && _affordances->type() == detail::PropertyType::Type) \
    {                                                                       \
      return reinterpret_cast<const Type *>(_affordances->property());      \
    }                                                                       \
    return nullptr;                                                         \
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


GET_PROPERTY_IMPL(Bool);
GET_PROPERTY_IMPL(Colour);
GET_PROPERTY_IMPL(Enum);
GET_PROPERTY_IMPL(Int);
GET_PROPERTY_IMPL(UInt);
GET_PROPERTY_IMPL(Float);
GET_PROPERTY_IMPL(Double);

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
}  // namespace tes::view::settings
