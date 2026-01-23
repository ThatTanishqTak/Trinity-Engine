#include "Engine/Renderer/Renderer.h"

#include "Engine/Utilities/Utilities.h"
#include "Engine/Renderer/Vulkan/VulkanRenderer.h"

namespace Engine
{
    std::unique_ptr<Renderer> Renderer::Create()
    {
        TR_CORE_INFO("Renderer: Creating Vulkan backend...");

        return std::make_unique<VulkanRenderer>();
    }
}