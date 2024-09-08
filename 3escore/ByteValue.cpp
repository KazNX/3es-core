//
// author: Kazys Stepanas
//
#include "ByteValue.h"

#include <array>

namespace tes
{
namespace
{
constexpr size_t kConversion = 1024u;
constexpr std::array<size_t, 8> kConversions = {
  1,
  kConversion,
  kConversion *kConversion,
  kConversion *kConversion *kConversion,
  kConversion *kConversion *kConversion *kConversion,
  kConversion *kConversion *kConversion *kConversion *kConversion,
  kConversion *kConversion *kConversion *kConversion *kConversion *kConversion,
};
}  // namespace

std::string_view ByteValue::unitSuffix(ByteUnit unit)
{
  static std::array<std::string_view, 8> names = {
    "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB",
  };
  return names[static_cast<size_t>(unit)];
}


uint64_t ByteValue::conversion(ByteUnit unit)
{
  return kConversions[static_cast<size_t>(unit)];
}


uint64_t ByteValue::fractionalConversion(ByteUnit unit)
{
  return (unit != ByteUnit::Bytes) ? kConversions[static_cast<size_t>(unit) - 1] : 0u;
}
}  // namespace tes
