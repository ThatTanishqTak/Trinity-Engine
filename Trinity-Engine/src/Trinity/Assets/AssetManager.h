#pragma once

#include "Trinity/Assets/AssetHandle.h"
#include "Trinity/Utilities/Log.h"

#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <typeindex>
#include <unordered_map>

namespace Trinity
{
    class AssetManager
    {
    public:
        static AssetManager& Get();

        void Initialize();
        void Shutdown();

        template<typename T>
        AssetHandle<T> Register(std::shared_ptr<T> asset, AssetUUID uuid = NullAssetUUID)
        {
            if (uuid == NullAssetUUID)
            {
                uuid = GenerateUUID();
            }

            {
                std::lock_guard<std::mutex> l_Lock(m_Mutex);
                m_Assets[uuid] = asset;
            }

            return AssetHandle<T>(uuid);
        }

        template<typename T>
        std::shared_ptr<T> Resolve(const AssetHandle<T>& handle) const
        {
            if (!handle)
            {
                return nullptr;
            }

            std::lock_guard<std::mutex> l_Lock(m_Mutex);
            auto a_It = m_Assets.find(handle.GetUUID());
            if (a_It == m_Assets.end())
            {
                TR_CORE_WARN("AssetManager::Resolve: unknown UUID {}", handle.GetUUID());

                return nullptr;
            }

            return std::static_pointer_cast<T>(a_It->second);
        }

        template<typename T>
        void LoadAsync(const std::string& path, std::function<std::shared_ptr<T>()> loader, std::function<void(AssetHandle<T>)> onComplete)
        {
            std::lock_guard<std::mutex> l_Lock(m_Mutex);
            m_Queue.push([this, loader, onComplete]()
                {
                    auto l_Asset = loader();
                    if (!l_Asset)
                    {
                        return;
                    }

                    const AssetHandle<T> l_Handle = Register<T>(l_Asset);

                    std::lock_guard<std::mutex> l_CompletionLock(m_CompletionMutex);
                    m_CompletionCallbacks.push([onComplete, l_Handle]()
                        {
                            onComplete(l_Handle);
                        });
                });

            m_WorkAvailable.notify_one();
        }

        void FlushCompletions();
        void Unload(AssetUUID uuid);
        bool IsLoaded(AssetUUID uuid) const;

        static AssetUUID GenerateUUID();

    private:
        AssetManager() = default;

        void WorkerThread();

    private:
        mutable std::mutex m_Mutex;
        std::unordered_map<AssetUUID, std::shared_ptr<void>> m_Assets;

        std::thread m_Worker;
        std::queue<std::function<void()>> m_Queue;
        std::condition_variable m_WorkAvailable;
        bool m_Shutdown = false;

        std::mutex m_CompletionMutex;
        std::queue<std::function<void()>> m_CompletionCallbacks;
    };
}