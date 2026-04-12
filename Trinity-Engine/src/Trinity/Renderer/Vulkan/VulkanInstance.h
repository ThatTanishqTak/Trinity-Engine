#pragma once

#include <vulkan/vulkan.h>

namespace Trinity
{
	class VulkanInstance
	{
	public:
		VulkanInstance() = default;
		~VulkanInstance() = default;

		void Initialize();
		void Shutdown();
	};
}