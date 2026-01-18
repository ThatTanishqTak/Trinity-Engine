#pragma once

#include "Engine/Events/Event.h"

namespace Engine
{
    class AppTickEvent : public Event
    {
    public:
        AppTickEvent() = default;

        TR_EVENT_CLASS_TYPE(AppTick)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class AppUpdateEvent : public Event
    {
    public:
        AppUpdateEvent() = default;

        TR_EVENT_CLASS_TYPE(AppUpdate)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class AppRenderEvent : public Event
    {
    public:
        AppRenderEvent() = default;

        TR_EVENT_CLASS_TYPE(AppRender)
            TR_EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
}