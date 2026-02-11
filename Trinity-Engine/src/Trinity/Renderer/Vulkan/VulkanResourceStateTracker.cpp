#include "Trinity/Renderer/Vulkan/VulkanResourceStateTracker.h"

#include <functional>

namespace Trinity
{
	size_t VulkanResourceStateTracker::ResourceBucketKeyHasher::operator()(const ResourceBucketKey& resourceBucketKey) const
	{
		size_t l_Seed = std::hash<uint64_t>{}(resourceBucketKey.m_ImageHandle);
		l_Seed ^= std::hash<uint32_t>{}(resourceBucketKey.m_SubresourceBucket.m_AspectMask) + 0x9e3779b9 + (l_Seed << 6) + (l_Seed >> 2);
		l_Seed ^= std::hash<uint32_t>{}(resourceBucketKey.m_SubresourceBucket.m_BaseMipLevel) + 0x9e3779b9 + (l_Seed << 6) + (l_Seed >> 2);
		l_Seed ^= std::hash<uint32_t>{}(resourceBucketKey.m_SubresourceBucket.m_LevelCount) + 0x9e3779b9 + (l_Seed << 6) + (l_Seed >> 2);
		l_Seed ^= std::hash<uint32_t>{}(resourceBucketKey.m_SubresourceBucket.m_BaseArrayLayer) + 0x9e3779b9 + (l_Seed << 6) + (l_Seed >> 2);
		l_Seed ^= std::hash<uint32_t>{}(resourceBucketKey.m_SubresourceBucket.m_LayerCount) + 0x9e3779b9 + (l_Seed << 6) + (l_Seed >> 2);

		return l_Seed;
	}

	uint64_t VulkanResourceStateTracker::EncodeImageHandle(VkImage image)
	{
		if constexpr (std::is_pointer_v<VkImage>)
		{
			return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(image));
		}
		else
		{
			return reinterpret_cast<uint64_t>(image);
		}
	}

	VulkanResourceStateTracker::SubresourceBucket VulkanResourceStateTracker::BuildSubresourceBucket(const VkImageSubresourceRange& subresourceRange)
	{
		SubresourceBucket l_SubresourceBucket{};
		l_SubresourceBucket.m_AspectMask = subresourceRange.aspectMask;
		l_SubresourceBucket.m_BaseMipLevel = subresourceRange.baseMipLevel;
		l_SubresourceBucket.m_LevelCount = subresourceRange.levelCount;
		l_SubresourceBucket.m_BaseArrayLayer = subresourceRange.baseArrayLayer;
		l_SubresourceBucket.m_LayerCount = subresourceRange.layerCount;

		return l_SubresourceBucket;
	}

	VulkanResourceStateTracker::ResourceBucketKey VulkanResourceStateTracker::BuildResourceBucketKey(VkImage image, const VkImageSubresourceRange& subresourceRange)
	{
		ResourceBucketKey l_ResourceBucketKey{};
		l_ResourceBucketKey.m_ImageHandle = EncodeImageHandle(image);
		l_ResourceBucketKey.m_SubresourceBucket = BuildSubresourceBucket(subresourceRange);

		return l_ResourceBucketKey;
	}

	void VulkanResourceStateTracker::Reset()
	{
		m_TrackedResourceStates.clear();
	}

	void VulkanResourceStateTracker::ForgetImage(VkImage image)
	{
		const uint64_t l_ImageHandle = EncodeImageHandle(image);

		for (auto it_State = m_TrackedResourceStates.begin(); it_State != m_TrackedResourceStates.end();)
		{
			if (it_State->first.m_ImageHandle == l_ImageHandle)
			{
				it_State = m_TrackedResourceStates.erase(it_State);
			}
			else
			{
				++it_State;
			}
		}
	}

	bool VulkanResourceStateTracker::TransitionImageResource(VkCommandBuffer commandBuffer, VkImage image, const VkImageSubresourceRange& subresourceRange, const VulkanImageTransitionState& newState)
	{
		const ResourceBucketKey l_ResourceBucketKey = BuildResourceBucketKey(image, subresourceRange);
		auto& a_TrackedResourceState = m_TrackedResourceStates[l_ResourceBucketKey];

		const bool l_RequiresTransition = !a_TrackedResourceState.m_IsInitialized || a_TrackedResourceState.m_State.m_Layout != newState.m_Layout
			|| a_TrackedResourceState.m_State.m_StageMask != newState.m_StageMask || a_TrackedResourceState.m_State.m_AccessMask != newState.m_AccessMask
			|| a_TrackedResourceState.m_State.m_QueueFamilyIndex != newState.m_QueueFamilyIndex;

		if (!l_RequiresTransition)
		{
			return false;
		}

		const VulkanImageTransitionState l_OldState = a_TrackedResourceState.m_IsInitialized ? a_TrackedResourceState.m_State : CreateVulkanImageTransitionState(VK_IMAGE_LAYOUT_UNDEFINED, VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE);

		Trinity::TransitionImage(commandBuffer, image, l_OldState, newState, subresourceRange);

		a_TrackedResourceState.m_State = newState;
		a_TrackedResourceState.m_IsInitialized = true;

		return true;
	}
}