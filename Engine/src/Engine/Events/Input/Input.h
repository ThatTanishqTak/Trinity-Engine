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

        // Called once per frame (clears per-frame flags like KeyPressed/Released, mouse delta, scroll, typed chars).
        void BeginFrame();

        // Called from Application::OnEvent (updates cached input state).
        void OnEvent(Engine::Event& e);

        // Optional: blocks query results (useful when UI wants to capture input).
        void SetBlocked(bool blocked);
        bool IsBlocked();

        // Keyboard (Unity-ish)
        bool KeyDown(KeyCode key);
        bool KeyPressed(KeyCode key);    // true only on the first press, ignores repeats
        bool KeyReleased(KeyCode key);

        // Mouse buttons
        bool MouseDown(MouseButton button);
        bool MousePressed(MouseButton button);
        bool MouseReleased(MouseButton button);

        // Mouse movement / scroll (per-frame deltas)
        double MouseX();
        double MouseY();
        double MouseDeltaX();
        double MouseDeltaY();
        double ScrollX();
        double ScrollY();

        // Text input (UTF-32 codepoints typed this frame)
        const std::vector<uint32_t>& TypedChars();

        // Last-known modifier state from input events
        uint16_t Mods();

        // Gamepad (works once gamepad events exist; safe to call now)
        bool GamepadConnected(int32_t gamepadId);
        bool GamepadButtonDown(int32_t gamepadId, GamepadButton button);
        bool GamepadButtonPressed(int32_t gamepadId, GamepadButton button);
        bool GamepadButtonReleased(int32_t gamepadId, GamepadButton button);
        float GetGamepadAxis(int32_t gamepadId, GamepadAxis axis);
    }
}