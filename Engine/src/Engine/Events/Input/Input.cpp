#include "Engine/Events/Input/Input.h"

#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/GamepadEvent.h"
#include "Engine/Events/WindowEvent.h"

#include <array>
#include <algorithm>

namespace Engine
{
    namespace Input
    {
        static bool s_Initialized = false;
        static bool s_Blocked = false;

        // Sizes derived from the last enum value (+1). Assumes contiguous enums
        static constexpr size_t s_KeyCount = static_cast<size_t>(TR_KEY_MENU) + 1;
        static constexpr size_t s_MouseCount = static_cast<size_t>(TR_MOUSE_8) + 1;

        static constexpr int32_t s_MaxGamepads = 16;
        static constexpr size_t s_GamepadButtonCount = static_cast<size_t>(TR_PAD_DPAD_LEFT) + 1;
        static constexpr size_t s_GamepadAxisCount = static_cast<size_t>(TR_PAD_RT) + 1;

        static std::array<uint8_t, s_KeyCount> s_KeyDown{};
        static std::array<uint8_t, s_KeyCount> s_KeyPressed{};
        static std::array<uint8_t, s_KeyCount> s_KeyReleased{};

        static std::array<uint8_t, s_MouseCount> s_MouseDown{};
        static std::array<uint8_t, s_MouseCount> s_MousePressed{};
        static std::array<uint8_t, s_MouseCount> s_MouseReleased{};

        static double s_MouseX = 0.0;
        static double s_MouseY = 0.0;
        static double s_MouseDeltaX = 0.0;
        static double s_MouseDeltaY = 0.0;
        static bool s_MousePosValid = false;

        static double s_ScrollX = 0.0;
        static double s_ScrollY = 0.0;

        static uint16_t s_Mods = Mod_None;
        static std::vector<uint32_t> s_TypedChars;

        static std::array<uint8_t, s_MaxGamepads> s_GamepadConnected{};
        static std::array<std::array<uint8_t, s_GamepadButtonCount>, s_MaxGamepads> s_GamepadDown{};
        static std::array<std::array<uint8_t, s_GamepadButtonCount>, s_MaxGamepads> s_GamepadPressed{};
        static std::array<std::array<uint8_t, s_GamepadButtonCount>, s_MaxGamepads> s_GamepadReleased{};
        static std::array<std::array<float, s_GamepadAxisCount>, s_MaxGamepads> s_GamepadAxes{};

        static size_t ToIndex(KeyCode key)
        {
            return static_cast<size_t>(key);
        }
        
        static size_t ToIndex(MouseButton button)
        {
            return static_cast<size_t>(button);
        }

        static size_t ToIndex(GamepadButton button)
        {
            return static_cast<size_t>(button);
        }

        static size_t ToIndex(GamepadAxis axis)
        {
            return static_cast<size_t>(axis);
        }

        static bool ValidGamepad(int32_t ID)
        {
            return ID >= 0 && ID < s_MaxGamepads;
        }

        void Initialize()
        {
            if (s_Initialized)
            {
                return;
            }

            std::fill(s_KeyDown.begin(), s_KeyDown.end(), 0);
            std::fill(s_KeyPressed.begin(), s_KeyPressed.end(), 0);
            std::fill(s_KeyReleased.begin(), s_KeyReleased.end(), 0);

            std::fill(s_MouseDown.begin(), s_MouseDown.end(), 0);
            std::fill(s_MousePressed.begin(), s_MousePressed.end(), 0);
            std::fill(s_MouseReleased.begin(), s_MouseReleased.end(), 0);

            std::fill(s_GamepadConnected.begin(), s_GamepadConnected.end(), 0);
            for (auto& it_Gamepad : s_GamepadDown)
            {
                std::fill(it_Gamepad.begin(), it_Gamepad.end(), 0);
            }

            for (auto& it_Gamepad : s_GamepadPressed)
            {
                std::fill(it_Gamepad.begin(), it_Gamepad.end(), 0);
            }

            for (auto& it_Gamepad : s_GamepadReleased)
            {
                std::fill(it_Gamepad.begin(), it_Gamepad.end(), 0);
            }

            for (auto& it_Gamepad : s_GamepadAxes)
            {
                std::fill(it_Gamepad.begin(), it_Gamepad.end(), 0.0f);
            }

            s_MousePosValid = false;
            s_MouseDeltaX = s_MouseDeltaY = 0.0;
            s_ScrollX = s_ScrollY = 0.0;
            s_Mods = Mod_None;
            s_TypedChars.clear();

            s_Initialized = true;
        }

