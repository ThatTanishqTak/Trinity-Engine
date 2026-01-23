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
        GamepadConnectedEvent(int gamepadID, std::string name, bool isGamepadMapping) : GamepadEvent(gamepadID), m_Name(std::move(name)), m_IsGamepadMapping(isGamepadMapping)
        {

        }

        const std::string& GetNameString() const { return m_Name; }
        bool HasStandardMapping() const { return m_IsGamepadMapping; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": id = " << m_GamepadID << ", name = \"" << m_Name << "\"" << ", mapped = " << (m_IsGamepadMapping ? "True" : "False");
            
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
            ss << GetName() << ": id=" << m_GamepadID;
            
            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadDisconnected)
    };

    class GamepadButtonEvent : public GamepadEvent
    {
    public:
        GamepadCode GetButton() const { return m_Button; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryGamepad)

    protected:
        GamepadButtonEvent(int gamepadID, GamepadCode button) : GamepadEvent(gamepadID), m_Button(button)
        {

        }

        GamepadCode m_Button = GamepadCode::TR_BUTTONA;
    };

    class GamepadButtonPressedEvent : public GamepadButtonEvent
    {
    public:
        GamepadButtonPressedEvent(int gamepadID, GamepadCode button) : GamepadButtonEvent(gamepadID, button)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            const int l_ButtonValue = static_cast<int>(m_Button);
            ss << GetName() << ": id=" << m_GamepadID << ", button=" << l_ButtonValue;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadButtonPressed)
    };

    class GamepadButtonReleasedEvent : public GamepadButtonEvent
    {
    public:
        GamepadButtonReleasedEvent(int gamepadID, GamepadCode button) : GamepadButtonEvent(gamepadID, button)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            const int l_ButtonValue = static_cast<int>(m_Button);
            ss << GetName() << ": id=" << m_GamepadID << ", button=" << l_ButtonValue;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadButtonReleased)
    };

    class GamepadAxisMovedEvent : public GamepadEvent
    {
    public:
        GamepadAxisMovedEvent(int gamepadID, GamepadCode axis, float value) : GamepadEvent(gamepadID), m_Axis(axis), m_Value(value)
        {

        }

        GamepadCode GetAxis() const { return m_Axis; }
        float GetValue() const { return m_Value; }

        std::string ToString() const override
        {
            std::stringstream ss;
            const int l_AxisValue = static_cast<int>(m_Axis);
            ss << GetName() << ": id=" << m_GamepadID << ", axis=" << l_AxisValue << ", value=" << m_Value;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadAxisMoved)
            TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryGamepad)

    private:
        GamepadCode m_Axis = GamepadCode::TR_AXISLEFTX;
        float m_Value = 0.0f;
    };
}