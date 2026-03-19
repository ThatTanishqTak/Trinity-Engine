#pragma once

#include "Trinity/Renderer/Renderer.h"
#include "Trinity/Renderer/ShaderLibrary.h"
#include "Trinity/Renderer/Texture.h"

#include "Trinity/Renderer/Vulkan/VulkanContext.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"
#include "Trinity/Renderer/Vulkan/VulkanAllocator.h"
#include "Trinity/Renderer/Vulkan/VulkanSwapchain.h"
#include "Trinity/Renderer/Vulkan/VulkanSync.h"
#include "Trinity/Renderer/Vulkan/VulkanCommand.h"
#include "Trinity/Renderer/Vulkan/VulkanPipeline.h"
#include "Trinity/Renderer/Vulkan/VulkanBuffer.h"
#include "Trinity/Renderer/Vulkan/VulkanUploadContext.h"
#include "Trinity/Renderer/Vulkan/VulkanResourceStateTracker.h"
#include "Trinity/Renderer/Vulkan/VulkanDescriptorAllocator.h"
#include "Trinity/Renderer/Vulkan/VulkanTexture.h"
#include "Trinity/Renderer/Vulkan/VulkanGeometryBuffer.h"

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

			std::string m_VertexShaderName = "Simple.vert";
			std::string m_VertexShaderPath = "Assets/Shaders/Simple.vert.spv";

			std::string m_FragmentShaderName = "Simple.frag";
			std::string m_FragmentShaderPath = "Assets/Shaders/Simple.frag.spv";
		};

		VulkanRenderer();
		explicit VulkanRenderer(const Configuration& configuration);

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

		void ReloadShaders();

		void DrawMesh(Geometry::PrimitiveType primitive, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection) override;
		void DrawMesh(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::vec4& color, const glm::mat4& viewProjection) override;
		void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color) override;
		void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& viewProjection) override;
		void DrawMesh(Geometry::PrimitiveType primitive, const glm::vec3& position, const glm::vec4& color, const glm::mat4& view, const glm::mat4& projection) override;

		void BeginShadowPass(const glm::mat4& lightSpaceMatrix) override;
		void EndShadowPass() override;
		void DrawMeshShadow(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& lightSpaceMVP) override;

		void BeginGeometryPass() override;
		void EndGeometryPass() override;
		void DrawMeshDeferred(VertexBuffer& vertexBuffer, IndexBuffer& indexBuffer, uint32_t indexCount, const glm::mat4& model, const glm::mat4& viewProjection, const glm::vec4& color, Texture2D* albedoTexture) override;

		void BeginLightingPass() override;
		void EndLightingPass() override;
		void UploadLights(const void* lightData, uint32_t byteSize) override;
		void DrawLightingQuad(const glm::mat4& invViewProjection, const glm::vec3& cameraPosition, float cameraNear, float cameraFar) override;

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
		VulkanAllocator& GetVMAAllocator() { return m_Allocator; }
		VulkanUploadContext& GetUploadContext() { return m_UploadContext; }

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
			GeneralComputeReadWrite,
			DepthShaderReadOnly
		};

		static VulkanImageTransitionState BuildTransitionState(ImageTransitionPreset preset);
		static VkImageSubresourceRange BuildColorSubresourceRange();
		static VkImageSubresourceRange BuildDepthSubresourceRange();

		void RecreateSwapchain(uint32_t width, uint32_t height);
		void TransitionImageResource(VkCommandBuffer commandBuffer, VkImage image, const VkImageSubresourceRange& subresourceRange, const VulkanImageTransitionState& newState);

		void RecreateSceneViewportTarget();
		void DestroySceneViewportTarget();
		void BeginPresentPass(VkCommandBuffer commandBuffer, const VkImageSubresourceRange& colorSubresourceRange, const VulkanImageTransitionState& colorAttachmentWriteState);

		void EnsurePrimitiveUploaded(Geometry::PrimitiveType primitive);
		void ValidateSceneColorPolicy() const;

		void InitializeDeferredResources();
		void ShutdownDeferredResources();
		void RecreateDeferredResources();

		void CreateShadowPipeline();
		void CreateGeometryBufferPipeline();
		void CreateLightingPipeline();
		void DestroyDeferredPipelines();

		VkDescriptorSet BuildGeometryBufferDescriptorSet();
		VkDescriptorSet BuildTextureDescriptorSet(VkImageView imageView, VkSampler sampler);

	private:
		struct PrimitiveGpu
		{
			std::unique_ptr<VulkanVertexBuffer> VulkanVB;
			std::unique_ptr<VulkanIndexBuffer> VulkanIB;
		};

		static constexpr uint32_t s_ShadowMapSize = 2048;
		static constexpr uint32_t s_MaxTextureDescriptorsPerFrame = 1024;

		Window* m_Window = nullptr;

		ShaderLibrary m_ShaderLibrary{};

		VulkanContext m_Context{};
		VulkanDevice m_Device{};
		VulkanAllocator m_Allocator{};
		VulkanSwapchain m_Swapchain{};
		VulkanSync m_Sync{};
		VulkanCommand m_Command{};
		VulkanPipeline m_Pipeline{};
		VulkanUploadContext m_UploadContext{};
		VulkanDescriptorAllocator m_DescriptorAllocator{};
		VulkanTexture2D m_WhiteTexture{};
		VulkanGeometryBuffer m_GBuffer{};

		uint32_t m_FramesInFlight = 2;
		uint32_t m_CurrentFrameIndex = 0;
		uint32_t m_CurrentImageIndex = 0;
		bool m_FrameBegun = false;

		ImGuiLayer* m_ImGuiLayer = nullptr;
		bool m_ImGuiVulkanInitialized = false;

		VulkanResourceStateTracker m_ResourceStateTracker{};

		std::array<PrimitiveGpu, static_cast<size_t>(Geometry::PrimitiveType::Count)> m_Primitives{};

		Configuration m_Configuration{};

		uint32_t m_SceneViewportWidth = 0;
		uint32_t m_SceneViewportHeight = 0;
		VkImage m_SceneViewportImage = VK_NULL_HANDLE;
		VmaAllocation m_SceneViewportImageAllocation = VK_NULL_HANDLE;
		VkImageView m_SceneViewportImageView = VK_NULL_HANDLE;
		VkFormat m_SceneViewportDepthFormat = VK_FORMAT_UNDEFINED;
		VkImage m_SceneViewportDepthImage = VK_NULL_HANDLE;
		VmaAllocation m_SceneViewportDepthImageAllocation = VK_NULL_HANDLE;
		VkImageView m_SceneViewportDepthImageView = VK_NULL_HANDLE;
		VkSampler m_SceneViewportSampler = VK_NULL_HANDLE;
		void* m_SceneViewportHandle = nullptr;
		bool m_ScenePassRecording = false;
		bool m_PresentPassRecording = false;

		bool m_PendingViewportRecreate = false;
		uint32_t m_PendingWidth = 0;
		uint32_t m_PendingHeight = 0;

		VkImage m_ShadowMapImage = VK_NULL_HANDLE;
		VmaAllocation m_ShadowMapAllocation = VK_NULL_HANDLE;
		VkImageView m_ShadowMapView = VK_NULL_HANDLE;
		VkSampler m_ShadowMapSampler = VK_NULL_HANDLE;
		VkSampler m_GBufferSampler = VK_NULL_HANDLE;

		VkPipeline m_ShadowPipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_ShadowPipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_ShadowTextureSetLayout = VK_NULL_HANDLE;

		VkPipeline m_GBufferPipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_GBufferPipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_GBufferTextureSetLayout = VK_NULL_HANDLE;

		VkPipeline m_LightingPipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_LightingPipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_LightingGBufferSetLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_LightingUBOSetLayout = VK_NULL_HANDLE;

		std::vector<VkDescriptorPool> m_LightingDescriptorPools;
		std::vector<VulkanUniformBuffer> m_LightingUBOs;

		bool m_ShadowPassRecording = false;
		bool m_GeometryPassRecording = false;
		bool m_LightingPassRecording = false;

		glm::mat4 m_CurrentLightSpaceMatrix = glm::mat4(1.0f);
	};
}