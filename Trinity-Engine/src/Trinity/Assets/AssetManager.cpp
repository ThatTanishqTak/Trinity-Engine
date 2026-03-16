#include "Trinity/Assets/AssetManager.h"

#include <chrono>
#include <random>
#include <utility>

namespace Trinity
{
    AssetManager& AssetManager::Get()
    {
        static AssetManager s_Instance;

        return s_Instance;
    }

    void AssetManager::Initialize()
    {
        if (m_Worker.joinable())
        {
            TR_CORE_WARN("AssetManager::Initialize called while already initialized");
            return;
        }

        m_Shutdown = false;
        m_Worker = std::thread(&AssetManager::WorkerThread, this);

        TR_CORE_INFO("AssetManager initialized");
    }

    void AssetManager::Shutdown()
    {
        {
            std::lock_guard<std::mutex> l_Lock(m_Mutex);
            if (!m_Worker.joinable())
            {
                m_Assets.clear();

                std::lock_guard<std::mutex> l_CompletionLock(m_CompletionMutex);
                std::queue<std::function<void()>>{}.swap(m_CompletionCallbacks);

                return;
            }

            m_Shutdown = true;
        }

        m_WorkAvailable.notify_one();
        m_Worker.join();

        {
            std::lock_guard<std::mutex> l_Lock(m_Mutex);
            std::queue<std::function<void()>>{}.swap(m_Queue);
            m_Assets.clear();
        }

        {
            std::lock_guard<std::mutex> l_CompletionLock(m_CompletionMutex);
            std::queue<std::function<void()>>{}.swap(m_CompletionCallbacks);
        }

        m_Shutdown = false;

        TR_CORE_INFO("AssetManager shutdown");
    }

    void AssetManager::FlushCompletions()
    {
        std::lock_guard<std::mutex> l_Lock(m_CompletionMutex);
        while (!m_CompletionCallbacks.empty())
        {
            m_CompletionCallbacks.front()();
            m_CompletionCallbacks.pop();
        }
    }

    void AssetManager::Unload(AssetUUID uuid)
    {
        std::lock_guard<std::mutex> l_Lock(m_Mutex);
        m_Assets.erase(uuid);
    }

    bool AssetManager::IsLoaded(AssetUUID uuid) const
    {
        std::lock_guard<std::mutex> l_Lock(m_Mutex);

        return m_Assets.find(uuid) != m_Assets.end();
    }

    AssetUUID AssetManager::GenerateUUID()
    {
        static std::mt19937_64 s_Engine(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        static std::uniform_int_distribution<AssetUUID> s_Distribution(1, UINT64_MAX);

        return s_Distribution(s_Engine);
    }

    void AssetManager::WorkerThread()
    {
        while (true)
        {
            std::function<void()> l_Job;

            {
                std::unique_lock<std::mutex> l_Lock(m_Mutex);
                m_WorkAvailable.wait(l_Lock, [this]()
                    {
                        return !m_Queue.empty() || m_Shutdown;
                    });

                if (m_Shutdown && m_Queue.empty())
                {
                    return;
                }

                l_Job = std::move(m_Queue.front());
                m_Queue.pop();
            }

            l_Job();
        }
    }
}