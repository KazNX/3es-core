//
// author: Kazys Stepanas
//
#pragma once

#include "CoreConfig.h"

#include <cstdint>

namespace tes
{
/// Calculate an 8-bit CRC value.
/// @param message The buffer to operate on.
/// @param byte_count The number of bytes in @p message.
/// @return An 8-bit CRC for @c message.
uint8_t TES_CORE_API crc8(const uint8_t *message, size_t byte_count);

/// Calculate an 16-bit CRC value.
/// @param message The buffer to operate on.
/// @param byte_count The number of bytes in @p message.
/// @return An 16-bit CRC for @c message.
uint16_t TES_CORE_API crc16(const uint8_t *message, size_t byte_count);

/// Calculate an 32-bit CRC value.
/// @param message The buffer to operate on.
/// @param byte_count The number of bytes in @p message.
/// @return An 32-bit CRC for @c message.
uint32_t TES_CORE_API crc32(const uint8_t *message, size_t byte_count);
}  // namespace tes
