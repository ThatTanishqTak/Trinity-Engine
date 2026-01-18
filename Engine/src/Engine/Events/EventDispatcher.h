#pragma once

#include "Engine/Events/Event.h"

namespace Engine
{
    class EventDispatcher
    {
    public:
        explicit EventDispatcher(Event& e) : m_Event(e)
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
}