#pragma once

#include "Engine/Events/Event.h"
#include <vector>

namespace Engine
{
    class WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() = default;

        TR_EVENT_CLASS_TYPE(WindowClose)
            TR_EVENT_CLASS_CATEGORY(EventCategoryWindow | EventCategoryApplication)
    };

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
            TR_EVENT_CLASS_CATEGORY(EventCategoryWindow | EventCategoryApplication)

    private:
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
    };

    class WindowMoveEvent : public Event
    {
    public:
        WindowMoveEvent(int32_t x, int32_t y) : m_X(x), m_Y(y) {}

        int32_t GetX() const { return m_X; }
        int32_t GetY() const { return m_Y; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_X << ", " << m_Y;

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(WindowMove)
            TR_EVENT_CLASS_CATEGORY(EventCategoryWindow | EventCategoryApplication)

    private:
        int32_t m_X = 0;
        int32_t m_Y = 0;
    };

    class WindowFocusEvent : public Event
    {
    public:
        WindowFocusEvent() = default;
        TR_EVENT_CLASS_TYPE(WindowFocus)
            TR_EVENT_CLASS_CATEGORY(EventCategoryWindow | EventCategoryApplication)
    };

    class WindowLostFocusEvent : public Event
    {
    public:
        WindowLostFocusEvent() = default;
        TR_EVENT_CLASS_TYPE(WindowLostFocus)
            TR_EVENT_CLASS_CATEGORY(EventCategoryWindow | EventCategoryApplication)
    };

    class WindowMinimizeEvent : public Event
    {
    public:
        WindowMinimizeEvent() = default;
        TR_EVENT_CLASS_TYPE(WindowMinimize)
            TR_EVENT_CLASS_CATEGORY(EventCategoryWindow | EventCategoryApplication)
    };

    class WindowMaximizeEvent : public Event
    {
    public:
        WindowMaximizeEvent() = default;
        TR_EVENT_CLASS_TYPE(WindowMaximize)
            TR_EVENT_CLASS_CATEGORY(EventCategoryWindow | EventCategoryApplication)
    };

    class WindowDropEvent : public Event
    {
    public:
        WindowDropEvent(std::vector<std::string> paths) : m_Paths(std::move(paths))
        {

        }

        const std::vector<std::string>& GetPaths() const { return m_Paths; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << GetName() << ": " << m_Paths.size() << " file(s)";

            return ss.str();
        }

        TR_EVENT_CLASS_TYPE(WindowDrop)
            TR_EVENT_CLASS_CATEGORY(EventCategoryWindow | EventCategoryApplication)

    private:
        std::vector<std::string> m_Paths;
    };
}