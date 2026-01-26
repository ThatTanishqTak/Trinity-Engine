#include "Engine/Renderer/Pass/RenderPassManager.h"

#include "Engine/Renderer/Vulkan/VulkanDescriptors.h"
#include "Engine/Renderer/Vulkan/VulkanRenderer.h"
#include "Engine/Utilities/Utilities.h"

#include <cstring>

namespace Engine
{
    namespace
    {
        class ScopedPassCreationGuard
        {
        public:
            ScopedPassCreationGuard()
            {
                VulkanDescriptors::SetPassCreationInProgress(true);
            }

            ~ScopedPassCreationGuard()
            {
                VulkanDescriptors::SetPassCreationInProgress(false);
            }

            ScopedPassCreationGuard(const ScopedPassCreationGuard&) = delete;
            ScopedPassCreationGuard& operator=(const ScopedPassCreationGuard&) = delete;
            ScopedPassCreationGuard(ScopedPassCreationGuard&&) = delete;
            ScopedPassCreationGuard& operator=(ScopedPassCreationGuard&&) = delete;
        };

        class ScopedPassRecordGuard
        {
        public:
            ScopedPassRecordGuard()
            {
                VulkanDescriptors::SetPassRecordInProgress(true);
            }

            ~ScopedPassRecordGuard()
            {
                VulkanDescriptors::SetPassRecordInProgress(false);
            }

            ScopedPassRecordGuard(const ScopedPassRecordGuard&) = delete;
            ScopedPassRecordGuard& operator=(const ScopedPassRecordGuard&) = delete;
            ScopedPassRecordGuard(ScopedPassRecordGuard&&) = delete;
            ScopedPassRecordGuard& operator=(ScopedPassRecordGuard&&) = delete;
        };
    }

    void RenderPassManager::AddPass(std::unique_ptr<IRenderPass> pass)
    {
        // Order of operations: register passes before creation.
        if (!pass)
        {
            return;
        }

        m_Passes.emplace_back(std::move(pass));
    }

    bool RenderPassManager::RemovePass(const char* passName)
    {
        // Order of operations: remove passes after destruction or before creation.
        if (!passName)
        {
            return false;
        }

        for (auto it_Pass = m_Passes.begin(); it_Pass != m_Passes.end(); ++it_Pass)
        {
            if (std::strcmp((*it_Pass)->GetName(), passName) == 0)
            {
                m_Passes.erase(it_Pass);

                return true;
            }
        }

        return false;
    }

    void RenderPassManager::OnCreateAll(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources)
    {
        // Order of operations: create all pass resources before any recording.
        if (m_IsCreated)
        {
            assert(false && "RenderPassManager::OnCreateAll called when passes are already created");

            return;
        }

        size_t l_PassIndex = 0;
        for (const auto& it_Pass : m_Passes)
        {
            TR_CORE_INFO("Creating render pass {}", l_PassIndex);
            it_Pass->Initialize(device, swapchain, frameResources);
            ++l_PassIndex;
        }

        m_IsCreated = true;
    }

    void RenderPassManager::OnDestroyAll(VulkanDevice& device)
    {
        // Order of operations: destroy all pass resources before renderer shutdown.
        if (!m_IsCreated)
        {
            assert(false && "RenderPassManager::OnDestroyAll called when passes are not created");

            return;
        }

        size_t l_PassIndex = 0;
        for (const auto& it_Pass : m_Passes)
        {
            TR_CORE_INFO("Destroying render pass {}", l_PassIndex);
            it_Pass->OnDestroy(device);
            ++l_PassIndex;
        }

        m_IsCreated = false;
    }

    void RenderPassManager::OnResizeAll(VulkanDevice& device, VulkanSwapchain& swapchain, VulkanFrameResources& frameResources, VulkanRenderer& renderer)
    {
        // Order of operations: resize pass resources after swapchain changes.
        if (!m_IsCreated)
        {
            return;
        }

        for (const auto& it_Pass : m_Passes)
        {
            const char* l_PassName = it_Pass->GetName();
            TR_CORE_INFO("Resizing render pass {}", l_PassName ? l_PassName : "Unknown");

            it_Pass->OnResize(device, swapchain, frameResources, renderer);
        }
    }


    void RenderPassManager::RecordAll(VkCommandBuffer command, uint32_t imageIndex, uint32_t currentFrame, VulkanRenderer& renderer)
    {
        // Order of operations: record pass commands inside a begun command buffer.
        if (!m_IsCreated)
        {
            return;
        }

        if (!renderer.IsFrameInProgress())
        {
            assert(false && "RenderPassManager::RecordAll called outside of a frame");
            return;
        }

        for (const auto& it_Pass : m_Passes)
        {
            it_Pass->RecordCommandBuffer(command, imageIndex, currentFrame, renderer);
        }
    }

    IRenderPass* RenderPassManager::FindPass(const char* passName)
    {
        if (!passName)
        {
            return nullptr;
        }

        for (const auto& it_Pass : m_Passes)
        {
            if (std::strcmp(it_Pass->GetName(), passName) == 0)
            {
                return it_Pass.get();
            }
        }

        return nullptr;
    }

    const IRenderPass* RenderPassManager::FindPass(const char* passName) const
    {
        if (!passName)
        {
            return nullptr;
        }

        for (const auto& it_Pass : m_Passes)
        {
            if (std::strcmp(it_Pass->GetName(), passName) == 0)
            {
                return it_Pass.get();
            }
        }

        return nullptr;
    }
}