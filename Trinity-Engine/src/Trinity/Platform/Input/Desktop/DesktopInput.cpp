#include "Trinity/Platform/Input/Desktop/DesktopInput.h"

#include "Trinity/Events/Event.h"
#include "Trinity/Events/GamepadEvent.h"
#include "Trinity/Events/KeyEvent.h"
#include "Trinity/Events/MouseEvent.h"

#include "Trinity/Utilities/Log.h"

namespace Trinity
{
    std::unordered_map<int, DesktopInput::ButtonState> DesktopInput::s_KeyStates;
    std::unordered_map<int, DesktopInput::ButtonState> DesktopInput::s_MouseButtonStates;
    DesktopInput::Vector2 DesktopInput::s_MousePosition = DesktopInput::Vector2(0.0f);
    DesktopInput::Vector2 DesktopInput::s_MouseScrollDelta = DesktopInput::Vector2(0.0f);
    DesktopInput::Vector2 DesktopInput::s_MouseRawDelta = DesktopInput::Vector2(0.0f);
    std::unordered_map<int, DesktopInput::GamepadState> DesktopInput::s_GamepadStates;

    void DesktopInput::BeginFrame()
    {
        // Clear one-frame edges
        for (auto& [it_Key, it_State] : s_KeyStates)
        {
            it_State.Pressed = false;
            it_State.Released = false;
        }

        for (auto& [it_Key, it_State] : s_MouseButtonStates)
        {
            it_State.Pressed = false;
            it_State.Released = false;
        }

        for (auto& [it_Button, it_Gamepad] : s_GamepadStates)
        {
            for (auto& [it_Button, it_State] : it_Gamepad.ButtonStates)
            {
                it_State.Pressed = false;
                it_State.Released = false;
            }
        }

        // This frame's scroll starts at 0
        s_MouseScrollDelta = Vector2(0.0f);
        s_MouseRawDelta = Vector2(0.0f);
    }