        void Shutdown()
        {
            s_Initialized = false;
            s_TypedChars.clear();
        }

        void BeginFrame()
        {
            if (!s_Initialized)
            {
                return;
            }

            std::fill(s_KeyPressed.begin(), s_KeyPressed.end(), 0);
            std::fill(s_KeyReleased.begin(), s_KeyReleased.end(), 0);

            std::fill(s_MousePressed.begin(), s_MousePressed.end(), 0);
            std::fill(s_MouseReleased.begin(), s_MouseReleased.end(), 0);

            for (auto& it_Gamepad : s_GamepadPressed)
            {
                std::fill(it_Gamepad.begin(), it_Gamepad.end(), 0);
            }

            for (auto& it_Gamepad : s_GamepadReleased)
            {
                std::fill(it_Gamepad.begin(), it_Gamepad.end(), 0);
            }

            s_MouseDeltaX = 0.0;
            s_MouseDeltaY = 0.0;
            s_ScrollX = 0.0;
            s_ScrollY = 0.0;

            s_TypedChars.clear();
        }

        void SetBlocked(bool blocked)
        {
            s_Blocked = blocked;
            if (s_Blocked)
            {
                std::fill(s_KeyPressed.begin(), s_KeyPressed.end(), 0);
                std::fill(s_KeyReleased.begin(), s_KeyReleased.end(), 0);
                std::fill(s_MousePressed.begin(), s_MousePressed.end(), 0);
                std::fill(s_MouseReleased.begin(), s_MouseReleased.end(), 0);

                s_TypedChars.clear();
                s_ScrollX = s_ScrollY = 0.0;
                s_MouseDeltaX = s_MouseDeltaY = 0.0;
            }
        }

        bool IsBlocked()
        {
            return s_Blocked;
        }

