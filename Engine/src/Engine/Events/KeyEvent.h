#pragma once

#include "Engine/Events/Event.h"

#include <sstream>

namespace Engine
{
    class KeyEvent : public Event
    {
    public:
        int GetKeyCode() const { return m_KeyCode; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard)

    protected:
        explicit KeyEvent(int keycode) : m_KeyCode(keycode)
        {

        }

        int m_KeyCode = 0;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(int keycode, int repeatCount) : KeyEvent(keycode), m_RepeatCount(repeatCount)
        {

        }

        int GetRepeatCount() const { return m_RepeatCount; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_KeyCode << " (repeats: " << m_RepeatCount << ")";
            
            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(KeyPressed)

    private:
        int m_RepeatCount = 0;
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        explicit KeyReleasedEvent(int keycode) : KeyEvent(keycode)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_KeyCode;
            
            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(KeyReleased)
    };

    class KeyTypedEvent : public KeyEvent
    {
    public:
        explicit KeyTypedEvent(int codepoint) : KeyEvent(codepoint)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_KeyCode;
 
            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(KeyTyped)
    };
}