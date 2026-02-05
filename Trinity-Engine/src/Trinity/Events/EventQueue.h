#pragma once

#include <deque>
#include <memory>

namespace Trinity
{
    class Event;

    class EventQueue
    {
    public:
        void PushEvent(std::unique_ptr<Event> event);
        bool TryPopEvent(std::unique_ptr<Event>& event);
        bool IsEmpty() const;

    private:
        std::deque<std::unique_ptr<Event>> m_Events;
    };
}