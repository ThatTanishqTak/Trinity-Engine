#pragma once

#include "Trinity/Renderer/RenderPass.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanFramebuffer.h"

#include <string>
#include <memory>

namespace Trinity
{
    class VulkanRendererAPI;

    class VulkanGeometryPass final : public RenderPass
    {
    public:
        VulkanGeometryPass(VulkanRendererAPI& renderer, uint32_t width, uint32_t height);
        ~VulkanGeometryPass() override = default;

        void Begin(uint32_t width, uint32_t height) override;
        void End() override;
        void Resize(uint32_t width, uint32_t height) override;

        std::shared_ptr<Framebuffer> GetFramebuffer() const override { return m_Framebuffer; }

    private:
        VulkanRendererAPI* m_Renderer = nullptr;
        std::shared_ptr<VulkanFramebuffer> m_Framebuffer;
        std::string m_Name;
    };
}