#pragma once
#include <gtest/gtest.h>
#include "DebugAllocator.hpp"
extern "C" {
#include "../include/upng.h"
}

static bool operator == (const upng_rect& a, const upng_rect& b)
{
    return
        a.x_offset == b.x_offset &&
        a.y_offset == b.y_offset &&
        a.width == b.width && 
        a.height == b.height;
}
