#pragma once

#include "Engine/Events/Event.h"
#include "Engine/Events/Input/MouseCodes.h"

#include <sstream>

namespace Engine
{
    using MouseButton = Input::MouseButton;

    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(double x, double y) : m_MouseX(x), m_MouseY(y)
        {

        }

        double GetX() const { return m_MouseX; }
        double GetY() const { return m_MouseY; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_MouseX << ", " << m_MouseY;
            
            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(MouseMoved)
            TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)

    private:
        double m_MouseX = 0.0, m_MouseY = 0.0;
    };

    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(double xOffset, double yOffset) : m_XOffset(xOffset), m_YOffset(yOffset)
        {

        }

        double GetXOffset() const { return m_XOffset; }
        double GetYOffset() const { return m_YOffset; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_XOffset << ", " << m_YOffset;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(MouseScrolled)
            TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)

    private:
        double m_XOffset = 0.0, m_YOffset = 0.0;
    };

    class MouseButtonEvent : public Event
    {
    public:
        MouseButton GetMouseButton() const { return m_Button; }
        uint16_t GetMods() const { return m_Mods; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse | EventCategoryMouseButton)

    protected:
        MouseButtonEvent(MouseButton button, uint16_t mods) : m_Button(button), m_Mods(mods)
        {

        }

        MouseButton m_Button;
        uint16_t m_Mods = Mod_None;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(MouseButton button, uint16_t mods) : MouseButtonEvent(button, mods)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_Button;
            
            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(MouseButtonPressed)
    };

    class MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(MouseButton button, uint16_t mods) : MouseButtonEvent(button, mods)
        {

        }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_Button;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(MouseButtonReleased)
    };
}