#pragma once

#include <cstdint>
#include <string>

namespace Engine
{
    enum class EventType : uint16_t
    {
        None = 0,

        // Window
        WindowClose, WindowResize, WindowMove, WindowFocus, WindowLostFocus,
        WindowMinimize, WindowMaximize, WindowDrop,

        // App
        AppTick, AppUpdate, AppRender,

        // Keyboard
        KeyPressed, KeyReleased, KeyTyped,

        // Mouse
        MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,

        // Gamepad
        GamepadConnected, GamepadDisconnected,
        GamepadButtonPressed, GamepadButtonReleased, GamepadAxisMoved
    };

    enum EventCategory : uint32_t
    {
        EventCategoryNone = 0,
        EventCategoryApplication = 1u << 0,
        EventCategoryWindow = 1u << 1,
        EventCategoryInput = 1u << 2,
        EventCategoryKeyboard = 1u << 3,
        EventCategoryMouse = 1u << 4,
        EventCategoryMouseButton = 1u << 5,
        EventCategoryGamepad = 1u << 6
    };

    enum Modifiers : uint16_t
    {
        Mod_None = 0,
        Mod_Shift = 1u << 0,
        Mod_Control = 1u << 1,
        Mod_Alt = 1u << 2,
        Mod_Super = 1u << 3,
        Mod_CapsLock = 1u << 4,
        Mod_NumLock = 1u << 5
    };

    class Event
    {
    public:
        virtual ~Event() = default;

        bool Handled = false;

        virtual EventType GetEventType() const = 0;
        virtual const char* GetName() const = 0;
        virtual uint32_t GetCategoryFlags() const = 0;

        virtual std::string ToString() const { return GetName(); }

        bool IsInCategory(EventCategory category) const
        {
            return (GetCategoryFlags() & category) != 0;
        }
    };

#define TR_EVENT_CLASS_TYPE(type) \
    static EventType GetStaticType() { return EventType::type; } \
    EventType GetEventType() const override { return GetStaticType(); } \
    const char* GetName() const override { return #type; }

#define TR_EVENT_CLASS_CATEGORY(categoryFlags) \
    uint32_t GetCategoryFlags() const override { return (categoryFlags); }
}