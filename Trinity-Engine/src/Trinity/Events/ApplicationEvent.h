#pragma once

#include "Trinity/Events/Event.h"

#include <cstdint>
#include <sstream>

namespace Trinity
{
    class WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(uint32_t width, uint32_t height) : m_Width(width), m_Height(height)
        {

        }

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_Width << ", " << m_Height;
            
            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(WindowResize)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication | EventCategoryWindow)

    private:
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
    };

    class WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() = default;

        TR_EVENT_CLASS_TYPE(WindowClose)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication | EventCategoryWindow)
    };

    class WindowFocusEvent : public Event
    {
    public:
        WindowFocusEvent() = default;

        TR_EVENT_CLASS_TYPE(WindowFocus)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication | EventCategoryWindow)
    };

    class WindowLostFocusEvent : public Event
    {
    public:
        WindowLostFocusEvent() = default;

        TR_EVENT_CLASS_TYPE(WindowLostFocus)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication | EventCategoryWindow)
    };

    class WindowMovedEvent : public Event
    {
    public:
        WindowMovedEvent(int x, int y) : m_X(x), m_Y(y)
        {

        }

        int GetX() const { return m_X; }
        int GetY() const { return m_Y; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_X << ", " << m_Y;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(WindowMoved)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication | EventCategoryWindow)

    private:
        int m_X = 0;
        int m_Y = 0;
    };
}