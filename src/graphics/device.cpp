#include "device.h"

#include "util.h"
#include "config.h"

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <limits>
#include <vector>

namespace gfx {

const uint32_t QueueFamilies::NOT_AN_ID = std::numeric_limits<uint32_t>::max();

static const char* RequiredExtensions[] {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	#ifdef __APPLE__
	, VK_KHR_portability_subset
	#endif
};

void Device::init(Instance &instance, const Window &window) {
	clean();
	pickPhysicalDevice(instance, window);
	createLogicalDevice(window);
}

void Device::clean() {
	if(!device) return;
	vkDestroyDevice(device, nullptr);
	device = nullptr;
}

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

static VkPhysicalDeviceProperties getPhysicalDeviceProperty(VkPhysicalDevice gpu) {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(gpu, &deviceProperties);
	return deviceProperties;
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
	const std::vector<VkExtensionProperties> extensions = vkGetList(vkEnumerateDeviceExtensionProperties, gpu, nullptr);
	for(const char* ext : RequiredExtensions)
		if(std::ranges::find_if(extensions, [&](const VkExtensionProperties &e) { return !strcmp(e.extensionName, ext); }) == extensions.end())
			return false;

	// Check format and present mode availability
	if(!vkGetListSize(vkGetPhysicalDeviceSurfaceFormatsKHR, gpu, surface)) return false;
	if(!vkGetListSize(vkGetPhysicalDeviceSurfacePresentModesKHR, gpu, surface)) return false;

	return true;
}

void Device::pickPhysicalDevice(Instance &instance, const Window &window) {
	const std::vector<VkPhysicalDevice> devices = vkGetList(vkEnumeratePhysicalDevices, (VkInstance) instance);
	if(devices.empty()) THROW_ERROR("failed to find a GPU with Vulkan support!");

	for(VkPhysicalDevice candidate : devices)
		if(isPhysicalDeviceSuitable(candidate, window.getSurface()) && (!gpu || !strcmp(getPhysicalDeviceProperty(candidate).deviceName, Config::data.preferred_gpu)))
			gpu = candidate;

	if(!gpu) THROW_ERROR("failed to find a suitable GPU!");
	PRINT_INFO("Using Physical Device:", getPhysicalDeviceProperty(gpu).deviceName);
}

void Device::createLogicalDevice(const Window &window) {
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

	features = {};
	// VkPhysicalDeviceFeatures deviceFeatures;
	// vkGetPhysicalDeviceFeatures(gpu, &deviceFeatures);
	// features.samplerAnisotropy = deviceFeatures.samplerAnisotropy;

	VkDeviceCreateInfo deviceInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.queueCreateInfoCount = (uint32_t) queueInfos.size(),
		.pQueueCreateInfos = queueInfos.data(),
		.enabledLayerCount = 0u,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = sizeof(RequiredExtensions) / sizeof(RequiredExtensions[0]),
		.ppEnabledExtensionNames = RequiredExtensions,
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
}

}