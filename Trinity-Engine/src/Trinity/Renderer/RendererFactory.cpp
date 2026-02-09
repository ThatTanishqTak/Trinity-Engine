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
				return std::make_unique<VulkanRenderer>();
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