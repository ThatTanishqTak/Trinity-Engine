#include "Engine/Events/Input/MouseCodes.h"

#include <GLFW/glfw3.h>

namespace Engine
{
    namespace Input
    {
        MouseButton FromGLFWMouseButton(int b)
        {
            switch (b)
            {
            case GLFW_MOUSE_BUTTON_LEFT: return TR_MOUSE_LEFT;
            case GLFW_MOUSE_BUTTON_RIGHT: return TR_MOUSE_RIGHT;
            case GLFW_MOUSE_BUTTON_MIDDLE: return TR_MOUSE_MIDDLE;
            case GLFW_MOUSE_BUTTON_4: return TR_MOUSE_4;
            case GLFW_MOUSE_BUTTON_5: return TR_MOUSE_5;
            case GLFW_MOUSE_BUTTON_6: return TR_MOUSE_6;
            case GLFW_MOUSE_BUTTON_7: return TR_MOUSE_7;
            case GLFW_MOUSE_BUTTON_8: return TR_MOUSE_8;
            default: return TR_MOUSE_UNKNOWN;
            }
        }
    }
}