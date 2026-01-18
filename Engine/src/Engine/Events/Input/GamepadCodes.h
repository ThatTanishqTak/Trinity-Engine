#pragma once
#include <cstdint>

namespace Engine
{
    namespace Input
    {
        enum GamepadButton : uint8_t
        {
            TR_PAD_BTN_UNKNOWN = 0,

            TR_PAD_A, TR_PAD_B, TR_PAD_X, TR_PAD_Y,
            TR_PAD_LB, TR_PAD_RB,
            TR_PAD_BACK, TR_PAD_START, TR_PAD_GUIDE,
            TR_PAD_LS, TR_PAD_RS,
            TR_PAD_DPAD_UP, TR_PAD_DPAD_RIGHT, TR_PAD_DPAD_DOWN, TR_PAD_DPAD_LEFT
        };

        enum GamepadAxis : uint8_t
        {
            TR_PAD_AXIS_UNKNOWN = 0,
            TR_PAD_LX, TR_PAD_LY, TR_PAD_RX, TR_PAD_RY,
            TR_PAD_LT, TR_PAD_RT
        };

        GamepadButton FromGLFWGamepadButton(int glfwButton);
        GamepadAxis   FromGLFWGamepadAxis(int glfwAxis);
    }
}