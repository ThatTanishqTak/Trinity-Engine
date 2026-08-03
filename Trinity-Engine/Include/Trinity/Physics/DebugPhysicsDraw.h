#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

namespace Trinity
{
    // Backend-agnostic debug geometry. Any subsystem may fill a DebugDrawBuffer and hand it to the renderer; this header must stay free of renderer and physics-backend types so headless targets can use it
    struct DebugLine
    {
        glm::vec3 Start{ 0.0f };
        glm::vec3 End{ 0.0f };
        uint32_t Color = 0xFFFFFFFF;   // packed R | G << 8 | B << 16 | A << 24
    };

    struct DebugDrawBuffer
    {
        std::vector<DebugLine> Lines;

        void Clear()
        {
            Lines.clear();
        }

        void AddLine(const glm::vec3& start, const glm::vec3& end, uint32_t color)
        {
            Lines.push_back(DebugLine{ start, end, color });
        }
    };
}