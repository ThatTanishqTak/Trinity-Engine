#pragma once

#include "Engine/Input/InputCodes.h"

#include <glm/vec2.hpp>
#include <unordered_map>

namespace Engine
{
    class Event;
    class KeyPressedEvent;
    class KeyReleasedEvent;
    class MouseButtonPressedEvent;
    class MouseButtonReleasedEvent;
    class MouseMovedEvent;
    class MouseScrolledEvent;
    class GamepadConnectedEvent;
    class GamepadDisconnectedEvent;
    class GamepadButtonPressedEvent;
    class GamepadButtonReleasedEvent;
    class GamepadAxisMovedEvent;

    class Input
    {
    public:
        using Vector2 = glm::vec2;

        // Call once per frame BEFORE polling events.
        static void BeginFrame();

        static void OnEvent(Event& e);

        // Keyboard
        static bool KeyDown(Code::KeyCode keyCode);
        static bool KeyPressed(Code::KeyCode keyCode);
        static bool KeyReleased(Code::KeyCode keyCode);

        // Mouse
        static bool MouseButtonDown(Code::MouseCode button);
        static bool MouseButtonPressed(Code::MouseCode button);
        static bool MouseButtonReleased(Code::MouseCode button);
        static Vector2 MousePosition();
        static Vector2 MouseScrolled(); // this frame's scroll delta (not consuming)

        // Gamepad
        static bool GamepadButtonDown(int gamepadID, Code::GamepadButton button);
        static bool GamepadButtonPressed(int gamepadID, Code::GamepadButton button);
        static bool GamepadButtonReleased(int gamepadID, Code::GamepadButton button);
        static float GamepadAxis(int gamepadID, Code::GamepadAxis axis);

    private:
        struct ButtonState
        {
            bool IsDown = false;
            bool Pressed = false;   // one-frame edge
            bool Released = false;  // one-frame edge
        };

        struct GamepadState
        {
            bool Connected = false;
            std::unordered_map<int, ButtonState> ButtonStates;
            std::unordered_map<int, float> AxisValues;
        };

        static ButtonState& AccessKeyState(Code::KeyCode keyCode);
        static ButtonState& AccessMouseButtonState(Code::MouseCode button);
        static GamepadState& AccessGamepadState(int gamepadID);
        static ButtonState& AccessGamepadButtonState(int gamepadID, Code::GamepadButton button);
        static float& AccessGamepadAxisState(int gamepadID, Code::GamepadAxis axis);

        static bool OnKeyPressedEvent(KeyPressedEvent& e);
        static bool OnKeyReleasedEvent(KeyReleasedEvent& e);
        static bool OnMouseButtonPressedEvent(MouseButtonPressedEvent& e);
        static bool OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e);
        static bool OnMouseMovedEvent(MouseMovedEvent& e);
        static bool OnMouseScrolledEvent(MouseScrolledEvent& e);
        static bool OnGamepadConnectedEvent(GamepadConnectedEvent& e);
        static bool OnGamepadDisconnectedEvent(GamepadDisconnectedEvent& e);
        static bool OnGamepadButtonPressedEvent(GamepadButtonPressedEvent& e);
        static bool OnGamepadButtonReleasedEvent(GamepadButtonReleasedEvent& e);
        static bool OnGamepadAxisMovedEvent(GamepadAxisMovedEvent& e);

        static std::unordered_map<int, ButtonState> s_KeyStates;
        static std::unordered_map<int, ButtonState> s_MouseButtonStates;
        static Vector2 s_MousePosition;
        static Vector2 s_MouseScrollDelta;
        static std::unordered_map<int, GamepadState> s_GamepadStates;
    };
}