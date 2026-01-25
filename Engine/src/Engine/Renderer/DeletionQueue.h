#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace Engine
{
    class DeletionQueue
    {
    public:
        DeletionQueue() = default;
        ~DeletionQueue() = default;

        DeletionQueue(const DeletionQueue&) = delete;
        DeletionQueue& operator=(const DeletionQueue&) = delete;
        DeletionQueue(DeletionQueue&&) = delete;
        DeletionQueue& operator=(DeletionQueue&&) = delete;

        void Initialize(uint32_t framesInFlight);
        void Reset();

        void Push(uint32_t frameIndex, std::function<void()>&& function);
        size_t Flush(uint32_t frameIndex);

    private:
        std::vector<std::vector<std::function<void()>>> m_Queues;
    };
}