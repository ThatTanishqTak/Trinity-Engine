#pragma once

#include "Engine/Events/Event.h"
#include "Engine/Input/InputCodes.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

namespace Engine
{
    class GamepadEvent : public Event
    {
    public:
        int GetGamepadID() const { return m_GamepadID; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryGamepad)

    protected:
        explicit GamepadEvent(int gamepadID) : m_GamepadID(gamepadID)
        {

        }

        int m_GamepadID = -1;
    };

    class GamepadConnectedEvent : public GamepadEvent
    {
    public:
        GamepadConnectedEvent(int gamepadID, std::string name, bool isGamepadMapping)
            : GamepadEvent(gamepadID), m_Name(std::move(name)), m_IsGamepadMapping(isGamepadMapping)
        {

        }

        const std::string& GetNameString() const { return m_Name; }
        bool HasStandardMapping() const { return m_IsGamepadMapping; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": ID = " << m_GamepadID << ", name = \"" << m_Name << "\"" << ", mapped = " << (m_IsGamepadMapping ? "True" : "False");

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadConnected)

    private:
        std::string m_Name;
        bool m_IsGamepadMapping = false;
    };

    class GamepadDisconnectedEvent : public GamepadEvent
    {
    public:
        explicit GamepadDisconnectedEvent(int gamepadID) : GamepadEvent(gamepadID)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": ID = " << m_GamepadID;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadDisconnected)
    };

    class GamepadButtonEvent : public GamepadEvent
    {
    public:
        Code::GamepadButton GetButton() const { return m_Button; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryGamepad)

    protected:
        GamepadButtonEvent(int gamepadID, Code::GamepadButton button) : GamepadEvent(gamepadID), m_Button(button)
        {

        }

        Code::GamepadButton m_Button = Code::GamepadButton::TR_BUTTON_A;
    };

    class GamepadButtonPressedEvent : public GamepadButtonEvent
    {
    public:
        GamepadButtonPressedEvent(int gamepadID, Code::GamepadButton button) : GamepadButtonEvent(gamepadID, button)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            const int l_ButtonValue = static_cast<int>(m_Button);
            ss << GetName() << ": ID = " << m_GamepadID << ", Button = " << l_ButtonValue;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadButtonPressed)
    };

    class GamepadButtonReleasedEvent : public GamepadButtonEvent
    {
    public:
        GamepadButtonReleasedEvent(int gamepadID, Code::GamepadButton button) : GamepadButtonEvent(gamepadID, button)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            const int l_ButtonValue = static_cast<int>(m_Button);
            ss << GetName() << ": ID = " << m_GamepadID << ", Button = " << l_ButtonValue;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadButtonReleased)
    };

    class GamepadAxisMovedEvent : public GamepadEvent
    {
    public:
        GamepadAxisMovedEvent(int gamepadID, Code::GamepadAxis axis, float value) : GamepadEvent(gamepadID), m_Axis(axis), m_Value(value)
        {

        }

        Code::GamepadAxis GetAxis() const { return m_Axis; }
        float GetValue() const { return m_Value; }

        std::string ToString() const override
        {
            std::stringstream ss;
            const int l_AxisValue = static_cast<int>(m_Axis);
            ss << GetName() << ": ID = " << m_GamepadID << ", axis = " << l_AxisValue << ", value = " << m_Value;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadAxisMoved)
            TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryGamepad)

    private:
        Code::GamepadAxis m_Axis = Code::GamepadAxis::TR_AXIS_LEFTX;
        float m_Value = 0.0f;
    };
}