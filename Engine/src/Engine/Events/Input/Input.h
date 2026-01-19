#pragma once

#include "Engine/Events/Event.h"
#include "Engine/Events/Input/KeyCodes.h"
#include "Engine/Events/Input/MouseCodes.h"
#include "Engine/Events/Input/GamepadCodes.h"

#include <cstdint>
#include <vector>

namespace Engine
{
    namespace Input
    {
        void Initialize();
        void Shutdown();

        // Called once per frame
        void BeginFrame();

        // Called from Application::OnEvent
        void OnEvent(Engine::Event& e);

        // Blocks query results
        void SetBlocked(bool blocked);
        bool IsBlocked();

        // Keyboard
        bool KeyDown(KeyCode key);
        bool KeyPressed(KeyCode key); 
        bool KeyReleased(KeyCode key);

        // Mouse buttons
        bool MouseDown(MouseButton button);
        bool MousePressed(MouseButton button);
        bool MouseReleased(MouseButton button);

        // Mouse movement / scroll (per-frame deltas)
        double GetMouseX();
        double GetMouseY();
        double GetMouseDeltaX();
        double GetMouseDeltaY();
        double GetScrollX();
        double GetScrollY();

        // Text input
        const std::vector<uint32_t>& TypedChars();

        // Last-known modifier state from input events
        uint16_t Mods();

        // Gamepad
        bool GamepadConnected(int32_t gamepadID);
        bool GamepadButtonDown(int32_t gamepadID, GamepadButton button);
        bool GamepadButtonPressed(int32_t gamepadID, GamepadButton button);
        bool GamepadButtonReleased(int32_t gamepadID, GamepadButton button);
        float GetGamepadAxis(int32_t gamepadID, GamepadAxis axis);
    }
}