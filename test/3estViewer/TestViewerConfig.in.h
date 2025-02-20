#pragma once

#include <3escore/CoreConfig.h>

#include <gtest/gtest.h>

#include <Magnum/Magnum.h>
#include <Magnum/Platform/@WINDOWLESS_APP@.h>

#define TES_MAGNUM_WINDOWLESS_APP Magnum::Platform::@WINDOWLESS_APP@

#cmakedefine01 TES_DISABLE_PAINTER_TESTS