    void DesktopInput::OnEvent(Event& e)
    {
        EventDispatcher l_Dispatcher(e);
        l_Dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent& event) { return DesktopInput::OnKeyPressedEvent(event); });
        l_Dispatcher.Dispatch<KeyReleasedEvent>([](KeyReleasedEvent& event) { return DesktopInput::OnKeyReleasedEvent(event); });
        l_Dispatcher.Dispatch<MouseButtonPressedEvent>([](MouseButtonPressedEvent& event) { return DesktopInput::OnMouseButtonPressedEvent(event); });
        l_Dispatcher.Dispatch<MouseButtonReleasedEvent>([](MouseButtonReleasedEvent& event) { return DesktopInput::OnMouseButtonReleasedEvent(event); });
        l_Dispatcher.Dispatch<MouseMovedEvent>([](MouseMovedEvent& event) { return DesktopInput::OnMouseMovedEvent(event); });
        l_Dispatcher.Dispatch<MouseScrolledEvent>([](MouseScrolledEvent& event) { return DesktopInput::OnMouseScrolledEvent(event); });
        l_Dispatcher.Dispatch<MouseRawDeltaEvent>([](MouseRawDeltaEvent& event) { return DesktopInput::OnMouseRawDeltaEvent(event); });

        l_Dispatcher.Dispatch<GamepadConnectedEvent>([](GamepadConnectedEvent& event) { return DesktopInput::OnGamepadConnectedEvent(event); });
        l_Dispatcher.Dispatch<GamepadDisconnectedEvent>([](GamepadDisconnectedEvent& event) { return DesktopInput::OnGamepadDisconnectedEvent(event); });
        l_Dispatcher.Dispatch<GamepadButtonPressedEvent>([](GamepadButtonPressedEvent& event) { return DesktopInput::OnGamepadButtonPressedEvent(event); });
        l_Dispatcher.Dispatch<GamepadButtonReleasedEvent>([](GamepadButtonReleasedEvent& event) { return DesktopInput::OnGamepadButtonReleasedEvent(event); });
        l_Dispatcher.Dispatch<GamepadAxisMovedEvent>([](GamepadAxisMovedEvent& event) { return DesktopInput::OnGamepadAxisMovedEvent(event); });
    }

    // ---------------- Keyboard ----------------

    bool DesktopInput::KeyDown(Code::KeyCode keyCode)
    {
        const int l_KeyValue = static_cast<int>(keyCode);
        auto a_Index = s_KeyStates.find(l_KeyValue);

        return (a_Index != s_KeyStates.end()) ? a_Index->second.IsDown : false;
    }

    bool DesktopInput::KeyPressed(Code::KeyCode keyCode)
    {
        const int l_KeyValue = static_cast<int>(keyCode);
        auto a_Index = s_KeyStates.find(l_KeyValue);

        return (a_Index != s_KeyStates.end()) ? a_Index->second.Pressed : false;
    }

    bool DesktopInput::KeyReleased(Code::KeyCode keyCode)
    {
        const int l_KeyValue = static_cast<int>(keyCode);
        auto a_Index = s_KeyStates.find(l_KeyValue);

        return (a_Index != s_KeyStates.end()) ? a_Index->second.Released : false;
    }

    // ---------------- Mouse ----------------

    bool DesktopInput::MouseButtonDown(Code::MouseCode button)
    {
        const int l_ButtonValue = static_cast<int>(button);
        auto a_Index = s_MouseButtonStates.find(l_ButtonValue);

        return (a_Index != s_MouseButtonStates.end()) ? a_Index->second.IsDown : false;
    }

    bool DesktopInput::MouseButtonPressed(Code::MouseCode button)
    {
        const int l_ButtonValue = static_cast<int>(button);
        auto a_Index = s_MouseButtonStates.find(l_ButtonValue);

        return (a_Index != s_MouseButtonStates.end()) ? a_Index->second.Pressed : false;
    }

    bool DesktopInput::MouseButtonReleased(Code::MouseCode button)
    {
        const int l_ButtonValue = static_cast<int>(button);
        auto a_Index = s_MouseButtonStates.find(l_ButtonValue);

        return (a_Index != s_MouseButtonStates.end()) ? a_Index->second.Released : false;
    }

    DesktopInput::Vector2 DesktopInput::MousePosition()
    {
        return s_MousePosition;
    }

    DesktopInput::Vector2 DesktopInput::MouseScrolled()
    {
        return s_MouseScrollDelta;
    }

    DesktopInput::Vector2 DesktopInput::MouseDelta()
    {
        return s_MouseRawDelta;
    }

    // ---------------- Gamepad ----------------

    bool DesktopInput::GamepadButtonDown(int gamepadID, Code::GamepadButton button)
    {
        auto a_GamepadIndex = s_GamepadStates.find(gamepadID);
        if (a_GamepadIndex == s_GamepadStates.end() || !a_GamepadIndex->second.Connected)
        {
            return false;
        }

        const int l_ButtonValue = static_cast<int>(button);
        auto& a_ButtonMap = a_GamepadIndex->second.ButtonStates;
        auto a_ButtonIndex = a_ButtonMap.find(l_ButtonValue);

        return (a_ButtonIndex != a_ButtonMap.end()) ? a_ButtonIndex->second.IsDown : false;
    }

    bool DesktopInput::GamepadButtonPressed(int gamepadID, Code::GamepadButton button)
    {
        auto a_GamepadIndex = s_GamepadStates.find(gamepadID);
        if (a_GamepadIndex == s_GamepadStates.end() || !a_GamepadIndex->second.Connected)
        {
            return false;
        }

        const int l_ButtonValue = static_cast<int>(button);
        auto& a_ButtonMap = a_GamepadIndex->second.ButtonStates;
        auto a_ButtonIndex = a_ButtonMap.find(l_ButtonValue);

        return (a_ButtonIndex != a_ButtonMap.end()) ? a_ButtonIndex->second.Pressed : false;
    }

    bool DesktopInput::GamepadButtonReleased(int gamepadID, Code::GamepadButton button)
    {
        auto a_GamepadIndex = s_GamepadStates.find(gamepadID);
        if (a_GamepadIndex == s_GamepadStates.end() || !a_GamepadIndex->second.Connected)
        {
            return false;
        }

        const int l_ButtonValue = static_cast<int>(button);
        auto& a_ButtonMap = a_GamepadIndex->second.ButtonStates;
        auto a_ButtonIndex = a_ButtonMap.find(l_ButtonValue);

        return (a_ButtonIndex != a_ButtonMap.end()) ? a_ButtonIndex->second.Released : false;
    }

    float DesktopInput::GamepadAxis(int gamepadID, Code::GamepadAxis axis)
    {
        auto a_GamepadIndex = s_GamepadStates.find(gamepadID);
        if (a_GamepadIndex == s_GamepadStates.end() || !a_GamepadIndex->second.Connected)
        {
            return 0.0f;
        }

        const int l_AxisValue = static_cast<int>(axis);
        auto& axisMap = a_GamepadIndex->second.AxisValues;
        auto axisIt = axisMap.find(l_AxisValue);

        return (axisIt != axisMap.end()) ? axisIt->second : 0.0f;
    }

    // ---------------- Internal access ----------------

    DesktopInput::ButtonState& DesktopInput::AccessKeyState(Code::KeyCode keyCode)
    {
        const int l_KeyValue = static_cast<int>(keyCode);
        
        return s_KeyStates[l_KeyValue];
    }

    DesktopInput::ButtonState& DesktopInput::AccessMouseButtonState(Code::MouseCode button)
    {
        const int l_ButtonValue = static_cast<int>(button);

        return s_MouseButtonStates[l_ButtonValue];
    }

    DesktopInput::GamepadState& DesktopInput::AccessGamepadState(int gamepadID)
    {
        return s_GamepadStates[gamepadID];
    }

    DesktopInput::ButtonState& DesktopInput::AccessGamepadButtonState(int gamepadID, Code::GamepadButton button)
    {
        const int l_ButtonValue = static_cast<int>(button);
        
        return s_GamepadStates[gamepadID].ButtonStates[l_ButtonValue];
    }

    float& DesktopInput::AccessGamepadAxisState(int gamepadID, Code::GamepadAxis axis)
    {
        const int l_AxisValue = static_cast<int>(axis);

        return s_GamepadStates[gamepadID].AxisValues[l_AxisValue];
    }

    // ---------------- Event handlers ----------------

    bool DesktopInput::OnKeyPressedEvent(KeyPressedEvent& e)
    {
        ButtonState& l_State = AccessKeyState(e.GetKeyCode());

        // Only mark Pressed on the initial press (not repeats)
        if (e.GetRepeatCount() == 0 && !l_State.IsDown)
        {
            l_State.Pressed = true;
        }

        l_State.IsDown = true;
        
        return false;
    }

    bool DesktopInput::OnKeyReleasedEvent(KeyReleasedEvent& e)
    {
        ButtonState& l_State = AccessKeyState(e.GetKeyCode());

        l_State.IsDown = false;
        l_State.Released = true;
        
        return false;
    }

    bool DesktopInput::OnMouseButtonPressedEvent(MouseButtonPressedEvent& e)
    {
        ButtonState& l_State = AccessMouseButtonState(e.GetMouseButton());

        if (!l_State.IsDown)
        {
            l_State.Pressed = true;
        }

        l_State.IsDown = true;

        return false;
    }

    bool DesktopInput::OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e)
    {
        ButtonState& l_State = AccessMouseButtonState(e.GetMouseButton());

        l_State.IsDown = false;
        l_State.Released = true;
        
        return false;
    }

    bool DesktopInput::OnMouseMovedEvent(MouseMovedEvent& e)
    {
        s_MousePosition.x = e.GetX();
        s_MousePosition.y = e.GetY();

        return false;
    }

    bool DesktopInput::OnMouseScrolledEvent(MouseScrolledEvent& e)
    {
        s_MouseScrollDelta.x += e.GetXOffset();
        s_MouseScrollDelta.y += e.GetYOffset();

        return false;
    }

    bool DesktopInput::OnMouseRawDeltaEvent(MouseRawDeltaEvent& e)
    {
        s_MouseRawDelta.x += e.GetXDelta();
        s_MouseRawDelta.y += e.GetYDelta();
        
        return false;
    }

    bool DesktopInput::OnGamepadConnectedEvent(GamepadConnectedEvent& e)
    {
        GamepadState& l_State = AccessGamepadState(e.GetGamepadID());
        l_State.Connected = true;

        return false;
    }

    bool DesktopInput::OnGamepadDisconnectedEvent(GamepadDisconnectedEvent& e)
    {
        auto a_Index = s_GamepadStates.find(e.GetGamepadID());
        if (a_Index != s_GamepadStates.end())
        {
            a_Index->second.Connected = false;
            a_Index->second.ButtonStates.clear();
            a_Index->second.AxisValues.clear();
        }
        
        return false;
    }

    bool DesktopInput::OnGamepadButtonPressedEvent(GamepadButtonPressedEvent& e)
    {
        ButtonState& l_State = AccessGamepadButtonState(e.GetGamepadID(), e.GetButton());

        if (!l_State.IsDown)
        {
            l_State.Pressed = true;
        }
        l_State.IsDown = true;
        
        return false;
    }

    bool DesktopInput::OnGamepadButtonReleasedEvent(GamepadButtonReleasedEvent& e)
    {
        ButtonState& l_State = AccessGamepadButtonState(e.GetGamepadID(), e.GetButton());

        l_State.IsDown = false;
        l_State.Released = true;
        
        return false;
    }

    bool DesktopInput::OnGamepadAxisMovedEvent(GamepadAxisMovedEvent& e)
    {
        float& l_Value = AccessGamepadAxisState(e.GetGamepadID(), e.GetAxis());
        l_Value = e.GetValue();
 
        return false;
    }
}