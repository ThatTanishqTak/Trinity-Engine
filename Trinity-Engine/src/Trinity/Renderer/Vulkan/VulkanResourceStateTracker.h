#pragma once

#include "Trinity/Renderer/Vulkan/VulkanSync.h"

#include <cstdint>
#include <unordered_map>
#include <type_traits>

namespace Trinity
{
	class VulkanResourceStateTracker
	{
	public:
		void Reset();
		void ForgetImage(VkImage image);
		bool TransitionImageResource(VkCommandBuffer commandBuffer, VkImage image, const VkImageSubresourceRange& subresourceRange, const VulkanImageTransitionState& newState);

	private:
		struct SubresourceBucket
		{
			VkImageAspectFlags m_AspectMask = 0;
			uint32_t m_BaseMipLevel = 0;
			uint32_t m_LevelCount = 0;
			uint32_t m_BaseArrayLayer = 0;
			uint32_t m_LayerCount = 0;

			bool operator==(const SubresourceBucket& other) const = default;
		};

		struct ResourceBucketKey
		{
			uint64_t m_ImageHandle = 0;
			SubresourceBucket m_SubresourceBucket{};

			bool operator==(const ResourceBucketKey& other) const = default;
		};

		struct TrackedResourceState
		{
			VulkanImageTransitionState m_State = CreateVulkanImageTransitionState(VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE);
			bool m_IsInitialized = false;
		};

		struct ResourceBucketKeyHasher
		{
			size_t operator()(const ResourceBucketKey& resourceBucketKey) const;
		};

		static uint64_t EncodeImageHandle(VkImage image);
		static SubresourceBucket BuildSubresourceBucket(const VkImageSubresourceRange& subresourceRange);
		static ResourceBucketKey BuildResourceBucketKey(VkImage image, const VkImageSubresourceRange& subresourceRange);

	private:
		std::unordered_map<ResourceBucketKey, TrackedResourceState, ResourceBucketKeyHasher> m_TrackedResourceStates{};
	};
}