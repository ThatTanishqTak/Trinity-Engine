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

        std::lock_guard<std::mutex> l_Lock(m_Mutex);
        m_Events.push_back(std::move(event));
    }

    bool EventQueue::TryPopEvent(std::unique_ptr<Event>& event)
    {
        std::lock_guard<std::mutex> l_Lock(m_Mutex);

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
        std::lock_guard<std::mutex> l_Lock(m_Mutex);
 
        return m_Events.empty();
    }
}