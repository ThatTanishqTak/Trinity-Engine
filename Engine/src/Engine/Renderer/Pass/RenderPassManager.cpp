#include "Engine/Renderer/Pass/RenderPassManager.h"

#include "Engine/Renderer/Pass/IRenderPass.h"
#include "Engine/Utilities/Utilities.h"

#include <cstring>

namespace Engine
{
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

    void RenderPassManager::OnCreateAll(VulkanRenderer& renderer)
    {
        // Order of operations: create all pass resources before any recording.
        if (m_IsCreated)
        {
            return;
        }

        for (const auto& it_Pass : m_Passes)
        {
            TR_CORE_INFO("Creating render pass: {}", it_Pass->GetName());
            it_Pass->OnCreate(renderer);
        }

        m_IsCreated = true;
    }

    void RenderPassManager::OnDestroyAll(VulkanRenderer& renderer)
    {
        // Order of operations: destroy all pass resources before renderer shutdown.
        if (!m_IsCreated)
        {
            return;
        }

        for (const auto& it_Pass : m_Passes)
        {
            TR_CORE_INFO("Destroying render pass: {}", it_Pass->GetName());
            it_Pass->OnDestroy(renderer);
        }

        m_IsCreated = false;
    }

    void RenderPassManager::OnResizeAll(VulkanRenderer& renderer)
    {
        // Order of operations: resize pass resources after swapchain changes.
        if (!m_IsCreated)
        {
            return;
        }

        for (const auto& it_Pass : m_Passes)
        {
            it_Pass->OnResize(renderer);
        }
    }

    void RenderPassManager::RecordAll(VulkanRenderer& renderer, VkCommandBuffer command, uint32_t imageIndex, VulkanFrameResources::PerFrame& frame)
    {
        // Order of operations: record pass commands inside a begun command buffer.
        if (!m_IsCreated)
        {
            return;
        }

        for (const auto& it_Pass : m_Passes)
        {
            it_Pass->Record(renderer, command, imageIndex, frame);
        }
    }
}