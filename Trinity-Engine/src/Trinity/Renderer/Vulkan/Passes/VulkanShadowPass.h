#pragma once

#include "Trinity/Renderer/RenderPass.h"
#include "Trinity/Renderer/Vulkan/Resources/VulkanFramebuffer.h"

#include <string>
#include <memory>

namespace Trinity
{
    class VulkanRendererAPI;

    class VulkanShadowPass final : public RenderPass
    {
    public:
        VulkanShadowPass(VulkanRendererAPI& renderer, uint32_t shadowMapSize = 2048);
        ~VulkanShadowPass() override = default;

        void Begin(uint32_t width, uint32_t height) override;
        void End() override;
        void Resize(uint32_t width, uint32_t height) override;

        std::shared_ptr<Framebuffer> GetFramebuffer() const override { return m_Framebuffer; }
        std::shared_ptr<Texture> GetShadowMap() const;

    private:
        VulkanRendererAPI* m_Renderer = nullptr;
        std::shared_ptr<VulkanFramebuffer> m_Framebuffer;
        uint32_t m_ShadowMapSize = 2048;
        std::string m_Name;
    };
}