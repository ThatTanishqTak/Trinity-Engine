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

        // Sizes derived from the last enum value (+1). Assumes contiguous enums (yours are).
        static constexpr size_t s_KeyCount = static_cast<size_t>(TR_KEY_MENU) + 1;
        static constexpr size_t s_MouseCount = static_cast<size_t>(TR_MOUSE_8) + 1;

        static constexpr int32_t s_MaxGamepads = 16;
        static constexpr size_t s_GamepadButtonCount = static_cast<size_t>(TR_PAD_DPAD_LEFT) + 1;
        static constexpr size_t s_GamepadAxisCount = static_cast<size_t>(TR_PAD_RT) + 1;

        static std::array<uint8_t, s_KeyCount>   s_KeyDown{};
        static std::array<uint8_t, s_KeyCount>   s_KeyPressed{};
        static std::array<uint8_t, s_KeyCount>   s_KeyReleased{};

        static std::array<uint8_t, s_MouseCount> s_MouseDown{};
        static std::array<uint8_t, s_MouseCount> s_MousePressed{};
        static std::array<uint8_t, s_MouseCount> s_MouseReleased{};

        static double s_MouseX = 0.0;
        static double s_MouseY = 0.0;
        static double s_MouseDeltaX = 0.0;
        static double s_MouseDeltaY = 0.0;
        static bool   s_MousePosValid = false;

        static double s_ScrollX = 0.0;
        static double s_ScrollY = 0.0;

        static uint16_t s_Mods = Mod_None;
        static std::vector<uint32_t> s_TypedChars;

        static std::array<uint8_t, s_MaxGamepads> s_GamepadConnected{};
        static std::array<std::array<uint8_t, s_GamepadButtonCount>, s_MaxGamepads> s_GamepadDown{};
        static std::array<std::array<uint8_t, s_GamepadButtonCount>, s_MaxGamepads> s_GamepadPressed{};
        static std::array<std::array<uint8_t, s_GamepadButtonCount>, s_MaxGamepads> s_GamepadReleased{};
        static std::array<std::array<float, s_GamepadAxisCount>, s_MaxGamepads> s_GamepadAxes{};

        static size_t ToIndex(KeyCode key) { return static_cast<size_t>(key); }
        static size_t ToIndex(MouseButton button) { return static_cast<size_t>(button); }
        static size_t ToIndex(GamepadButton button) { return static_cast<size_t>(button); }
        static size_t ToIndex(GamepadAxis axis) { return static_cast<size_t>(axis); }

        static bool ValidGamepad(int32_t id)
        {
            return id >= 0 && id < s_MaxGamepads;
        }

        void Initialize()
        {
            if (s_Initialized) return;

            std::fill(s_KeyDown.begin(), s_KeyDown.end(), 0);
            std::fill(s_KeyPressed.begin(), s_KeyPressed.end(), 0);
            std::fill(s_KeyReleased.begin(), s_KeyReleased.end(), 0);

            std::fill(s_MouseDown.begin(), s_MouseDown.end(), 0);
            std::fill(s_MousePressed.begin(), s_MousePressed.end(), 0);
            std::fill(s_MouseReleased.begin(), s_MouseReleased.end(), 0);

            std::fill(s_GamepadConnected.begin(), s_GamepadConnected.end(), 0);
            for (auto& pad : s_GamepadDown)     std::fill(pad.begin(), pad.end(), 0);
            for (auto& pad : s_GamepadPressed)  std::fill(pad.begin(), pad.end(), 0);
            for (auto& pad : s_GamepadReleased) std::fill(pad.begin(), pad.end(), 0);
            for (auto& pad : s_GamepadAxes)     std::fill(pad.begin(), pad.end(), 0.0f);

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
            if (!s_Initialized) return;

            // Resets per-frame edge flags and deltas.
            std::fill(s_KeyPressed.begin(), s_KeyPressed.end(), 0);
            std::fill(s_KeyReleased.begin(), s_KeyReleased.end(), 0);

            std::fill(s_MousePressed.begin(), s_MousePressed.end(), 0);
            std::fill(s_MouseReleased.begin(), s_MouseReleased.end(), 0);

            for (auto& pad : s_GamepadPressed)  std::fill(pad.begin(), pad.end(), 0);
            for (auto& pad : s_GamepadReleased) std::fill(pad.begin(), pad.end(), 0);

            s_MouseDeltaX = 0.0;
            s_MouseDeltaY = 0.0;
            s_ScrollX = 0.0;
            s_ScrollY = 0.0;

            s_TypedChars.clear();
        }

        void SetBlocked(bool blocked)
        {
            s_Blocked = blocked;

            // Prevents “stored” one-frame triggers when toggling block state.
            if (blocked)
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

        bool IsBlocked() { return s_Blocked; }

        void OnEvent(Engine::Event& e)
        {
            if (!s_Initialized) return;

            switch (e.GetEventType())
            {
                // Clear stuck input when window loses focus.
            case EventType::WindowLostFocus:
            {
                std::fill(s_KeyDown.begin(), s_KeyDown.end(), 0);
                std::fill(s_MouseDown.begin(), s_MouseDown.end(), 0);
                for (auto& pad : s_GamepadDown) std::fill(pad.begin(), pad.end(), 0);
                s_Mods = Mod_None;
                return;
            }

            case EventType::KeyPressed:
            {
                auto& ev = static_cast<Engine::KeyPressedEvent&>(e);
                const size_t idx = ToIndex(ev.GetKeyCode());

                if (idx < s_KeyCount)
                {
                    s_KeyDown[idx] = 1;

                    // Unity behavior: repeat should not count as a “fresh press”.
                    if (!ev.IsRepeat())
                        s_KeyPressed[idx] = 1;
                }

                s_Mods = ev.GetMods();
                return;
            }

            case EventType::KeyReleased:
            {
                auto& ev = static_cast<Engine::KeyReleasedEvent&>(e);
                const size_t idx = ToIndex(ev.GetKeyCode());

                if (idx < s_KeyCount)
                {
                    s_KeyDown[idx] = 0;
                    s_KeyReleased[idx] = 1;
                }

                s_Mods = ev.GetMods();
                return;
            }

            case EventType::KeyTyped:
            {
                auto& ev = static_cast<Engine::KeyTypedEvent&>(e);
                s_TypedChars.emplace_back(ev.GetCodepoint());
                return;
            }

            case EventType::MouseMoved:
            {
                auto& ev = static_cast<Engine::MouseMovedEvent&>(e);
                const double x = ev.GetX();
                const double y = ev.GetY();

                if (s_MousePosValid)
                {
                    s_MouseDeltaX += (x - s_MouseX);
                    s_MouseDeltaY += (y - s_MouseY);
                }

                s_MouseX = x;
                s_MouseY = y;
                s_MousePosValid = true;
                return;
            }

            case EventType::MouseScrolled:
            {
                auto& ev = static_cast<Engine::MouseScrolledEvent&>(e);
                s_ScrollX += ev.GetXOffset();
                s_ScrollY += ev.GetYOffset();
                return;
            }

            case EventType::MouseButtonPressed:
            {
                auto& ev = static_cast<Engine::MouseButtonPressedEvent&>(e);
                const size_t idx = ToIndex(ev.GetMouseButton());

                if (idx < s_MouseCount)
                {
                    s_MouseDown[idx] = 1;
                    s_MousePressed[idx] = 1;
                }

                s_Mods = ev.GetMods();
                return;
            }

            case EventType::MouseButtonReleased:
            {
                auto& ev = static_cast<Engine::MouseButtonReleasedEvent&>(e);
                const size_t idx = ToIndex(ev.GetMouseButton());

                if (idx < s_MouseCount)
                {
                    s_MouseDown[idx] = 0;
                    s_MouseReleased[idx] = 1;
                }

                s_Mods = ev.GetMods();
                return;
            }

            // Gamepad events (state stays default until events are emitted by the platform layer)
            case EventType::GamepadConnected:
            {
                auto& ev = static_cast<Engine::GamepadConnectedEvent&>(e);
                if (ValidGamepad(ev.GetGamepadId()))
                    s_GamepadConnected[ev.GetGamepadId()] = 1;
                return;
            }

            case EventType::GamepadDisconnected:
            {
                auto& ev = static_cast<Engine::GamepadDisconnectedEvent&>(e);
                if (ValidGamepad(ev.GetGamepadId()))
                {
                    const int32_t id = ev.GetGamepadId();
                    s_GamepadConnected[id] = 0;
                    std::fill(s_GamepadDown[id].begin(), s_GamepadDown[id].end(), 0);
                    std::fill(s_GamepadPressed[id].begin(), s_GamepadPressed[id].end(), 0);
                    std::fill(s_GamepadReleased[id].begin(), s_GamepadReleased[id].end(), 0);
                    std::fill(s_GamepadAxes[id].begin(), s_GamepadAxes[id].end(), 0.0f);
                }
                return;
            }

            case EventType::GamepadButtonPressed:
            {
                auto& ev = static_cast<Engine::GamepadButtonPressedEvent&>(e);
                if (!ValidGamepad(ev.GetGamepadId())) return;

                const int32_t id = ev.GetGamepadId();
                const size_t idx = ToIndex(ev.GetButton());
                if (idx < s_GamepadButtonCount)
                {
                    s_GamepadDown[id][idx] = 1;
                    s_GamepadPressed[id][idx] = 1;
                }
                return;
            }

            case EventType::GamepadButtonReleased:
            {
                auto& ev = static_cast<Engine::GamepadButtonReleasedEvent&>(e);
                if (!ValidGamepad(ev.GetGamepadId())) return;

                const int32_t id = ev.GetGamepadId();
                const size_t idx = ToIndex(ev.GetButton());
                if (idx < s_GamepadButtonCount)
                {
                    s_GamepadDown[id][idx] = 0;
                    s_GamepadReleased[id][idx] = 1;
                }
                return;
            }

            case EventType::GamepadAxisMoved:
            {
                auto& ev = static_cast<Engine::GamepadAxisMovedEvent&>(e);
                if (!ValidGamepad(ev.GetGamepadId())) return;

                const int32_t id = ev.GetGamepadId();
                const size_t idx = ToIndex(ev.GetAxis());
                if (idx < s_GamepadAxisCount)
                    s_GamepadAxes[id][idx] = ev.GetValue();
                return;
            }

            default:
                return;
            }
        }

        // Queries (blocked means returning “no input” without destroying internal state)
        bool KeyDown(KeyCode key)
        {
            if (s_Blocked) return false;
            const size_t idx = ToIndex(key);
            return idx < s_KeyCount && s_KeyDown[idx] != 0;
        }

        bool KeyPressed(KeyCode key)
        {
            if (s_Blocked) return false;
            const size_t idx = ToIndex(key);
            return idx < s_KeyCount && s_KeyPressed[idx] != 0;
        }

        bool KeyReleased(KeyCode key)
        {
            if (s_Blocked) return false;
            const size_t idx = ToIndex(key);
            return idx < s_KeyCount && s_KeyReleased[idx] != 0;
        }

        bool MouseDown(MouseButton button)
        {
            if (s_Blocked) return false;
            const size_t idx = ToIndex(button);
            return idx < s_MouseCount && s_MouseDown[idx] != 0;
        }

        bool MousePressed(MouseButton button)
        {
            if (s_Blocked) return false;
            const size_t idx = ToIndex(button);
            return idx < s_MouseCount && s_MousePressed[idx] != 0;
        }

        bool MouseReleased(MouseButton button)
        {
            if (s_Blocked) return false;
            const size_t idx = ToIndex(button);
            return idx < s_MouseCount && s_MouseReleased[idx] != 0;
        }

        double MouseX() { return s_MouseX; }
        double MouseY() { return s_MouseY; }
        double MouseDeltaX() { return s_MouseDeltaX; }
        double MouseDeltaY() { return s_MouseDeltaY; }
        double ScrollX() { return s_ScrollX; }
        double ScrollY() { return s_ScrollY; }

        const std::vector<uint32_t>& TypedChars()
        {
            static const std::vector<uint32_t> empty;
            return s_Blocked ? empty : s_TypedChars;
        }

        uint16_t Mods() { return s_Mods; }

        bool GamepadConnected(int32_t gamepadId)
        {
            if (s_Blocked) return false;
            return ValidGamepad(gamepadId) && s_GamepadConnected[gamepadId] != 0;
        }

        bool GamepadButtonDown(int32_t gamepadId, GamepadButton button)
        {
            if (s_Blocked) return false;
            if (!ValidGamepad(gamepadId)) return false;
            const size_t idx = ToIndex(button);
            return idx < s_GamepadButtonCount && s_GamepadDown[gamepadId][idx] != 0;
        }

        bool GamepadButtonPressed(int32_t gamepadId, GamepadButton button)
        {
            if (s_Blocked) return false;
            if (!ValidGamepad(gamepadId)) return false;
            const size_t idx = ToIndex(button);
            return idx < s_GamepadButtonCount && s_GamepadPressed[gamepadId][idx] != 0;
        }

        bool GamepadButtonReleased(int32_t gamepadId, GamepadButton button)
        {
            if (s_Blocked)
            {
                return false;
            }
            
            if (!ValidGamepad(gamepadId))
            {
                return false;
            }

            const size_t idx = ToIndex(button);

            return idx < s_GamepadButtonCount && s_GamepadReleased[gamepadId][idx] != 0;
        }

        float GetGamepadAxis(int32_t gamepadId, GamepadAxis axis)
        {
            if (s_Blocked)
            {
                return 0.0f;
            }

            if (!ValidGamepad(gamepadId))
            {
                return 0.0f;
            }

            const size_t idx = ToIndex(axis);

            return idx < s_GamepadAxisCount ? s_GamepadAxes[gamepadId][idx] : 0.0f;
        }
    }
}