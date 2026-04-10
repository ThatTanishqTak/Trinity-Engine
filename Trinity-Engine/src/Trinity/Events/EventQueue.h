#pragma once

#include "Trinity/Events/Event.h"

#include <deque>
#include <memory>
#include <mutex>

namespace Trinity
{
    class EventQueue
    {
    public:
        void PushEvent(std::unique_ptr<Event> event);
        bool TryPopEvent(std::unique_ptr<Event>& event);
        bool IsEmpty() const;

    private:
        mutable std::mutex m_Mutex;
        std::deque<std::unique_ptr<Event>> m_Events;
    };
}