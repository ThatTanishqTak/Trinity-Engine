#pragma once

#include "Engine/Events/Event.h"
#include "Engine/Input/InputCodes.h"

#include <sstream>

namespace Engine
{
    class KeyEvent : public Event
    {
    public:
        KeyCode GetKeyCode() const { return m_KeyCode; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard)

    protected:
        explicit KeyEvent(KeyCode keycode) : m_KeyCode(keycode)
        {

        }

        KeyCode m_KeyCode = KeyCode::Unknown;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(KeyCode keycode, int repeatCount) : KeyEvent(keycode), m_RepeatCount(repeatCount)
        {

        }

        int GetRepeatCount() const { return m_RepeatCount; }

        std::string ToString() const override
        {
            std::stringstream ss;
            const int l_KeyValue = static_cast<int>(m_KeyCode);
            ss << GetName() << ": " << l_KeyValue << " (repeats: " << m_RepeatCount << ")";

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(KeyPressed)

    private:
        int m_RepeatCount = 0;
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        explicit KeyReleasedEvent(KeyCode keycode) : KeyEvent(keycode)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            const int l_KeyValue = static_cast<int>(m_KeyCode);
            ss << GetName() << ": " << l_KeyValue;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(KeyReleased)
    };

    class KeyTypedEvent : public KeyEvent
    {
    public:
        explicit KeyTypedEvent(KeyCode codepoint) : KeyEvent(codepoint)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            const int l_KeyValue = static_cast<int>(m_KeyCode);
            ss << GetName() << ": " << l_KeyValue;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(KeyTyped)
    };
}