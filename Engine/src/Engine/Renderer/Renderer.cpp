#include "Engine/Renderer/Renderer.h"
#include "Engine/Platform/Window.h"
#include "Engine/Core/Utilities.h"

#include "Engine/Renderer/Vulkan/VulkanRenderer.h"

namespace Engine
{
    namespace
    {
        VulkanRenderer* s_Vulkan = nullptr;
    }

    namespace Renderer
    {
        void Initialize(Window& window)
        {
            TR_CORE_INFO("Renderer: Initializing Vulkan backend...");

            if (s_Vulkan)
            {
                return;
            }

            s_Vulkan = new VulkanRenderer();
            s_Vulkan->Initialize(window);
        }

        void Shutdown()
        {
            if (!s_Vulkan)
            {
                return;
            }

            TR_CORE_INFO("Renderer: Shutting down Vulkan backend...");
            s_Vulkan->Shutdown();
            delete s_Vulkan;
            s_Vulkan = nullptr;
        }

        void BeginFrame()
        {
            if (s_Vulkan)
            {
                s_Vulkan->BeginFrame();
            }
        }

        void EndFrame()
        {
            if (s_Vulkan)
            {
                s_Vulkan->EndFrame();
            }
        }

        void OnResize(uint32_t width, uint32_t height)
        {
            if (s_Vulkan)
            {
                s_Vulkan->OnResize(width, height);
            }
        }
    }
}