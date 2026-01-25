#include "Engine/Renderer/DeletionQueue.h"

namespace Engine
{
    void DeletionQueue::Initialize(uint32_t framesInFlight)
    {
        // Prepare queues for each frame in flight.
        m_Queues.clear();
        m_Queues.resize(framesInFlight);
    }

    void DeletionQueue::Reset()
    {
        // Drop any queued work and release memory.
        m_Queues.clear();
    }

    void DeletionQueue::Push(uint32_t frameIndex, std::function<void()>&& function)
    {
        if (frameIndex >= m_Queues.size())
        {
            return;
        }

        // Store the work to be executed after the frame fence signals.
        m_Queues[frameIndex].push_back(std::move(function));
    }

    size_t DeletionQueue::Flush(uint32_t frameIndex)
    {
        if (frameIndex >= m_Queues.size())
        {
            return 0;
        }

        std::vector<std::function<void()>> l_Functions = std::move(m_Queues[frameIndex]);
        m_Queues[frameIndex].clear();

        for (const std::function<void()>& l_Function : l_Functions)
        {
            l_Function();
        }

        return l_Functions.size();
    }
}