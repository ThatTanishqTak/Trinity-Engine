#include "Trinity/Events/EventQueue.h"

#include "Trinity/Events/Event.h"

namespace Trinity
{
    void EventQueue::PushEvent(std::unique_ptr<Event> event)
    {
        if (!event)
        {
            return;
        }

        m_Events.push_back(std::move(event));
    }

    bool EventQueue::TryPopEvent(std::unique_ptr<Event>& event)
    {
        if (m_Events.empty())
        {
            return false;
        }

        event = std::move(m_Events.front());
        m_Events.pop_front();

        return true;
    }

    bool EventQueue::IsEmpty() const
    {
        return m_Events.empty();
    }
}