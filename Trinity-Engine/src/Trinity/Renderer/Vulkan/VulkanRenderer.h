#pragma once

#include "Trinity/Renderer/Renderer.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"
#include "Trinity/Renderer/Vulkan/VulkanSync.h"
#include "Trinity/Renderer/Vulkan/VulkanCommand.h"
#include "Trinity/Renderer/Vulkan/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/VulkanResourceStateTracker.h"

#include "Trinity/Geometry/Geometry.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <memory>

namespace Trinity
{
	class Window;
	class ImGuiLayer;

	class VulkanRenderer : public Renderer
	{
	public:
		struct Configuration
		{
			VulkanSwapchain::ColorOutputPolicy m_ColorOutputPolicy = VulkanSwapchain::ColorOutputPolicy::SDRsRGB;
		};

		VulkanRenderer();
		explicit VulkanRenderer(const Configuration& configuration);
		~VulkanRenderer();

		void SetWindow(Window& window) override;
		void SetConfiguration(const Configuration& configuration);

		void Initialize() override;
		void Shutdown() override;

		void Resize(uint32_t width, uint32_t height) override;

		void BeginFrame() override;
		void EndFrame() override;
		void RenderImGui(ImGuiLayer& imGuiLayer) override;
		void SetSceneViewportSize(uint32_t width, uint32_t height) override;
		void* GetSceneViewportHandle() const override;

		void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color) override;
		void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& viewProjection) override;
		void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection) override;

		VkInstance GetVulkanInstance() const { return m_Context.GetInstance(); }
		VkAllocationCallbacks* GetVulkanAllocator() const { return m_Context.GetAllocator(); }
		VkPhysicalDevice GetVulkanPhysicalDevice() const { return m_Device.GetPhysicalDevice(); }
		VkDevice GetVulkanDevice() const { return m_Device.GetDevice(); }
		VkQueue GetVulkanGraphicsQueue() const { return m_Device.GetGraphicsQueue(); }
		uint32_t GetVulkanGraphicsQueueFamilyIndex() const { return m_Device.GetGraphicsQueueFamilyIndex(); }
		uint32_t GetVulkanSwapchainImageCount() const { return m_Swapchain.GetImageCount(); }
		VkFormat GetVulkanSwapchainImageFormat() const { return m_Swapchain.GetImageFormat(); }
		VkCommandBuffer GetVulkanCurrentCommandBuffer() const { return m_Command.GetCommandBuffer(m_CurrentFrameIndex); }
		VkCommandPool GetVulkanCommandPool(uint32_t frameIndex) const { return m_Command.GetCommandPool(frameIndex); }
		bool IsFrameBegun() const { return m_FrameBegun; }

	private:
		enum class ImageTransitionPreset
		{
			Present,
			ColorAttachmentWrite,
			DepthAttachmentWrite,
			ShaderReadOnly,
			TransferSource,
			TransferDestination,
			GeneralComputeReadWrite
		};

		static VulkanImageTransitionState BuildTransitionState(ImageTransitionPreset preset);
		static VkImageSubresourceRange BuildColorSubresourceRange();

		void RecreateSwapchain(uint32_t width, uint32_t height);
		void TransitionImageResource(VkCommandBuffer commandBuffer, VkImage image, const VkImageSubresourceRange& subresourceRange, const VulkanImageTransitionState& newState);

		void RecreateSceneViewportTarget();
		void DestroySceneViewportTarget();
		void BeginPresentPass(VkCommandBuffer commandBuffer, const VkImageSubresourceRange& colorSubresourceRange, const VulkanImageTransitionState& colorAttachmentWriteState);

		void EnsurePrimitiveUploaded(Geometry::PrimitiveType primitive);
		void ValidateSceneColorPolicy() const;

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
		ImGuiLayer* m_ImGuiLayer = nullptr;
		bool m_ImGuiVulkanInitialized = false;

		VulkanResourceStateTracker m_ResourceStateTracker{};

		std::array<PrimitiveGpu, (size_t)Geometry::PrimitiveType::Count> m_Primitives{};

		std::string m_VertexShaderPath = "Assets/Shaders/Simple.vert.spv";
		std::string m_FragmentShaderPath = "Assets/Shaders/Simple.frag.spv";
		Configuration m_Configuration{};

		uint32_t m_SceneViewportWidth = 0;
		uint32_t m_SceneViewportHeight = 0;
		VkImage m_SceneViewportImage = VK_NULL_HANDLE;
		VkDeviceMemory m_SceneViewportImageMemory = VK_NULL_HANDLE;
		VkImageView m_SceneViewportImageView = VK_NULL_HANDLE;
		VkSampler m_SceneViewportSampler = VK_NULL_HANDLE;
		void* m_SceneViewportHandle = nullptr;
		bool m_ScenePassRecording = false;
		bool m_PresentPassRecording = false;
	};
}