#pragma once
#include <cstdint>

namespace Engine
{
    namespace Input
    {
        enum MouseButton : uint8_t
        {
            TR_MOUSE_UNKNOWN = 0,
            TR_MOUSE_LEFT,
            TR_MOUSE_RIGHT,
            TR_MOUSE_MIDDLE,
            TR_MOUSE_4,
            TR_MOUSE_5,
            TR_MOUSE_6,
            TR_MOUSE_7,
            TR_MOUSE_8
        };

        MouseButton FromGLFWMouseButton(int glfwButton);
    }
}