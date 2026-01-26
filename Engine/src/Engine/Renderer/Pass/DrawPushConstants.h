#pragma once

#include <cstdint>

namespace Engine
{
    struct DrawPushConstant
    {
        uint32_t TransformIndex = 0;
    };
}