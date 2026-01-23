#pragma once

#include "Engine/Events/Event.h"
#include "Engine/Input/InputCodes.h"

#include <cstdint>
#include <sstream>

namespace Engine
{
    class KeyEvent : public Event
    {
    public:
        Code::KeyCode GetKeyCode() const { return m_KeyCode; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard)

    protected:
        explicit KeyEvent(Code::KeyCode keycode) : m_KeyCode(keycode)
        {

        }

        Code::KeyCode m_KeyCode = Code::KeyCode::UNKNOWN;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(Code::KeyCode keycode, int repeatCount) : KeyEvent(keycode), m_RepeatCount(repeatCount)
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
        explicit KeyReleasedEvent(Code::KeyCode keycode) : KeyEvent(keycode)
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

    // NOTE: Typed characters are Unicode codepoints, not physical key codes.
    class KeyTypedEvent : public Event
    {
    public:
        explicit KeyTypedEvent(uint32_t codepoint) : m_Codepoint(codepoint)
        {

        }

        uint32_t GetCodepoint() const { return m_Codepoint; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": codepoint=" << m_Codepoint;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(KeyTyped)
            TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard)

    private:
        uint32_t m_Codepoint = 0;
    };
}