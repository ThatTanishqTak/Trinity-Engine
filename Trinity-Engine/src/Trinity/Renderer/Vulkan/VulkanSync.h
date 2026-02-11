#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Trinity
{
	struct VulkanImageTransitionState
	{
		VkPipelineStageFlags2 m_StageMask = VK_PIPELINE_STAGE_2_NONE;
		VkAccessFlags2 m_AccessMask = VK_ACCESS_2_NONE;
		VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;
		uint32_t m_QueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	};

	[[nodiscard]] constexpr VulkanImageTransitionState CreateVulkanImageTransitionState(VkImageLayout layout, VkPipelineStageFlags2 stageMask, VkAccessFlags2 accessMask, uint32_t queueFamilyIndex = VK_QUEUE_FAMILY_IGNORED)
	{
		VulkanImageTransitionState l_State{};
		l_State.m_Layout = layout;
		l_State.m_StageMask = stageMask;
		l_State.m_AccessMask = accessMask;
		l_State.m_QueueFamilyIndex = queueFamilyIndex;

		return l_State;
	}

	inline constexpr VulkanImageTransitionState g_PresentImageState = CreateVulkanImageTransitionState(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE);
	inline constexpr VulkanImageTransitionState g_ColorAttachmentWriteImageState = CreateVulkanImageTransitionState(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
	inline constexpr VulkanImageTransitionState g_TransferSourceImageState = CreateVulkanImageTransitionState(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT);
	inline constexpr VulkanImageTransitionState g_TransferDestinationImageState = CreateVulkanImageTransitionState(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);
	inline constexpr VulkanImageTransitionState g_ShaderReadOnlyImageState = CreateVulkanImageTransitionState(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
	inline constexpr VulkanImageTransitionState g_GeneralComputeReadWriteImageState = CreateVulkanImageTransitionState(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);

	void TransitionImage(VkCommandBuffer commandBuffer, VkImage image, const VulkanImageTransitionState& oldState, const VulkanImageTransitionState& newState,
		const VkImageSubresourceRange& subresourceRange);

	class VulkanContext;
	class VulkanDevice;

	class VulkanSync
	{
	public:
		void Initialize(const VulkanContext& context, const VulkanDevice& device, uint32_t framesInFlight, uint32_t swapchainImageCount);
		void Shutdown();

		void OnSwapchainRecreated(uint32_t swapchainImageCount);

		uint32_t GetFramesInFlight() const { return m_FramesInFlight; }
		uint32_t GetSwapchainImageCount() const { return m_SwapchainImageCount; }

		VkSemaphore GetImageAvailableSemaphore(uint32_t frameIndex) const;
		VkSemaphore GetRenderFinishedSemaphore(uint32_t imageIndex) const;
		VkFence GetInFlightFence(uint32_t frameIndex) const;

		void WaitForFrameFence(uint32_t frameIndex, uint64_t timeout = UINT64_MAX) const;
		void ResetFrameFence(uint32_t frameIndex) const;

		VkFence GetImageInFlightFence(uint32_t imageIndex) const;
		void SetImageInFlightFence(uint32_t imageIndex, VkFence fence);
		void ClearImagesInFlight();

	private:
		void DestroySyncObjects();

		void DestroyRenderFinishedSemaphores();
		void CreateRenderFinishedSemaphores(uint32_t swapchainImageCount);

		void ValidateFrameIndex(uint32_t frameIndex) const;
		void ValidateImageIndex(uint32_t imageIndex) const;

	private:
		VkAllocationCallbacks* m_Allocator = nullptr;
		VkDevice m_Device = VK_NULL_HANDLE;

		uint32_t m_FramesInFlight = 0;
		uint32_t m_SwapchainImageCount = 0;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores{};
		std::vector<VkSemaphore> m_RenderFinishedSemaphores{};
		std::vector<VkFence> m_InFlightFences{};
		std::vector<VkFence> m_ImagesInFlight{};
	};
}