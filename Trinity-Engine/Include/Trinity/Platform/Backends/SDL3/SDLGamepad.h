#pragma once

#include <array>
#include <string>
#include <cstdint>

#include <Trinity/Platform/Gamepad.h>

struct SDL_Gamepad;

namespace Trinity
{
    class SDLGamepad : public Gamepad
    {
    public:
        ~SDLGamepad() override;

        bool IsConnected(uint32_t index) const override;
        std::string GetName(uint32_t index) const override;

        bool IsButtonPressed(uint32_t index, GamepadButton button) const override;
        float GetAxis(uint32_t index, GamepadAxis acis) const override;

        void HandleDeviceAdded(uint32_t instanceID);
        void HandleDeviceRemoved(uint32_t instanceID);
        void CloseAll();

    private:
        struct Slot
        {
            SDL_Gamepad* Handle = nullptr;
            
            uint32_t InstanceID = 0;
            
            bool Connected = false;
        };

        std::array<Slot, MaxGamepads> m_Slots;
    };
}