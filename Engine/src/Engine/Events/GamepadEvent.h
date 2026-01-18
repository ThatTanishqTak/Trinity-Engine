#pragma once

#include "Engine/Events/Event.h"
#include "Engine/Events/Input/GamepadCodes.h"

namespace Engine
{
    using GamepadButton = Input::GamepadButton;
    using GamepadAxis = Input::GamepadAxis;

    class GamepadEvent : public Event
    {
    public:
        int32_t GetGamepadId() const { return m_GamepadID; }
        TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryGamepad)

    protected:
        explicit GamepadEvent(int32_t gamepadID) : m_GamepadID(gamepadID) {}
        int32_t m_GamepadID = 0;
    };

    class GamepadConnectedEvent : public GamepadEvent
    {
    public:
        explicit GamepadConnectedEvent(int32_t gamepadID) : GamepadEvent(gamepadID) {}
        TR_EVENT_CLASS_TYPE(GamepadConnected)
    };

    class GamepadDisconnectedEvent : public GamepadEvent
    {
    public:
        explicit GamepadDisconnectedEvent(int32_t gamepadID) : GamepadEvent(gamepadID) {}
        TR_EVENT_CLASS_TYPE(GamepadDisconnected)
    };

    class GamepadButtonPressedEvent : public GamepadEvent
    {
    public:
        GamepadButtonPressedEvent(int32_t gamepadID, GamepadButton button, float value) : GamepadEvent(gamepadID), m_Button(button), m_Value(value)
        {

        }

        GamepadButton GetButton() const { return m_Button; }
        float GetValue() const { return m_Value; }

        TR_EVENT_CLASS_TYPE(GamepadButtonPressed)

    private:
        GamepadButton m_Button = Input::TR_PAD_BTN_UNKNOWN;
        float m_Value = 0.0f;
    };

    class GamepadButtonReleasedEvent : public GamepadEvent
    {
    public:
        GamepadButtonReleasedEvent(int32_t gamepadID, GamepadButton button) : GamepadEvent(gamepadID), m_Button(button)
        {

        }

        GamepadButton GetButton() const { return m_Button; }

        TR_EVENT_CLASS_TYPE(GamepadButtonReleased)

    private:
        GamepadButton m_Button = Input::TR_PAD_BTN_UNKNOWN;
    };

    class GamepadAxisMovedEvent : public GamepadEvent
    {
    public:
        GamepadAxisMovedEvent(int32_t gamepadID, GamepadAxis axis, float value) : GamepadEvent(gamepadID), m_Axis(axis), m_Value(value)
        {

        }

        GamepadAxis GetAxis() const { return m_Axis; }
        float GetValue() const { return m_Value; }

        TR_EVENT_CLASS_TYPE(GamepadAxisMoved)

    private:
        GamepadAxis m_Axis = Input::TR_PAD_AXIS_UNKNOWN;
        float m_Value = 0.0f;
    };
}