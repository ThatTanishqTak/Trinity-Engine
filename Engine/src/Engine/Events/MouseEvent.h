#pragma once

#include "Engine/Events/Event.h"

#include <sstream>

namespace Engine
{
    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(float x, float y) : m_MouseX(x), m_MouseY(y)
        {

        }

        float GetX() const { return m_MouseX; }
        float GetY() const { return m_MouseY; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_MouseX << ", " << m_MouseY;
            
            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(MouseMoved)
            TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)

    private:
        float m_MouseX = 0.0f;
        float m_MouseY = 0.0f;
    };

    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(float xOffset, float yOffset) : m_XOffset(xOffset), m_YOffset(yOffset)
        {

        }

        float GetXOffset() const { return m_XOffset; }
        float GetYOffset() const { return m_YOffset; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_XOffset << ", " << m_YOffset;
            
            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(MouseScrolled)
            TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)

    private:
        float m_XOffset = 0.0f;
        float m_YOffset = 0.0f;
    };

    class MouseButtonEvent : public Event
    {
    public:
        int GetMouseButton() const { return m_Button; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse | EventCategoryMouseButton)

    protected:
        explicit MouseButtonEvent(int button) : m_Button(button)
        {

        }

        int m_Button = 0;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        explicit MouseButtonPressedEvent(int button) : MouseButtonEvent(button)
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
        explicit MouseButtonReleasedEvent(int button) : MouseButtonEvent(button)
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