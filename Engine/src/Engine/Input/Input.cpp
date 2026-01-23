#include "Engine/Input/Input.h"

#include "Engine/Events/Event.h"
#include "Engine/Events/GamepadEvent.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"

#include "Engine/Utilities/Utilities.h"

namespace Engine
{
    std::unordered_map<int, Input::ButtonState> Input::s_KeyStates;
    std::unordered_map<int, Input::ButtonState> Input::s_MouseButtonStates;
    Input::Vector2 Input::s_MousePosition = Input::Vector2(0.0f);
    Input::Vector2 Input::s_MouseScrollDelta = Input::Vector2(0.0f);
    std::unordered_map<int, Input::GamepadState> Input::s_GamepadStates;

    void Input::OnEvent(Event& e)
    {
        EventDispatcher l_Dispatcher(e);
        l_Dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent& event) { return Input::OnKeyPressedEvent(event); });
        l_Dispatcher.Dispatch<KeyReleasedEvent>([](KeyReleasedEvent& event) { return Input::OnKeyReleasedEvent(event); });
        l_Dispatcher.Dispatch<MouseButtonPressedEvent>([](MouseButtonPressedEvent& event) { return Input::OnMouseButtonPressedEvent(event); });
        l_Dispatcher.Dispatch<MouseButtonReleasedEvent>([](MouseButtonReleasedEvent& event) { return Input::OnMouseButtonReleasedEvent(event); });
        l_Dispatcher.Dispatch<MouseMovedEvent>([](MouseMovedEvent& event) { return Input::OnMouseMovedEvent(event); });
        l_Dispatcher.Dispatch<MouseScrolledEvent>([](MouseScrolledEvent& event) { return Input::OnMouseScrolledEvent(event); });
        l_Dispatcher.Dispatch<GamepadConnectedEvent>([](GamepadConnectedEvent& event) { return Input::OnGamepadConnectedEvent(event); });
        l_Dispatcher.Dispatch<GamepadDisconnectedEvent>([](GamepadDisconnectedEvent& event) { return Input::OnGamepadDisconnectedEvent(event); });
        l_Dispatcher.Dispatch<GamepadButtonPressedEvent>([](GamepadButtonPressedEvent& event) { return Input::OnGamepadButtonPressedEvent(event); });
        l_Dispatcher.Dispatch<GamepadButtonReleasedEvent>([](GamepadButtonReleasedEvent& event) { return Input::OnGamepadButtonReleasedEvent(event); });
        l_Dispatcher.Dispatch<GamepadAxisMovedEvent>([](GamepadAxisMovedEvent& event) { return Input::OnGamepadAxisMovedEvent(event); });

        if (e.Handled)
        {
            TR_CORE_TRACE(e.ToString());

            return;
        }
    }

    bool Input::KeyPressed(Code::KeyCode keyCode)
    {
        const int l_KeyValue = static_cast<int>(keyCode);
        auto a_StateInterator = s_KeyStates.find(l_KeyValue);
        if (a_StateInterator == s_KeyStates.end())
        {
            return false;
        }

        return ConsumePressed(a_StateInterator->second);
    }

    bool Input::KeyReleased(Code::KeyCode keyCode)
    {
        const int l_KeyValue = static_cast<int>(keyCode);
        auto a_StateInterator = s_KeyStates.find(l_KeyValue);
        if (a_StateInterator == s_KeyStates.end())
        {
            return false;
        }

        return ConsumeReleased(a_StateInterator->second);
    }

    bool Input::MouseButtonPressed(Code::MouseCode button)
    {
        const int l_ButtonValue = static_cast<int>(button);
        auto a_StateInterator = s_MouseButtonStates.find(l_ButtonValue);
        if (a_StateInterator == s_MouseButtonStates.end())
        {
            return false;
        }

        return ConsumePressed(a_StateInterator->second);
    }

    bool Input::MouseButtonReleased(Code::MouseCode button)
    {
        const int l_ButtonValue = static_cast<int>(button);
        auto a_StateInterator = s_MouseButtonStates.find(l_ButtonValue);
        if (a_StateInterator == s_MouseButtonStates.end())
        {
            return false;
        }

        return ConsumeReleased(a_StateInterator->second);
    }

    Input::Vector2 Input::MousePosition()
    {
        return s_MousePosition;
    }

    Input::Vector2 Input::MouseScrolled()
    {
        const Vector2 l_Delta = s_MouseScrollDelta;
        s_MouseScrollDelta = Vector2{};

        return l_Delta;
    }

    bool Input::GamepadButtonPressed(int gamepadId, Code::GamepadButton button)
    {
        const int l_ButtonValue = static_cast<int>(button);
        auto a_GamepadInterator = s_GamepadStates.find(gamepadId);
        if (a_GamepadInterator == s_GamepadStates.end() || !a_GamepadInterator->second.Connected)
        {
            return false;
        }

        auto& a_ButtonStates = a_GamepadInterator->second.ButtonStates;
        auto a_ButtonInterator = a_ButtonStates.find(l_ButtonValue);
        if (a_ButtonInterator == a_ButtonStates.end())
        {
            return false;
        }

        return ConsumePressed(a_ButtonInterator->second);
    }

    bool Input::GamepadButtonReleased(int gamepadId, Code::GamepadButton button)
    {
        const int l_ButtonValue = static_cast<int>(button);
        auto a_GamepadInterator = s_GamepadStates.find(gamepadId);
        if (a_GamepadInterator == s_GamepadStates.end() || !a_GamepadInterator->second.Connected)
        {
            return false;
        }

        auto& a_ButtonStates = a_GamepadInterator->second.ButtonStates;
        auto a_ButtonInterator = a_ButtonStates.find(l_ButtonValue);
        if (a_ButtonInterator == a_ButtonStates.end())
        {
            return false;
        }

        return ConsumeReleased(a_ButtonInterator->second);
    }

    float Input::GamepadAxis(int gamepadId, Code::GamepadAxis axis)
    {
        const int l_AxisValue = static_cast<int>(axis);
        auto a_GamepadInterator = s_GamepadStates.find(gamepadId);
        if (a_GamepadInterator == s_GamepadStates.end() || !a_GamepadInterator->second.Connected)
        {
            return 0.0f;
        }

        auto& a_AxisValues = a_GamepadInterator->second.AxisValues;
        auto a_AxisInterator = a_AxisValues.find(l_AxisValue);
        if (a_AxisInterator == a_AxisValues.end())
        {
            return 0.0f;
        }

        return a_AxisInterator->second;
    }

    Input::ButtonState& Input::AccessKeyState(Code::KeyCode keyCode)
    {
        const int l_KeyValue = static_cast<int>(keyCode);
        auto& a_State = s_KeyStates[l_KeyValue];

        return a_State;
    }

    Input::ButtonState& Input::AccessMouseButtonState(Code::MouseCode button)
    {
        const int l_ButtonValue = static_cast<int>(button);
        auto& a_State = s_MouseButtonStates[l_ButtonValue];

        return a_State;
    }

    Input::GamepadState& Input::AccessGamepadState(int gamepadId)
    {
        auto& a_State = s_GamepadStates[gamepadId];

        return a_State;
    }

    Input::ButtonState& Input::AccessGamepadButtonState(int gamepadId, Code::GamepadButton button)
    {
        const int l_ButtonValue = static_cast<int>(button);
        auto& a_State = s_GamepadStates[gamepadId].ButtonStates[l_ButtonValue];

        return a_State;
    }

    float& Input::AccessGamepadAxisState(int gamepadId, Code::GamepadAxis axis)
    {
        const int l_AxisValue = static_cast<int>(axis);
        auto& a_Value = s_GamepadStates[gamepadId].AxisValues[l_AxisValue];

        return a_Value;
    }

    bool Input::ConsumePressed(ButtonState& state)
    {
        const bool l_Pressed = state.Pressed;
        state.Pressed = false;

        return l_Pressed;
    }

    bool Input::ConsumeReleased(ButtonState& state)
    {
        const bool l_Released = state.Released;
        state.Released = false;

        return l_Released;
    }

    bool Input::OnKeyPressedEvent(KeyPressedEvent& e)
    {
        ButtonState& l_State = AccessKeyState(e.GetKeyCode());

        l_State.IsDown = true;
        if (e.GetRepeatCount() == 0)
        {
            l_State.Pressed = true;
            l_State.Released = false;
        }

        return false;
    }

    bool Input::OnKeyReleasedEvent(KeyReleasedEvent& e)
    {
        ButtonState& l_State = AccessKeyState(e.GetKeyCode());

        l_State.IsDown = false;
        l_State.Released = true;
        l_State.Pressed = false;

        return false;
    }

    bool Input::OnMouseButtonPressedEvent(MouseButtonPressedEvent& e)
    {
        ButtonState& l_State = AccessMouseButtonState(e.GetMouseButton());

        l_State.IsDown = true;
        l_State.Pressed = true;
        l_State.Released = false;

        return false;
    }

    bool Input::OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e)
    {
        ButtonState& l_State = AccessMouseButtonState(e.GetMouseButton());

        l_State.IsDown = false;
        l_State.Released = true;
        l_State.Pressed = false;

        return false;
    }

    bool Input::OnMouseMovedEvent(MouseMovedEvent& e)
    {
        s_MousePosition.x = e.GetX();
        s_MousePosition.y = e.GetY();

        return false;
    }

    bool Input::OnMouseScrolledEvent(MouseScrolledEvent& e)
    {
        s_MouseScrollDelta.x += e.GetXOffset();
        s_MouseScrollDelta.y += e.GetYOffset();

        return false;
    }

    bool Input::OnGamepadConnectedEvent(GamepadConnectedEvent& e)
    {
        GamepadState& l_State = AccessGamepadState(e.GetGamepadID());
        l_State.Connected = true;

        return false;
    }

    bool Input::OnGamepadDisconnectedEvent(GamepadDisconnectedEvent& e)
    {
        auto a_GamepadInterator = s_GamepadStates.find(e.GetGamepadID());
        if (a_GamepadInterator != s_GamepadStates.end())
        {
            a_GamepadInterator->second.Connected = false;
            a_GamepadInterator->second.ButtonStates.clear();
            a_GamepadInterator->second.AxisValues.clear();
        }

        return false;
    }

    bool Input::OnGamepadButtonPressedEvent(GamepadButtonPressedEvent& e)
    {
        ButtonState& l_State = AccessGamepadButtonState(e.GetGamepadID(), e.GetButton());

        l_State.IsDown = true;
        l_State.Pressed = true;
        l_State.Released = false;

        return false;
    }

    bool Input::OnGamepadButtonReleasedEvent(GamepadButtonReleasedEvent& e)
    {
        ButtonState& l_State = AccessGamepadButtonState(e.GetGamepadID(), e.GetButton());

        l_State.IsDown = false;
        l_State.Released = true;
        l_State.Pressed = false;

        return false;
    }

    bool Input::OnGamepadAxisMovedEvent(GamepadAxisMovedEvent& e)
    {
        float& l_Value = AccessGamepadAxisState(e.GetGamepadID(), e.GetAxis());
        l_Value = e.GetValue();

        return false;
    }
}