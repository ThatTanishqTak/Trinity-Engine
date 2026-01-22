#pragma once

#include "Engine/Events/Event.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

namespace Engine
{
    // GLFW standardized gamepad indices are used:
    // Buttons: 0..14, Axes: 0..5
    // (A/B/X/Y etc, plus sticks/triggers)
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
        int GetButton() const { return m_Button; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryGamepad)

    protected:
        GamepadButtonEvent(int gamepadID, int button) : GamepadEvent(gamepadID), m_Button(button)
        {

        }

        int m_Button = -1;
    };

    class GamepadButtonPressedEvent : public GamepadButtonEvent
    {
    public:
        GamepadButtonPressedEvent(int gamepadID, int button) : GamepadButtonEvent(gamepadID, button)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": id=" << m_GamepadID << ", button=" << m_Button;
            
            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadButtonPressed)
    };

    class GamepadButtonReleasedEvent : public GamepadButtonEvent
    {
    public:
        GamepadButtonReleasedEvent(int gamepadID, int button) : GamepadButtonEvent(gamepadID, button)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": id=" << m_GamepadID << ", button=" << m_Button;
            
            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadButtonReleased)
    };

    class GamepadAxisMovedEvent : public GamepadEvent
    {
    public:
        GamepadAxisMovedEvent(int gamepadID, int axis, float value) : GamepadEvent(gamepadID), m_Axis(axis), m_Value(value)
        {

        }

        int GetAxis() const { return m_Axis; }
        float GetValue() const { return m_Value; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": id=" << m_GamepadID << ", axis=" << m_Axis << ", value=" << m_Value;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(GamepadAxisMoved)
            TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryGamepad)

    private:
        int m_Axis = -1;
        float m_Value = 0.0f;
    };
}