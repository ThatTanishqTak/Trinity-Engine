#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

#include <Trinity/Core/UUID.h>

namespace Trinity
{
    enum class ContactPhase : uint32_t
    {
        Begin = 0,
        End
    };

    // Events are queued during Step and drained afterwards; they are never dispatched from inside a backend callback
    struct ContactEvent
    {
        UUID A = UUID(0);
        UUID B = UUID(0);
        glm::vec3 Point{ 0.0f };
        glm::vec3 Normal{ 0.0f };
        float Impulse = 0.0f;
        ContactPhase Phase = ContactPhase::Begin;
    };

    struct TriggerEvent
    {
        UUID Trigger = UUID(0);
        UUID Other = UUID(0);
        ContactPhase Phase = ContactPhase::Begin;
    };

    struct PhysicsEventQueue
    {
        std::vector<ContactEvent> Contacts;
        std::vector<TriggerEvent> Triggers;

        void Clear()
        {
            Contacts.clear();
            Triggers.clear();
        }
    };
}