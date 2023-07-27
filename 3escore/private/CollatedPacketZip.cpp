#include "CollatedPacketZip.h"

namespace tes
{
const std::array<int, static_cast<unsigned>(CompressionLevel::Levels)>
  kTesToGZipCompressionLevel = {
    0,  // ClNone,
    3,  // ClLow,
    5,  // ClMedium
    7,  // ClHigh,
#ifdef TES_ZLIB
    Z_BEST_COMPRESSION  // ClVeryHigh (9)
#else                   // TES_ZLIB
    9
#endif                  // TES_ZLIB
  };

#ifdef TES_ZLIB
const int CollatedPacketZip::DefaultCompressionLevel =
  tes::kTesToGZipCompressionLevel[static_cast<unsigned>(CompressionLevel::Default)];
#endif  // TES_ZLIB
}  // namespace tes
