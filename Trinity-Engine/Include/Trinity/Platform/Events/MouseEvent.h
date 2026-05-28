#pragma once

#include <string>

#include <Trinity/Core/Event.h>
#include <Trinity/Platform/MouseCodes.h>

namespace Trinity
{
    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(float x, float y) : m_X(x), m_Y(y)
        {

        }

        float GetX() const { return m_X; }
        float GetY() const { return m_Y; }

        std::string ToString() const override { return "MouseMovedEvent: " + std::to_string(m_X) + ", " + std::to_string(m_Y); }

        TR_EVENT_CLASS_TYPE(MouseMoved)
            TR_EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

    private:
        float m_X;
        float m_Y;
    };

    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(float offsetX, float offsetY) : m_OffsetX(offsetX), m_OffsetY(offsetY)
        {

        }

        float GetOffsetX() const { return m_OffsetX; }
        float GetOffsetY() const { return m_OffsetY; }

        std::string ToString() const override { return "MouseScrolledEvent: " + std::to_string(m_OffsetX) + ", " + std::to_string(m_OffsetY); }

        TR_EVENT_CLASS_TYPE(MouseScrolled)
            TR_EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

    private:
        float m_OffsetX;
        float m_OffsetY;
    };

    class MouseButtonEvent : public Event
    {
    public:
        MouseCode GetMouseButton() const { return m_Button; }

        TR_EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryMouseButton | EventCategoryInput)

    protected:
        MouseButtonEvent(MouseCode button) : m_Button(button)
        {

        }

        MouseCode m_Button;
    };

    class MouseButtonPressedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonPressedEvent(MouseCode button) : MouseButtonEvent(button)
        {

        }

        std::string ToString() const override { return "MouseButtonPressedEvent: " + std::to_string(m_Button); }

        TR_EVENT_CLASS_TYPE(MouseButtonPressed)
    };

    class MouseButtonReleasedEvent : public MouseButtonEvent
    {
    public:
        MouseButtonReleasedEvent(MouseCode button) : MouseButtonEvent(button)
        {

        }

        std::string ToString() const override { return "MouseButtonReleasedEvent: " + std::to_string(m_Button); }

        TR_EVENT_CLASS_TYPE(MouseButtonReleased)
    };
}