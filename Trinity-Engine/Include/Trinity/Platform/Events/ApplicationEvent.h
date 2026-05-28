#pragma once

#include <cstdint>
#include <string>

#include <Trinity/Core/Event.h>

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

        std::string ToString() const override { return "WindowResizeEvent: " + std::to_string(m_Width) + ", " + std::to_string(m_Height); }

        TR_EVENT_CLASS_TYPE(WindowResize)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication)

    private:
        uint32_t m_Width;
        uint32_t m_Height;
    };

    class WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() = default;

        TR_EVENT_CLASS_TYPE(WindowClose)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class WindowFocusEvent : public Event
    {
    public:
        WindowFocusEvent() = default;

        TR_EVENT_CLASS_TYPE(WindowFocus)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class WindowLostFocusEvent : public Event
    {
    public:
        WindowLostFocusEvent() = default;

        TR_EVENT_CLASS_TYPE(WindowLostFocus)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class WindowMovedEvent : public Event
    {
    public:
        WindowMovedEvent(int32_t x, int32_t y) : m_X(x), m_Y(y)
        {

        }

        int32_t GetX() const { return m_X; }
        int32_t GetY() const { return m_Y; }

        std::string ToString() const override
        {
            return "WindowMovedEvent: " + std::to_string(m_X) + ", " + std::to_string(m_Y);
        }

        TR_EVENT_CLASS_TYPE(WindowMoved)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication)

    private:
        int32_t m_X;
        int32_t m_Y;
    };
}