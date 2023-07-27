//
// author: Kazys Stepanas
//
//
#include "Feature.h"

namespace tes
{
uint64_t featureFlag(Feature feature)
{
  return uint64_t(1ull) << static_cast<uint64_t>(feature);
}


Feature featureForFlag(uint64_t flag)
{
  // Simple solution for now.
  uint64_t bit = 1u;
  for (unsigned i = 0; i < static_cast<unsigned>(Feature::End); ++i, bit = bit << 1u)
  {
    if (flag & bit)
    {
      return static_cast<Feature>(i);
    }
  }

  return Feature::Invalid;
}

bool checkFeature(Feature feature)
{
  return checkFeatureFlag(featureFlag(feature));
}


bool checkFeatureFlag(uint64_t feature_flag)
{
  switch (feature_flag)
  {
  case (1ull << static_cast<unsigned>(Feature::Compression)):
#ifdef TES_ZLIB
    return true;
#endif  // TES_ZLIB
    break;

  default:
    break;
  }

  return false;
}


bool checkFeatures(uint64_t feature_flags)
{
  uint64_t bit = 1u;
  for (unsigned i = 0; i < static_cast<unsigned>(Feature::End) && feature_flags != 0ull;
       ++i, bit = bit << 1u)
  {
    if (feature_flags & bit)
    {
      if (!checkFeatureFlag(bit))
      {
        return false;
      }
    }
    feature_flags &= ~bit;
  }

  return true;
}
}  // namespace tes
