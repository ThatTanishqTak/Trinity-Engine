#include "Trinity/Renderer/Vulkan/VulkanContext.h"

#include "Trinity/Renderer/Vulkan/VulkanInstance.h"
#include "Trinity/Renderer/Vulkan/VulkanSurface.h"
#include "Trinity/Renderer/Vulkan/VulkanDevice.h"

namespace Trinity
{
	VulkanContext VulkanContext::Initialize(VulkanInstance& instance, VulkanSurface& surface, const VulkanDevice& device)
	{
		VulkanContext l_Context{};
		l_Context.Instance = instance.GetInstance();
		l_Context.Surface = surface.GetSurface();
		l_Context.PhysicalDevice = device.GetPhysicalDevice();
		l_Context.Device = device.GetDevice();
		l_Context.Allocator = instance.GetAllocator();
		l_Context.DeviceRef = &device;

		l_Context.Queues.GraphicsQueue = device.GetGraphicsQueue();
		l_Context.Queues.PresentQueue = device.GetPresentQueue();
		l_Context.Queues.TransferQueue = device.GetTransferQueue();
		l_Context.Queues.ComputeQueue = device.GetComputeQueue();

		l_Context.Queues.GraphicsFamilyIndex = device.GetGraphicsQueueFamilyIndex();
		l_Context.Queues.PresentFamilyIndex = device.GetPresentQueueFamilyIndex();
		l_Context.Queues.TransferFamilyIndex = device.GetTransferQueueFamilyIndex();
		l_Context.Queues.ComputeFamilyIndex = device.GetComputeQueueFamilyIndex();

		return l_Context;
	}
}