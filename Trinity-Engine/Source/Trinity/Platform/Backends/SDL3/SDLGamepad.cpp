#include <Trinity/Platform/Backends/SDL3/SDLGamepad.h>

#include <algorithm>

#include <SDL3/SDL_gamepad.h>

#include <Trinity/Core/Log.h>

namespace Trinity
{
    SDLGamepad::~SDLGamepad()
    {
        CloseAll();
    }

    void SDLGamepad::HandleDeviceAdded(uint32_t instanceID)
    {
        for (const Slot& l_Slot : m_Slots)
        {
            if (l_Slot.Connected && l_Slot.InstanceID == instanceID)
            {
                return;
            }
        }

        SDL_Gamepad* l_Handle = SDL_OpenGamepad(instanceID);
        if (l_Handle == nullptr)
        {
            ("SDLGamepad: failed to open gamepad {}", instanceID);
            return;
        }

        for (uint32_t l_Index = 0; l_Index < MaxGamepads; l_Index++)
        {
            Slot& l_Slot = m_Slots[l_Index];
            if (!l_Slot.Connected)
            {
                l_Slot.Handle = l_Handle;
                l_Slot.InstanceID = instanceID;
                l_Slot.Connected = true;
                
                ("SDLGamepad: connected '{}' at slot {}", SDL_GetGamepadName(l_Handle), l_Index);
                
                return;
            }
        }

        ("SDLGamepad: no free slot for gamepad {}", instanceID);
        SDL_CloseGamepad(l_Handle);
    }

    void SDLGamepad::HandleDeviceRemoved(uint32_t instanceID)
    {
        for (uint32_t l_Index = 0; l_Index < MaxGamepads; l_Index++)
        {
            Slot& l_Slot = m_Slots[l_Index];
            if (l_Slot.Connected && l_Slot.InstanceID == instanceID)
            {
                SDL_CloseGamepad(l_Slot.Handle);
                l_Slot.Handle = nullptr;
                l_Slot.InstanceID = 0;
                l_Slot.Connected = false;
                
                ("SDLGamepad: disconnected slot {}", l_Index);

                return;
            }
        }
    }

    void SDLGamepad::CloseAll()
    {
        for (Slot& l_Slot : m_Slots)
        {
            if (l_Slot.Connected && l_Slot.Handle != nullptr)
            {
                SDL_CloseGamepad(l_Slot.Handle);
            }

            l_Slot.Handle = nullptr;
            l_Slot.InstanceID = 0;
            l_Slot.Connected = false;
        }
    }

    bool SDLGamepad::IsConnected(uint32_t index) const
    {
        if (index >= MaxGamepads)
        {
            return false;
        }

        return m_Slots[index].Connected;
    }

    std::string SDLGamepad::GetName(uint32_t index) const
    {
        if (index >= MaxGamepads || !m_Slots[index].Connected)
        {
            return {};
        }

        const char* l_Name = SDL_GetGamepadName(m_Slots[index].Handle);

        return l_Name != nullptr ? std::string(l_Name) : std::string{};
    }

    bool SDLGamepad::IsButtonPressed(uint32_t index, GamepadButton button) const
    {
        if (index >= MaxGamepads || !m_Slots[index].Connected)
        {
            return false;
        }

        if (button >= GamepadCode::ButtonCount)
        {
            return false;
        }

        return SDL_GetGamepadButton(m_Slots[index].Handle, static_cast<SDL_GamepadButton>(button));
    }

    float SDLGamepad::GetAxis(uint32_t index, GamepadAxis axis) const
    {
        if (index >= MaxGamepads || !m_Slots[index].Connected)
        {
            return 0.0f;
        }

        if (axis >= GamepadCode::AxisCount)
        {
            return 0.0f;
        }

        int16_t l_Raw = SDL_GetGamepadAxis(m_Slots[index].Handle, static_cast<SDL_GamepadAxis>(axis));
        float l_Value = static_cast<float>(l_Raw) / 32767.0f;

        return std::clamp(l_Value, -1.0f, 1.0f);
    }
}