        void OnEvent(Engine::Event& e)
        {
            if (!s_Initialized)
            {
                return;
            }

            switch (e.GetEventType())
            {
            case EventType::WindowLostFocus:
            {
                std::fill(s_KeyDown.begin(), s_KeyDown.end(), 0);
                std::fill(s_MouseDown.begin(), s_MouseDown.end(), 0);

                for (auto& it_Gamepad : s_GamepadDown)
                {
                    std::fill(it_Gamepad.begin(), it_Gamepad.end(), 0);
                }

                s_Mods = Mod_None;

                return;
            }

            case EventType::KeyPressed:
            {
                auto& a_Event = static_cast<Engine::KeyPressedEvent&>(e);
                const size_t l_Index = ToIndex(a_Event.GetKeyCode());

                if (l_Index < s_KeyCount)
                {
                    s_KeyDown[l_Index] = 1;

                    if (!a_Event.IsRepeat())
                    {
                        s_KeyPressed[l_Index] = 1;
                    }
                }

                s_Mods = a_Event.GetMods();

                return;
            }

            case EventType::KeyReleased:
            {
                auto& a_Event = static_cast<Engine::KeyReleasedEvent&>(e);
                const size_t l_Index = ToIndex(a_Event.GetKeyCode());

                if (l_Index < s_KeyCount)
                {
                    s_KeyDown[l_Index] = 0;
                    s_KeyReleased[l_Index] = 1;
                }

                s_Mods = a_Event.GetMods();

                return;
            }

            case EventType::KeyTyped:
            {
                auto& a_Event = static_cast<Engine::KeyTypedEvent&>(e);
                s_TypedChars.emplace_back(a_Event.GetCodepoint());

                return;
            }

            case EventType::MouseMoved:
            {
                auto& a_Event = static_cast<Engine::MouseMovedEvent&>(e);
                const double l_X = a_Event.GetX();
                const double l_Y = a_Event.GetY();

                if (s_MousePosValid)
                {
                    s_MouseDeltaX += (l_X - s_MouseX);
                    s_MouseDeltaY += (l_Y - s_MouseY);
                }

                s_MouseX = l_X;
                s_MouseY = l_Y;
                s_MousePosValid = true;

                return;
            }

            case EventType::MouseScrolled:
            {
                auto& a_Event = static_cast<Engine::MouseScrolledEvent&>(e);
                s_ScrollX += a_Event.GetXOffset();
                s_ScrollY += a_Event.GetYOffset();

                return;
            }

            case EventType::MouseButtonPressed:
            {
                auto& a_Event = static_cast<Engine::MouseButtonPressedEvent&>(e);
                const size_t l_Index = ToIndex(a_Event.GetMouseButton());

                if (l_Index < s_MouseCount)
                {
                    s_MouseDown[l_Index] = 1;
                    s_MousePressed[l_Index] = 1;
                }

                s_Mods = a_Event.GetMods();

                return;
            }

            case EventType::MouseButtonReleased:
            {
                auto& a_Event = static_cast<Engine::MouseButtonReleasedEvent&>(e);
                const size_t l_Index = ToIndex(a_Event.GetMouseButton());

                if (l_Index < s_MouseCount)
                {
                    s_MouseDown[l_Index] = 0;
                    s_MouseReleased[l_Index] = 1;
                }

                s_Mods = a_Event.GetMods();

                return;
            }

            case EventType::GamepadConnected:
            {
                auto& a_Event = static_cast<Engine::GamepadConnectedEvent&>(e);
                if (ValidGamepad(a_Event.GetGamepadId()))
                {
                    s_GamepadConnected[a_Event.GetGamepadId()] = 1;
                }
                
                return;
            }

            case EventType::GamepadDisconnected:
            {
                auto& a_Event = static_cast<Engine::GamepadDisconnectedEvent&>(e);
                if (ValidGamepad(a_Event.GetGamepadId()))
                {
                    const int32_t l_ID = a_Event.GetGamepadId();
                    s_GamepadConnected[l_ID] = 0;

                    std::fill(s_GamepadDown[l_ID].begin(), s_GamepadDown[l_ID].end(), 0);
                    std::fill(s_GamepadPressed[l_ID].begin(), s_GamepadPressed[l_ID].end(), 0);
                    std::fill(s_GamepadReleased[l_ID].begin(), s_GamepadReleased[l_ID].end(), 0);
                    std::fill(s_GamepadAxes[l_ID].begin(), s_GamepadAxes[l_ID].end(), 0.0f);
                }

                return;
            }

            case EventType::GamepadButtonPressed:
            {
                auto& a_Event = static_cast<Engine::GamepadButtonPressedEvent&>(e);
                if (!ValidGamepad(a_Event.GetGamepadId()))
                {
                    return;
                }

                const int32_t l_ID = a_Event.GetGamepadId();
                const size_t l_Index = ToIndex(a_Event.GetButton());

                if (l_Index < s_GamepadButtonCount)
                {
                    s_GamepadDown[l_ID][l_Index] = 1;
                    s_GamepadPressed[l_ID][l_Index] = 1;
                }

                return;
            }

            case EventType::GamepadButtonReleased:
            {
                auto& a_Event = static_cast<Engine::GamepadButtonReleasedEvent&>(e);
                if (!ValidGamepad(a_Event.GetGamepadId()))
                {
                    return;
                }

                const int32_t l_ID = a_Event.GetGamepadId();
                const size_t l_Index = ToIndex(a_Event.GetButton());
                if (l_Index < s_GamepadButtonCount)
                {
                    s_GamepadDown[l_ID][l_Index] = 0;
                    s_GamepadReleased[l_ID][l_Index] = 1;
                }

                return;
            }

            case EventType::GamepadAxisMoved:
            {
                auto& a_Event = static_cast<Engine::GamepadAxisMovedEvent&>(e);
                if (!ValidGamepad(a_Event.GetGamepadId()))
                {
                    return;
                }

                const int32_t l_ID = a_Event.GetGamepadId();
                const size_t l_Index = ToIndex(a_Event.GetAxis());
                if (l_Index < s_GamepadAxisCount)
                {
                    s_GamepadAxes[l_ID][l_Index] = a_Event.GetValue();
                }
                
                return;
            }

            default:
                return;
            }
        }

