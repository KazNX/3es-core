//
// author: Kazys Stepanas
//
#include "3estViewer/TestViewerConfig.h"

#include "TestViewer.h"

#include <3esview/settings/Settings.h>

#include <array>
#include <string>

namespace tes::view::settings
{
template <typename T>
struct ExtendedPropertyTester
{
  void testExtended([[maybe_unused]] T &property) const {}
};

template <typename T>
struct PropertyComparer
{
  static void compare(const T &a, const T &b)
  {
    EXPECT_EQ(a.label(), b.label());
    EXPECT_EQ(a.tip(), b.tip());
    EXPECT_EQ(a.value(), b.value());
  }
};

template <typename T>
struct PropertyTester
{
  PropertyTester(T &&property)
    : _property(std::move(property))
  {}

  void test()
  {
    testCommon();
    testExtended();
  }

protected:
  static T copyAndMove(const T &property)
  {
    T copy = property;
    return std::move(copy);
  }

  void testCommon()
  {
    // Start by duplicating the property.
    T copy = _property;
    compare(copy, _property);
    copy = copyAndMove(_property);
    compare(copy, _property);
  }

  void testExtended() { _extended.testExtended(_property); }

  void compare(const T &a, const T &b) { PropertyComparer<T>::compare(a, b); }

  T _property;
  ExtendedPropertyTester<T> _extended = {};
};

template <typename T>
struct PropertyComparer<Numeric<T>>
{
  static void compare(const Numeric<T> &a, const Numeric<T> &b)
  {
    EXPECT_EQ(a.label(), b.label());
    EXPECT_EQ(a.tip(), b.tip());
    EXPECT_EQ(a.value(), b.value());
    EXPECT_EQ(a.hasMinimum(), b.hasMinimum());
    EXPECT_EQ(a.hasMaximum(), b.hasMaximum());
    if (a.hasMinimum() && b.hasMinimum())
    {
      EXPECT_EQ(a.minimum(), b.minimum());
    }
    if (a.hasMaximum() && b.hasMaximum())
    {
      EXPECT_EQ(a.maximum(), b.maximum());
    }
  }
};

template <typename T>
struct ExtendedPropertyTester<Numeric<T>>
{
  void testExtended(Numeric<T> &property) const
  {
    auto copy = property;
    const auto modified_value = property.value() + T{ 1 };
    copy.setValue(modified_value);
    EXPECT_EQ(copy.value(), modified_value);
    if (property.hasMinimum())
    {
      copy.setValue(copy.minimum());
      EXPECT_EQ(copy.value(), property.minimum());
    }
    if (property.hasMaximum())
    {
      copy.setValue(copy.maximum());
      EXPECT_EQ(copy.value(), property.maximum());
    }
  }
};

template <>
struct ExtendedPropertyTester<Bool>
{
  void testExtended(Bool &property) const
  {
    auto copy = property;
    copy.setValue(true);
    EXPECT_TRUE(copy.value());
    copy.setValue(false);
    EXPECT_FALSE(copy.value());
  }
};

template <>
struct PropertyComparer<Enum>
{
  static void compare(const Enum &a, const Enum &b)
  {
    EXPECT_EQ(a.label(), b.label());
    EXPECT_EQ(a.tip(), b.tip());
    EXPECT_EQ(a.value(), b.value());
    EXPECT_EQ(a.valueName(), b.valueName());
    EXPECT_EQ(a.namedValues().size(), b.namedValues().size());
    const auto limit = std::min(a.namedValues().size(), b.namedValues().size());
    for (size_t i = 0; i < limit; ++i)
    {
      EXPECT_EQ(a.namedValues()[i], b.namedValues()[i]);
    }
  }
};

TEST(Settings, PropertiesBool)
{
  PropertyTester<Bool>({ "bool", false, "a boolean property" }).test();
  PropertyTester<Bool>({ "bool", true, "a boolean property" }).test();
}

TEST(Settings, PropertiesNumeric)
{
  // Test numeric properties with/without min/max limits.
  // Int.
  PropertyTester<Int>({ "int", -6, "an int property" }).test();
  PropertyTester<Int>({ "int", -6, -10, "an int property" }).test();
  PropertyTester<Int>({ "int", -6, "an int property", 10 }).test();
  PropertyTester<Int>({ "int", -6, -10, 10, "an int property" }).test();
  // UInt
  PropertyTester<UInt>({ "uint", 42u, "a uint property" }).test();
  PropertyTester<UInt>({ "uint", 42u, 1, "a uint property" }).test();
  PropertyTester<UInt>({ "uint", 42u, "a uint property", 100 }).test();
  PropertyTester<UInt>({ "uint", 42u, 0, 100, "a uint property" }).test();
  // Float
  PropertyTester<Float>({ "float", 3.141f, "a float property" }).test();
  PropertyTester<Float>({ "float", 3.141f, -1.0f, "a float property" }).test();
  PropertyTester<Float>({ "float", 3.141f, "a float property", 6.0f }).test();
  PropertyTester<Float>({ "float", 3.141f, 0.0f, 6.0f, "a float property" }).test();
  // Double
  PropertyTester<Double>({ "double", -2.76, "a double property" }).test();
  PropertyTester<Double>({ "double", -2.76, -5.0, "a double property" }).test();
  PropertyTester<Double>({ "double", -2.76, "a double property", 5.0 }).test();
  PropertyTester<Double>({ "double", -2.76, -5.0, 5.0, "a double property" }).test();
}

TEST(Settings, PropertiesEnum)
{
  enum class Value
  {
    Zero,
    One,
    Two,
    Many = 10,
    Lots = 100
  };
  PropertyTester<Enum>({ "enum",
                         Value::One,
                         "troll counting",
                         {
                           { Value::Zero, "zero" },
                           { Value::One, "one" },
                           { Value::Two, "two" },
                           { Value::Many, "many" },
                           { Value::Lots, "lots" },
                         } })
    .test();
}
}  // namespace tes::view::settings
