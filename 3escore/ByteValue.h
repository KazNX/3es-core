//
// author: Kazys Stepanas
//
#ifndef TES_CORE_BYTE_VALUE_H
#define TES_CORE_BYTE_VALUE_H

#include "CoreConfig.h"

#include <iostream>
#include <string>

namespace tes
{
enum class ByteUnit
{
  Bytes,
  KibiBytes,
  MebiBytes,
  GibiBytes,
  TebiBytes,
  PetiBytes,
  ExiBytes,
};

class TES_CORE_API ByteValue
{
public:
  ByteValue() = default;
  explicit ByteValue(uint64_t value, ByteUnit unit = ByteUnit::Bytes)
    : _value(value)
    , _fractional(0.0)
    , _unit(unit)
  {}
  ByteValue(uint64_t value, double fractional, ByteUnit unit)
    : _value(value)
    , _fractional(fractional)
    , _unit(unit)
  {}
  ByteValue(const ByteValue &other) = default;

  ~ByteValue() = default;

  ByteValue &operator=(const ByteValue &other) = default;

  uint64_t bytes() const;
  uint64_t value() const { return _value; }
  double fractional() const { return _fractional; }
  unsigned remainder() const { return static_cast<unsigned>(_fractional * 1000.0); }
  ByteUnit unit() const { return _unit; }

  ByteValue succinct() const;
  ByteValue as(ByteUnit unit) const;

  std::string_view unitSuffix() const { return unitSuffix(_unit); }
  static std::string_view unitSuffix(ByteUnit unit);

  static uint64_t conversion(ByteUnit unit);
  static uint64_t fractionalConversion(ByteUnit unit);

private:
  uint64_t _value = 0;
  double _fractional = 0;
  ByteUnit _unit = ByteUnit::Bytes;
};


inline uint64_t ByteValue::bytes() const
{
  return _value * conversion(_unit) +
         static_cast<uint64_t>(_fractional * fractionalConversion(_unit));
}


inline ByteValue ByteValue::succinct() const
{
  const uint64_t byte = this->bytes();

  uint64_t divisor = 1;
  auto unit = ByteUnit::Bytes;
  if (byte >= conversion(ByteUnit::ExiBytes))
  {
    divisor = conversion(ByteUnit::ExiBytes);
    unit = ByteUnit::ExiBytes;
  }
  else if (byte >= conversion(ByteUnit::PetiBytes))
  {
    divisor = conversion(ByteUnit::PetiBytes);
    unit = ByteUnit::PetiBytes;
  }
  else if (byte >= conversion(ByteUnit::TebiBytes))
  {
    divisor = conversion(ByteUnit::TebiBytes);
    unit = ByteUnit::TebiBytes;
  }
  else if (byte >= conversion(ByteUnit::GibiBytes))
  {
    divisor = conversion(ByteUnit::GibiBytes);
    unit = ByteUnit::GibiBytes;
  }
  else if (byte >= conversion(ByteUnit::MebiBytes))
  {
    divisor = conversion(ByteUnit::MebiBytes);
    unit = ByteUnit::MebiBytes;
  }
  else if (byte >= conversion(ByteUnit::KibiBytes))
  {
    divisor = conversion(ByteUnit::KibiBytes);
    unit = ByteUnit::KibiBytes;
  }
  else
  {
    return *this;
  }

  const uint64_t scaled = byte / divisor;
  const double fractional =
    static_cast<double>(byte - scaled * divisor) / static_cast<double>(divisor);

  return { scaled, fractional, unit };
}

inline ByteValue ByteValue::as(ByteUnit unit) const
{
  const uint64_t bytes = this->bytes();
  const uint64_t divisor = conversion(unit);
  const uint64_t scaled = bytes / divisor;
  const double fractional =
    static_cast<double>(bytes - scaled * divisor) / static_cast<double>(divisor);

  return { scaled, fractional, unit };
}

inline std::istream &operator>>(std::istream &in, ByteUnit &unit)
{
  std::string str_unit;
  in >> str_unit;
  for (int i = static_cast<int>(ByteUnit::Bytes); i <= static_cast<int>(ByteUnit::ExiBytes); ++i)
  {
    if (str_unit == ByteValue::unitSuffix(static_cast<ByteUnit>(i)))
    {
      unit = static_cast<ByteUnit>(i);
      return in;
    }
  }

  in.setstate(std::ios_base::failbit);
  return in;
}

inline std::ostream &operator<<(std::ostream &out, const ByteUnit unit)
{
  out << ByteValue::unitSuffix(unit);
  return out;
}

inline std::ostream &operator<<(std::ostream &out, const ByteValue &bytes)
{
  out << bytes.value();
  if (const auto remainder = bytes.fractional())
  {
    const auto w = out.width();
    const auto f = out.fill();
    const auto integer_remainder = static_cast<int>(remainder * 1000);
    if (integer_remainder)
    {
      out << '.';
      out.width(3);
      out.fill('0');
      out << integer_remainder;
      out.width(w);
      out.fill(f);
    }
  }
  out << bytes.unit();
  return out;
}
}  // namespace tes

#endif  // TES_CORE_BYTE_VALUE_H