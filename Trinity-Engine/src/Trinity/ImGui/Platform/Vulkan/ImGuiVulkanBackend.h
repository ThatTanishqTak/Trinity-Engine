#pragma once

// Internal header — never include this outside Trinity/ImGui/Platform/Vulkan/.
// ImGuiLayer.cpp is the only permitted consumer.

#include "Trinity/ImGui/ImGuiLayer.h"

#include <vulkan/vulkan.h>

#include <cstdint>

struct SDL_Window;

namespace Trinity
{
    class VulkanRendererAPI;

    struct ImGuiLayer::Impl
    {
        VkDevice         Device         = VK_NULL_HANDLE;
        VkDescriptorPool DescriptorPool = VK_NULL_HANDLE;
        VkSampler        DefaultSampler = VK_NULL_HANDLE;

        void Initialize(SDL_Window* window, VulkanRendererAPI& api);
        void Shutdown();

        void NewFrame();
        void Render();
        void ProcessPlatformEvent(const void* sdlEvent);

        uint64_t RegisterTexture(uint64_t opaqueImageViewHandle);
        void     UnregisterTexture(uint64_t textureID);

        VkSampler GetDefaultSampler() const { return DefaultSampler; }

    private:
        void CreateDescriptorPool();
        void CreateDefaultSampler();
    };
}
