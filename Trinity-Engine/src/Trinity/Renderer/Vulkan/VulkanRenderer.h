#pragma once

#include "Trinity/Renderer/Renderer.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"
#include "Trinity/Renderer/Vulkan/VulkanSync.h"
#include "Trinity/Renderer/Vulkan/VulkanCommand.h"
#include "Trinity/Renderer/Vulkan/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"

#include "Trinity/Geometry/Geometry.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <memory>

namespace Trinity
{
	class Window;

	class VulkanRenderer : public Renderer
	{
	public:
		VulkanRenderer();
		~VulkanRenderer();

		void SetWindow(Window& window) override;

		void Initialize() override;
		void Shutdown() override;

		void Resize(uint32_t width, uint32_t height) override;

		void BeginFrame() override;
		void EndFrame() override;

		void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color) override;

	private:
		void RecreateSwapchain(uint32_t width, uint32_t height);
		void TransitionSwapchainImage(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkImageLayout newLayout);

		void EnsurePrimitiveUploaded(Geometry::PrimitiveType primitive);

	private:
		struct PrimitiveGpu
		{
			std::unique_ptr<VulkanVertexBuffer> VulkanVB;
			std::unique_ptr<VulkanIndexBuffer> VulkanIB;
		};

		Window* m_Window = nullptr;

		VulkanContext m_Context{};
		VulkanDevice m_Device{};
		VulkanSwapchain m_Swapchain{};
		VulkanSync m_Sync{};
		VulkanCommand m_Command{};
		VulkanPipeline m_Pipeline{};

		uint32_t m_FramesInFlight = 2;
		uint32_t m_CurrentFrameIndex = 0;
		uint32_t m_CurrentImageIndex = 0;
		bool m_FrameBegun = false;

		std::vector<VkImageLayout> m_SwapchainImageLayouts{};

		std::array<PrimitiveGpu, (size_t)Geometry::PrimitiveType::Count> m_Primitives{};

		std::string m_VertexShaderPath = "Assets/Shaders/Simple.vert.spv";
		std::string m_FragmentShaderPath = "Assets/Shaders/Simple.frag.spv";
	};
}