#include "Engine/Events/Input/GamepadCodes.h"

#include <GLFW/glfw3.h>

namespace Engine
{
    namespace Input
    {
        GamepadButton FromGLFWGamepadButton(int b)
        {
            switch (b)
            {
            case GLFW_GAMEPAD_BUTTON_A: return TR_PAD_A;
            case GLFW_GAMEPAD_BUTTON_B: return TR_PAD_B;
            case GLFW_GAMEPAD_BUTTON_X: return TR_PAD_X;
            case GLFW_GAMEPAD_BUTTON_Y: return TR_PAD_Y;
            case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER: return TR_PAD_LB;
            case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER: return TR_PAD_RB;
            case GLFW_GAMEPAD_BUTTON_BACK: return TR_PAD_BACK;
            case GLFW_GAMEPAD_BUTTON_START: return TR_PAD_START;
            case GLFW_GAMEPAD_BUTTON_GUIDE: return TR_PAD_GUIDE;
            case GLFW_GAMEPAD_BUTTON_LEFT_THUMB: return TR_PAD_LS;
            case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB: return TR_PAD_RS;
            case GLFW_GAMEPAD_BUTTON_DPAD_UP: return TR_PAD_DPAD_UP;
            case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT: return TR_PAD_DPAD_RIGHT;
            case GLFW_GAMEPAD_BUTTON_DPAD_DOWN: return TR_PAD_DPAD_DOWN;
            case GLFW_GAMEPAD_BUTTON_DPAD_LEFT: return TR_PAD_DPAD_LEFT;
            default: return TR_PAD_BTN_UNKNOWN;
            }
        }

        GamepadAxis FromGLFWGamepadAxis(int a)
        {
            switch (a)
            {
            case GLFW_GAMEPAD_AXIS_LEFT_X: return TR_PAD_LX;
            case GLFW_GAMEPAD_AXIS_LEFT_Y: return TR_PAD_LY;
            case GLFW_GAMEPAD_AXIS_RIGHT_X: return TR_PAD_RX;
            case GLFW_GAMEPAD_AXIS_RIGHT_Y: return TR_PAD_RY;
            case GLFW_GAMEPAD_AXIS_LEFT_TRIGGER: return TR_PAD_LT;
            case GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER: return TR_PAD_RT;
            default: return TR_PAD_AXIS_UNKNOWN;
            }
        }
    }
}