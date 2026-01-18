#pragma once

#include "Engine/Events/Event.h"

#include <unordered_map>
#include <typeindex>
#include <vector>
#include <functional>
#include <type_traits>

namespace Engine
{
    class EventBus
    {
    public:
        template<typename T>
        using HandlerFn = std::function<void(T&)>;

        template<typename T>
        void Subscribe(const HandlerFn<T>& handler)
        {
            static_assert(std::is_base_of_v<Event, T>, "Subscribed type must derive from Event");

            auto& vec = m_Handlers[typeid(T)];
            vec.emplace_back([handler](Event& e)
                {
                    handler(static_cast<T&>(e));
                });
        }

        void SubscribeAny(const std::function<void(Event&)>& handler)
        {
            m_AnyHandlers.emplace_back(handler);
        }

        void Publish(Event& e)
        {
            for (auto& h : m_AnyHandlers)
            {
                h(e);
            }

            auto it = m_Handlers.find(typeid(e));
            if (it == m_Handlers.end())
            {
                return;
            }

            for (auto& h : it->second)
            {
                h(e);
            }
        }

    private:
        std::unordered_map<std::type_index, std::vector<std::function<void(Event&)>>> m_Handlers;
        std::vector<std::function<void(Event&)>> m_AnyHandlers;
    };
}