        bool KeyDown(KeyCode key)
        {
            if (s_Blocked)
            {
                return false;
            }
            
            const size_t l_Index = ToIndex(key);
            
            return l_Index < s_KeyCount && s_KeyDown[l_Index] != 0;
        }

        bool KeyPressed(KeyCode key)
        {
            if (s_Blocked)
            {
                return false;
            }
            
            const size_t l_Index = ToIndex(key);
            
            return l_Index < s_KeyCount && s_KeyPressed[l_Index] != 0;
        }

        bool KeyReleased(KeyCode key)
        {
            if (s_Blocked)
            {
                return false;
            }
            
            const size_t l_Index = ToIndex(key);
            
            return l_Index < s_KeyCount && s_KeyReleased[l_Index] != 0;
        }

        bool MouseDown(MouseButton button)
        {
            if (s_Blocked)
            {
                return false;
            }
            
            const size_t l_Index = ToIndex(button);
            
            return l_Index < s_MouseCount && s_MouseDown[l_Index] != 0;
        }

        bool MousePressed(MouseButton button)
        {
            if (s_Blocked)
            {
                return false;
            }
            
            const size_t l_Index = ToIndex(button);
            
            return l_Index < s_MouseCount && s_MousePressed[l_Index] != 0;
        }

        bool MouseReleased(MouseButton button)
        {
            if (s_Blocked)
            {
                return false;
            }
            
            const size_t l_Index = ToIndex(button);
            
            return l_Index < s_MouseCount && s_MouseReleased[l_Index] != 0;
        }

        double GetMouseX()
        { 
            return s_MouseX; 
        }

        double GetMouseY()
        {
            return s_MouseY;
        }

        double GetMouseDeltaX()
        {
            return s_MouseDeltaX; 
        }

        double GetMouseDeltaY()
        {
            return s_MouseDeltaY;
        }

        double GetScrollX()
        {
            return s_ScrollX;
        }

        double GetScrollY()
        {
            return s_ScrollY;
        }

        const std::vector<uint32_t>& TypedChars()
        {
            static const std::vector<uint32_t> l_Empty;

            return s_Blocked ? l_Empty : s_TypedChars;
        }

        uint16_t Mods()
        {
            return s_Mods;
        }

        bool GamepadConnected(int32_t gamepadID)
        {
            if (s_Blocked)
            {
                return false;
            }
            
            return ValidGamepad(gamepadID) && s_GamepadConnected[gamepadID] != 0;
        }

        bool GamepadButtonDown(int32_t gamepadID, GamepadButton button)
        {
            if (s_Blocked || !ValidGamepad(gamepadID))
            {
                return false;
            }

            const size_t l_Index = ToIndex(button);

            return l_Index < s_GamepadButtonCount && s_GamepadDown[gamepadID][l_Index] != 0;
        }

        bool GamepadButtonPressed(int32_t gamepadID, GamepadButton button)
        {
            if (s_Blocked || !ValidGamepad(gamepadID))
            {
                return false;
            }

            const size_t l_Index = ToIndex(button);
            
            return l_Index < s_GamepadButtonCount && s_GamepadPressed[gamepadID][l_Index] != 0;
        }

        bool GamepadButtonReleased(int32_t gamepadID, GamepadButton button)
        {
            if (s_Blocked || !ValidGamepad(gamepadID))
            {
                return false;
            }

            const size_t l_Index = ToIndex(button);

            return l_Index < s_GamepadButtonCount && s_GamepadReleased[gamepadID][l_Index] != 0;
        }

        float GetGamepadAxis(int32_t gamepadID, GamepadAxis axis)
        {
            if (s_Blocked || !ValidGamepad(gamepadID))
            {
                return 0.0f;
            }

            const size_t l_Index = ToIndex(axis);

            return l_Index < s_GamepadAxisCount ? s_GamepadAxes[gamepadID][l_Index] : 0.0f;
        }
    }
}