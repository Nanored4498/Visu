#include "device.h"

#include "commandbuffer.h"
#include "debug.h"

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <limits>
#include <vector>

namespace gfx {

const uint32_t QueueFamilies::NOT_AN_ID = std::numeric_limits<uint32_t>::max();

static const char* RequiredExtensions[] {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static QueueFamilies findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
	const std::vector<VkQueueFamilyProperties> queueFamilies = vkGetList(vkGetPhysicalDeviceQueueFamilyProperties, device);

	QueueFamilies indices;
	for(uint32_t i = 0; i < (uint32_t) queueFamilies.size(); ++i) {
		if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsId = i;
		VkBool32 surfaceSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &surfaceSupport);
		if(surfaceSupport) {
			indices.presentId = i;
			if(indices.presentId == indices.graphicsId) return indices;
		}
	}

	return indices;
}

static inline bool extensionAvailable(const std::vector<VkExtensionProperties> &properties, const char* extension) {
	return std::ranges::find_if(properties, [&](const VkExtensionProperties &e) {
		return !strcmp(e.extensionName, extension);
	}) != properties.end();
}

static bool isPhysicalDeviceSuitable(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
	// Need geometry shader
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(gpu, &deviceFeatures);
	if(!deviceFeatures.geometryShader) return false;

	// Check queues
	QueueFamilies queueIndices = findQueueFamilies(gpu, surface);
	if(!queueIndices.correct()) return false;

	// Check extensions
	const std::vector<VkExtensionProperties> properties = vkGetList(vkEnumerateDeviceExtensionProperties, gpu, nullptr);
	for(const char* ext : RequiredExtensions)
		if(!extensionAvailable(properties, ext))
			return false;

	// Check format and present mode availability
	if(!vkGetListSize(vkGetPhysicalDeviceSurfaceFormatsKHR, gpu, surface)) return false;
	if(!vkGetListSize(vkGetPhysicalDeviceSurfacePresentModesKHR, gpu, surface)) return false;

	return true;
}

std::vector<VkPhysicalDevice> Device::getAvailableDevices(const Instance &instance, const Window &window) {
	std::vector<VkPhysicalDevice> devices = vkGetList(vkEnumeratePhysicalDevices, (VkInstance) instance);
	if(devices.empty()) THROW_ERROR("failed to find a GPU with Vulkan support!");
	for(std::size_t i = 0; i < devices.size(); ++i) if(!isPhysicalDeviceSuitable(devices[i], window.getSurface())) {
		devices[i] = devices.back();
		devices.pop_back();
		--i;
	}
	if(devices.empty()) THROW_ERROR("failed to find a GPU which supports the window!");
	return devices;
}

void Device::init(const Window &window, VkPhysicalDevice gpu) {
	clean();

	this->gpu = gpu;

	// Choose queue families
	queueFamilies = findQueueFamilies(gpu, window.getSurface());
	std::vector<VkDeviceQueueCreateInfo> queueInfos;
	float queuePriority = 1.f;
	const auto addQ = [&](uint32_t i) {
		queueInfos.push_back({
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0u,
			.queueFamilyIndex = i,
			.queueCount = 1u,
			.pQueuePriorities = &queuePriority
		});
	};
	addQ(queueFamilies.graphicsId);
	if(queueFamilies.presentId != queueFamilies.graphicsId) addQ(queueFamilies.presentId);

	std::vector<const char*> extensions(RequiredExtensions, RequiredExtensions + std::size(RequiredExtensions));
	#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
	const std::vector<VkExtensionProperties> properties = vkGetList(vkEnumerateDeviceExtensionProperties, gpu, nullptr);
	if(extensionAvailable(properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))
		extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
	#endif

	features = {};
	// VkPhysicalDeviceFeatures deviceFeatures;
	// vkGetPhysicalDeviceFeatures(gpu, &deviceFeatures);
	// features.samplerAnisotropy = deviceFeatures.samplerAnisotropy;

	// Create logical device
	VkDeviceCreateInfo deviceInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.queueCreateInfoCount = (uint32_t) queueInfos.size(),
		.pQueueCreateInfos = queueInfos.data(),
		.enabledLayerCount = 0u,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = (uint32_t) extensions.size(),
		.ppEnabledExtensionNames = extensions.data(),
		.pEnabledFeatures = &features
	};
	#ifndef NDEBUG
		deviceInfo.enabledLayerCount = 1u;
		deviceInfo.ppEnabledLayerNames = &Instance::validation_layer;
	#endif
	if(vkCreateDevice(gpu, &deviceInfo, nullptr, &device) != VK_SUCCESS)
		THROW_ERROR("failed to create logical device!");
		
	// Get queues
	vkGetDeviceQueue(device, queueFamilies.graphicsId, 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamilies.presentId, 0, &presentQueue);

	// Create command pool
	VkCommandPoolCreateInfo poolInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = queueFamilies.graphicsId
	};

	if(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		THROW_ERROR("failed to create command pool!");

	// Create OT fence
	const VkFenceCreateInfo fenceInfo {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u
	};
	if(vkCreateFence(this->device = device, &fenceInfo, nullptr, &OTFence) != VK_SUCCESS)
		THROW_ERROR("failed to create OT fence!");
}

void Device::clean() {
	if(!device) return;
	vkDestroyFence(device, OTFence, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
	vkDestroyDevice(device, nullptr);
	device = nullptr;
}

CommandBuffer Device::createCommandBuffer(bool primary) const {
	CommandBuffer cmdBuf;
	allocCommandBuffers(&cmdBuf, 1u, primary);
	return cmdBuf;
}

void Device::allocCommandBuffers(CommandBuffer *cmdBufs, uint32_t size, bool primary) const {
	const VkCommandBufferAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = commandPool,
		.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY,
		.commandBufferCount = size
	};
	if(vkAllocateCommandBuffers(device, &allocInfo, reinterpret_cast<VkCommandBuffer*>(cmdBufs)) != VK_SUCCESS)
		THROW_ERROR("failed to allocate command buffers!");
}

uint32_t Device::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
	VkPhysicalDeviceMemoryProperties memProp;
	vkGetPhysicalDeviceMemoryProperties(gpu, &memProp);
	for(uint32_t i = 0u; i < memProp.memoryTypeCount; ++i)
		if(((1 << i) & typeFilter) && (memProp.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	THROW_ERROR("failed to find suitable memory type!");
}

}