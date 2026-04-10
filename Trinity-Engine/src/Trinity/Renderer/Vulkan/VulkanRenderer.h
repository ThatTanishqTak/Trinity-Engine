#pragma once

#include "Trinity/Renderer/Renderer.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"

namespace Trinity
{
	class VulkanRenderer : public Renderer
	{
	public:
		VulkanRenderer() = default;
		~VulkanRenderer() = default;

		void Initialize() override;
		void Shutdown() override;

		void BeginFrame() override;
		void EndFrame() override;

	private:
		VulkanContext m_VulkanContext;
	};
}