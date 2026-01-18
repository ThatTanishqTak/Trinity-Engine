#pragma once

#include "Engine/Events/Event.h"
#include "Engine/Events/Input/KeyCodes.h"

#include <sstream>

namespace Engine
{
    using KeyCode = Input::KeyCode;

    class KeyEvent : public Event
    {
    public:
        KeyCode GetKeyCode() const { return m_KeyCode; }
        uint16_t GetMods() const { return m_Mods; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard)

    protected:
        KeyEvent(KeyCode keycode, uint16_t mods) : m_KeyCode(keycode), m_Mods(mods)
        {

        }

        KeyCode m_KeyCode;
        uint16_t m_Mods = Mod_None;
    };

    class KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(KeyCode keycode, uint16_t mods, uint16_t repeatCount) : KeyEvent(keycode, mods), m_RepeatCount(repeatCount)
        {

        }

        uint16_t GetRepeatCount() const { return m_RepeatCount; }
        bool IsRepeat() const { return m_RepeatCount > 0; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_KeyCode << " (repeat=" << m_RepeatCount << ")";

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(KeyPressed)

    private:
        uint16_t m_RepeatCount = 0;
    };

    class KeyReleasedEvent : public KeyEvent
    {
    public:
        KeyReleasedEvent(KeyCode keycode, uint16_t mods) : KeyEvent(keycode, mods)
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

    class KeyTypedEvent : public Event
    {
    public:
        KeyTypedEvent(uint32_t codepoint) : m_Codepoint(codepoint)
        {

        }

        uint32_t GetCodepoint() const { return m_Codepoint; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_Codepoint;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(KeyTyped)
            TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard)

    private:
        uint32_t m_Codepoint = 0;
    };
}