#pragma once

#include "Trinity/Renderer/Vulkan/VulkanRenderPass.h"
#include "Trinity/Renderer/Vulkan/VulkanFrameContext.h"

#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Trinity
{
    class VulkanRenderGraph
    {
    public:
        template<typename T>
        T* AddPass(std::unique_ptr<T> pass)
        {
            static_assert(std::is_base_of_v<VulkanRenderPass, T>, "T must derive from VulkanRenderPass");

            T* l_RawPtr = pass.get();
            m_PassMap[std::type_index(typeid(T))] = l_RawPtr;
            m_Passes.push_back(std::move(pass));

            return l_RawPtr;
        }

        template<typename T>
        T* GetPass()
        {
            static_assert(std::is_base_of_v<VulkanRenderPass, T>, "T must derive from VulkanRenderPass");

            auto a_It = m_PassMap.find(std::type_index(typeid(T)));
            if (a_It == m_PassMap.end())
            {
                return nullptr;
            }

            return static_cast<T*>(a_It->second);
        }

        template<typename T>
        const T* GetPass() const
        {
            static_assert(std::is_base_of_v<VulkanRenderPass, T>, "T must derive from VulkanRenderPass");

            auto a_It = m_PassMap.find(std::type_index(typeid(T)));
            if (a_It == m_PassMap.end())
            {
                return nullptr;
            }

            return static_cast<const T*>(a_It->second);
        }

        void Execute(const VulkanFrameContext& frameContext);
        void Recreate(uint32_t width, uint32_t height);
        void Shutdown();

    private:
        std::vector<std::unique_ptr<VulkanRenderPass>> m_Passes;
        std::unordered_map<std::type_index, VulkanRenderPass*> m_PassMap;
    };
}