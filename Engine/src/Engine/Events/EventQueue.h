#pragma once

#include "Engine/Events/Event.h"

#include <queue>
#include <mutex>
#include <memory>
#include <type_traits>
#include <utility>

namespace Engine
{
    class EventQueue
    {
    public:
        EventQueue() = default;

        template<typename T, typename... Args>
        void Enqueue(Args&&... args)
        {
            static_assert(std::is_base_of_v<Event, T>, "Enqueued type must derive from Event");
            std::scoped_lock lock(m_Mutex);
            m_Queue.emplace(std::make_unique<T>(std::forward<Args>(args)...));
        }

        std::unique_ptr<Event> Dequeue()
        {
            std::scoped_lock lock(m_Mutex);
            if (m_Queue.empty())
            {
                return nullptr;
            }

            auto e = std::move(m_Queue.front());
            m_Queue.pop();
            
            return e;
        }

        bool Empty() const
        {
            std::scoped_lock lock(m_Mutex);

            return m_Queue.empty();
        }

        void Clear()
        {
            std::scoped_lock lock(m_Mutex);
            while (!m_Queue.empty())
            {
                m_Queue.pop();
            }
        }

    private:
        mutable std::mutex m_Mutex;
        std::queue<std::unique_ptr<Event>> m_Queue;
    };
}