#include "Trinity/Renderer/RendererFactory.h"

#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"
#include "Trinity/Renderer/MoltenVK/MoltenVKRenderer.h"
#include "Trinity/Renderer/DirectX/DirectXRenderer.h"

#include "Trinity/Utilities/Log.h"

namespace Trinity
{
	std::unique_ptr<Renderer> RendererFactory::Create(RendererAPI api)
	{
		switch (api)
		{
		case RendererAPI::VULKAN:
		{
#if defined(_WIN32)
			return std::make_unique<VulkanRenderer>();
#elif defined(__APPLE__)
			TR_CORE_WARN("RendererFactory: VULKAN requested on Apple platform — routing to MoltenVKRenderer");
			return std::make_unique<MoltenVKRenderer>();
#else
			TR_CORE_CRITICAL("RendererFactory::Create: VULKAN is not supported on this platform");
			return nullptr;
#endif
		}
		case RendererAPI::MOLTENVK:
			return std::make_unique<MoltenVKRenderer>();
		case RendererAPI::DIRECTX:
			return std::make_unique<DirectXRenderer>();
		case RendererAPI::NONE:
		default:
			TR_CORE_CRITICAL("RendererFactory::Create: Unsupported RendererAPI selected");

			return nullptr;
		}
	}
}