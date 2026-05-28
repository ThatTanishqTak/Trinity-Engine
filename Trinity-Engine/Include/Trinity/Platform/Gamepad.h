#pragma once

#include <string>
#include <cstdint>

#include <Trinity/Platform/GamepadCodes.h>

namespace Trinity
{
    class Gamepad
    {
    public:
        static constexpr uint32_t MaxGamepads = 8;

        virtual ~Gamepad() = default;

        virtual bool IsConnected(uint32_t index) const = 0;
        virtual std::string GetName(uint32_t index) const = 0;

        virtual bool IsButtonPressed(uint32_t index, GamepadButton button) const = 0;
        virtual float GetAxis(uint32_t index, GamepadAxis axis) const = 0;
    };
}