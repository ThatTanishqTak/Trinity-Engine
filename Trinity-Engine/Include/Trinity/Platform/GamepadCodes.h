#pragma once

#include <cstdint>

namespace Trinity
{
    using GamepadButton = uint8_t;
    using GamepadAxis = uint8_t;

    namespace GamepadCode
    {
        enum Button : GamepadButton
        {
            ButtonSouth = 0,
            ButtonEast = 1,
            ButtonWest = 2,
            ButtonNorth = 3,

            ButtonBack = 4,
            ButtonGuide = 5,
            ButtonStart = 6,

            ButtonLeftStick = 7,
            ButtonRightStick = 8,
            ButtonLeftShoulder = 9,
            ButtonRightShoulder = 10,

            ButtonDpadUp = 11,
            ButtonDpadDown = 12,
            ButtonDpadLeft = 13,
            ButtonDpadRight = 14,

            ButtonCount = 15
        };

        enum Axis : GamepadAxis
        {
            AxisLeftX = 0,
            AxisLeftY = 1,
            AxisRightX = 2,
            AxisRightY = 3,
            AxisLeftTrigger = 4,
            AxisRightTrigger = 5,

            AxisCount = 6
        };
    }
}