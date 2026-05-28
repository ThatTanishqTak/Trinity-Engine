#pragma once

#include <string>

#include <Trinity/Core/Event.h>
#include <Trinity/Platform/KeyCodes.h>

namespace Trinity
{
    class KeyEvent : public Event
    {
    public:
        KeyCode GetKeyCode() const { return m_KeyCode; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

    protected:
        KeyEvent(KeyCode keyCode) : m_KeyCode(keyCode)
        {

        }

        KeyCode m_KeyCode;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(KeyCode keyCode, bool repeat = false) : KeyEvent(keyCode), m_Repeat(repeat)
        {

        }

        bool IsRepeat() const { return m_Repeat; }

        std::string ToString() const override { return "KeyPressedEvent: " + std::to_string(m_KeyCode) + (m_Repeat ? " (repeat)" : ""); }

        TR_EVENT_CLASS_TYPE(KeyPressed)

    private:
        bool m_Repeat;
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(KeyCode keyCode) : KeyEvent(keyCode)
        {

        }

        std::string ToString() const override { return "KeyReleasedEvent: " + std::to_string(m_KeyCode); }

        TR_EVENT_CLASS_TYPE(KeyReleased)
    };

    class KeyTypedEvent : public KeyEvent
    {
    public:
        KeyTypedEvent(KeyCode keyCode)
            : KeyEvent(keyCode)
        {
        }

        std::string ToString() const override { return "KeyTypedEvent: " + std::to_string(m_KeyCode); }

        TR_EVENT_CLASS_TYPE(KeyTyped)
    };
}