#pragma once

#include <functional>
#include <ostream>
#include <string>

namespace Trinity
{
    enum class EventType
    {
        None = 0,

        // Window
        WindowClose, 
        WindowResize, 
        WindowFocus, WindowLostFocus, 
        WindowMoved,

        // Keyboard
        KeyPressed, KeyReleased, KeyTyped,

        // Mouse
        MouseButtonPressed, MouseButtonReleased, 
        MouseMoved, 
        MouseScrolled,

        // Gamepad
        GamepadConnected, GamepadDisconnected,
        GamepadButtonPressed, GamepadButtonReleased,
        GamepadAxisMoved
    };

    enum EventCategory : int
    {
        None = 0,
        EventCategoryApplication = 1 << 0,
        EventCategoryInput = 1 << 1,
        EventCategoryKeyboard = 1 << 2,
        EventCategoryMouse = 1 << 3,
        EventCategoryMouseButton = 1 << 4,
        EventCategoryWindow = 1 << 5,
        EventCategoryGamepad = 1 << 6
    };

    class Event
    {
        friend class EventDispatcher;

    public:
        virtual ~Event() = default;

        bool Handled = false;

        virtual EventType GetEventType() const = 0;
        virtual const char* GetName() const = 0;
        virtual int GetCategoryFlags() const = 0;

        virtual std::string ToString() const { return GetName(); }

        bool IsInCategory(EventCategory category) const
        {
            return (GetCategoryFlags() & category) != 0;
        }
    };

    class EventDispatcher
    {
    public:
        explicit EventDispatcher(Event& event) : m_Event(event)
        {

        }

        template<typename T, typename F>
        bool Dispatch(const F& func)
        {
            if (m_Event.GetEventType() == T::GetStaticType())
            {
                m_Event.Handled |= func(static_cast<T&>(m_Event));

                return true;
            }

            return false;
        }

    private:
        Event& m_Event;
    };

    inline std::ostream& operator<<(std::ostream& os, const Event& e)
    {
        return os << e.ToString();
    }
}

// Event class helper macros
#define TR_EVENT_CLASS_TYPE(type) \
    static ::Trinity::EventType GetStaticType() { return ::Trinity::EventType::type; } \
    ::Trinity::EventType GetEventType() const override { return GetStaticType(); } \
    const char* GetName() const override { return #type; }

#define TR_EVENT_CLASS_CATEGORY(category) \
    int GetCategoryFlags() const override { return category; }

// Handy binder
#define TR_BIND_EVENT_FN(fn) [this](auto& e) -> bool { return this->fn(e); }