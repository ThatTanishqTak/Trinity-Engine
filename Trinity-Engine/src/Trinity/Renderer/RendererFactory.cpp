#include "Trinity/Renderer/RendererFactory.h"

#include "Trinity/Renderer/Vulkan/VulkanRenderer.h"
#include "Trinity/Utilities/Utilities.h"

namespace Trinity
{
	std::unique_ptr<Renderer> RendererFactory::Create(RendererAPI api)
	{
		switch (api)
		{
			case RendererAPI::VULKAN:
				return std::make_unique<VulkanRenderer>();

			case RendererAPI::NONE:
			case RendererAPI::MOLTENVK:
			case RendererAPI::DIRECTX:
			default:
				TR_CORE_CRITICAL("RendererFactory::Create: Unsupported RendererAPI selected");

				return nullptr;
		}
	}

	std::unique_ptr<Renderer> RendererFactory::CreateDefault()
	{
		return Create(RendererAPI::VULKAN);
	}
}